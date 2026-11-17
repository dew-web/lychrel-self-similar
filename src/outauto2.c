// outauto2.c — 端のオートマトン（outauto.c とは独立に書き直した版）
//
// 目的は outauto.c と同じ: 周期 P・桁あふれパターン ov を固定して、両端の窓を「外側から1対ずつ内側へ」
// 組み立てる有限オートマトンの到達状態を調べ尽くし、端の解（受理状態）があるかを長さ無制限で判定する。
//
// outauto.c との違い:
//   1. 状態キーは 128 ビット（数字は必要なビット数だけ使う）。親を持たない判定専用モードが既定で、1状態 約24バイト。
//   2. 回転対称性で状態をまとめる。ov が r 位相ずらしても同じなら（例: 0101... は r=2）、状態を r 位相ずらしたものも
//      到達可能で、遷移も受理条件も回転と可換。だから回転で移り合う状態は代表（キー最小）だけを持てばよい。
//      軌道の大きさを数えているので、対称性なしの到達状態数（outauto.c の reachable と同じ値）も出力する。
//   3. 表は必要に応じて大きくなる（上限 2^cap 状態）。
//   4. -w を付けると親を記録し、受理状態ごとに窓を復元して、独立な直接シミュレーションで確かめてから
//      outer_*.txt（coreauto2 が読む形式）に書く。
//
// 状態 = 各位相 t の (cb_t, ca_t, pd_t):
//   cb_t = 下側（下位）の繰り上がり。外から内へ流れ、次の対に入る。
//   ca_t = 上側（上位）で、次の内側の対から出てくるはずの繰り上がり（外側の対がそう仮定した）。
//   pd_t = 位相 t で桁あふれが起きるとき、上側の窓は1つ内側へずれる。そのとき次の対へ渡す数字。
// 受理:
//   型A（0 と ω=B-1 の列が鏡像に並ぶ）: ca_t = 1-cb_t かつ pd_t = ω·cb_t
//   型C（中央に一様な列 d_t）        : cb_t = ca_t = g_t かつ pd_t = d_{t+1}、さらに回文を避ける旗が全部立っている
//
// usage:
//   outauto2 [opts] A B <ov文字列>          1パターン（型A）
//   outauto2 [opts] C B <ov文字列> d0       1パターン（型C、中央列の最初の数字 d0）
//   outauto2 [opts] A B <P>                 周期 P の全パターン（回転の代表、桁あふれは偶数個）
//   outauto2 [opts] C B <P> d0              周期 P の全パターン（型C）
// opts:
//   -w          窓を復元して outer_*.txt を書く（1状態あたりのメモリが増える）
//   -nosym      回転対称性を使わない（検証用）
//   -cap N      状態数の上限 2^N（既定 26）
//   -layers     BFS の段ごとの状態数を stderr に出す
//   -part i/n   全パターン走査のとき、番号が n で割って i 余るパターンだけ調べる（並列実行用）
//   -min N      全パターン走査のとき、到達状態が N 以上のパターンだけ行を出す（既定 0 = 全部）
//   -odd        全パターン走査（型A）で、桁あふれが奇数個のパターンだけ調べる。端は周期 P で閉じるが、
//               中心は半桁ずれるので、数全体の周期は 2P になる（中心は ov と信号を2回くり返して調べる）
//   -rep k      全パターン走査のとき、長さ P/k の語 w を k 回くり返した形（w^k）のパターンだけ調べる
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
typedef unsigned __int128 u128;

#define MAXP 40
static int B, P, OV[MAXP], NOV, TYPEC, DBITS;
static int Dg[MAXP + 1], Gg[MAXP + 1], NEEDBAD[MAXP], NEEDNB[MAXP], FB[MAXP], FN[MAXP], NF;
static int R0 = 0, NROT = 1, MPD = 0, MFL = 0;       // 回転の最小単位 R0、回転の個数、1ブロックあたりの ov 数と旗の数
static int SYM = 1, WANTW = 0, LAYERS = 0, CAPLOG = 26;

// ---------------- 状態の表 ----------------
static u128 *KEYS; static uint8_t *ORB;                // ORB = 軌道の大きさ（対称性なしの状態数を数えるため）
static uint32_t *SLOT; static uint64_t NK, KALLOC, HSIZE;
typedef struct { uint32_t parent; uint32_t ch; uint16_t a0, b0; uint8_t rot, isroot; } meta_t;
static meta_t *META;
static int cap_hit;

static inline uint64_t hash128(u128 k) {
    uint64_t lo = (uint64_t)k, hi = (uint64_t)(k >> 64);
    uint64_t x = lo * 0x9E3779B97F4A7C15ull ^ (hi + 0x632BE59BD9B4E019ull) * 0xC2B2AE3D27D4EB4Full;
    x ^= x >> 29; x *= 0xBF58476D1CE4E5B9ull; x ^= x >> 32;
    return x;
}
static uint64_t KLIM;                                   // いまの論理的な容量（KALLOC は確保済みの大きさ）。常に HSIZE = 2·KLIM
static uint32_t USED[1u << 16]; static int grown;      // 小さい探索では、使った場所だけ消す（全パターン走査を速くするため）
static void table_reset(void) {
    if (!KEYS) { KALLOC = 1u << 16; KEYS = malloc(sizeof(u128) * KALLOC); ORB = malloc(KALLOC); if (WANTW) META = malloc(sizeof(meta_t) * KALLOC); }
    if (!SLOT || grown) { free(SLOT); HSIZE = 1u << 17; SLOT = calloc(HSIZE, sizeof(uint32_t)); }
    else for (uint64_t i = 0; i < NK; i++) SLOT[USED[i]] = 0;
    NK = 0; cap_hit = 0; KLIM = 1u << 16; grown = 0;
}
static void table_grow(void) {
    KLIM *= 2; grown = 1;
    if (KLIM > KALLOC) {
        KALLOC = KLIM;
        KEYS = realloc(KEYS, sizeof(u128) * KALLOC); ORB = realloc(ORB, KALLOC); if (WANTW) META = realloc(META, sizeof(meta_t) * KALLOC);
    }
    free(SLOT); HSIZE = 2 * KLIM; SLOT = calloc(HSIZE, sizeof(uint32_t));
    if (!KEYS || !ORB || !SLOT || (WANTW && !META)) { fprintf(stderr, "out of memory at %llu states\n", (unsigned long long)NK); exit(2); }
    for (uint64_t i = 0; i < NK; i++) { uint64_t h = hash128(KEYS[i]) & (HSIZE - 1); while (SLOT[h]) h = (h + 1) & (HSIZE - 1); SLOT[h] = (uint32_t)(i + 1); }
}
// 見つかれば番号、なければ追加して番号を返す。*isnew: 1=新規, 0=既存, -1=上限超過
static int64_t find_or_add(u128 key, int *isnew) {
    uint64_t h = hash128(key) & (HSIZE - 1);
    while (SLOT[h]) { if (KEYS[SLOT[h] - 1] == key) { *isnew = 0; return SLOT[h] - 1; } h = (h + 1) & (HSIZE - 1); }
    if (NK >= ((uint64_t)1 << CAPLOG)) { *isnew = -1; return -1; }
    if (NK == KLIM) { table_grow(); h = hash128(key) & (HSIZE - 1); while (SLOT[h]) h = (h + 1) & (HSIZE - 1); }
    if (!grown) USED[NK] = (uint32_t)h;
    SLOT[h] = (uint32_t)(NK + 1); KEYS[NK] = key; *isnew = 1; return (int64_t)NK++;
}

// ---------------- キーの詰め方と回転 ----------------
static inline u128 maskw(int w) { return w >= 128 ? ~(u128)0 : (((u128)1 << w) - 1); }
static u128 pack(const int *cb, const int *ca, const int *pd, uint64_t flags) {
    u128 k = 0; int sh = 0;
    for (int t = 0; t < P; t++) k |= (u128)cb[t] << sh++;
    for (int t = 0; t < P; t++) k |= (u128)ca[t] << sh++;
    for (int t = 0; t < P; t++) if (OV[t]) { k |= (u128)pd[t] << sh; sh += DBITS; }
    if (NF) k |= (u128)flags << sh;
    return k;
}
static void unpack(u128 k, int *cb, int *ca, int *pd, uint64_t *flags) {
    int sh = 0;
    for (int t = 0; t < P; t++) cb[t] = (int)(k >> sh++ & 1);
    for (int t = 0; t < P; t++) ca[t] = (int)(k >> sh++ & 1);
    for (int t = 0; t < P; t++) { if (OV[t]) { pd[t] = (int)(k >> sh & (((u128)1 << DBITS) - 1)); sh += DBITS; } else pd[t] = 0; }
    *flags = NF ? (uint64_t)(k >> sh & maskw(NF)) : 0;
}
static inline u128 rotr(u128 x, int w, int a) { if (!w || !a) return x; return ((x >> a) | (x << (w - a))) & maskw(w); }
// 状態を j·R0 位相ずらす: (ρS)_t = S_{t + j·R0}
static u128 rotkey(u128 k, int j) {
    int wpd = NOV * DBITS;
    u128 cb = k & maskw(P), ca = (k >> P) & maskw(P), pd = (k >> (2 * P)) & maskw(wpd), fl = NF ? (k >> (2 * P + wpd)) & maskw(NF) : 0;
    cb = rotr(cb, P, j * R0); ca = rotr(ca, P, j * R0); pd = rotr(pd, wpd, j * MPD * DBITS); fl = rotr(fl, NF, j * MFL);
    u128 r = cb | (ca << P) | (pd << (2 * P));
    if (NF) r |= fl << (2 * P + wpd);
    return r;
}

// ---------------- 遷移 ----------------
static int cur_cb[MAXP], cur_ca[MAXP], cur_pd[MAXP], cur_root; static uint64_t cur_flags; static uint32_t cur_parent;
static int n_cb[MAXP], n_ca[MAXP], n_pd[MAXP];
#define MAXTERM 100000
static int64_t TERMS[MAXTERM]; static int NTERMS; static int64_t NTERM_ALL;
static uint64_t NFULL;                                   // 対称性なしで数えた到達状態数（根を含む）

static int is_terminal(const int *cb, const int *ca, const int *pd, uint64_t flags) {
    if (!TYPEC) {
        for (int t = 0; t < P; t++) { if (ca[t] != 1 - cb[t]) return 0; if (OV[t] && pd[t] != (B - 1) * cb[t]) return 0; }
        return 1;
    }
    for (int t = 0; t < P; t++) { if (cb[t] != Gg[t] || ca[t] != Gg[t]) return 0; if (OV[t] && pd[t] != Dg[t + 1]) return 0; }
    return flags == (NF >= 64 ? ~0ull : ((1ull << NF) - 1));
}
static int COUNT_ONLY; static uint64_t NCOUNT;
static void emit(uint64_t flags, int a0, int b0, uint32_t ch) {
    if (COUNT_ONLY) { NCOUNT++; return; }
    u128 k = pack(n_cb, n_ca, n_pd, flags), kc = k; int rot = 0, orb = 1;
    if (NROT > 1) {
        u128 seen[MAXP]; seen[0] = k;
        for (int j = 1; j < NROT; j++) {
            u128 kr = rotkey(k, j); int dup = 0;
            for (int i = 0; i < orb; i++) if (seen[i] == kr) { dup = 1; break; }
            if (!dup) seen[orb++] = kr;
            if (kr < kc) { kc = kr; rot = j; }
        }
    }
    int isnew; int64_t id = find_or_add(kc, &isnew);
    if (isnew < 0) { cap_hit = 1; return; }
    if (!isnew) return;
    ORB[id] = (uint8_t)orb; NFULL += orb;
    if (META) { META[id].parent = cur_parent; META[id].ch = ch; META[id].a0 = (uint16_t)a0; META[id].b0 = (uint16_t)b0; META[id].rot = (uint8_t)rot; META[id].isroot = (uint8_t)cur_root; }
    if (is_terminal(n_cb, n_ca, n_pd, flags)) { NTERM_ALL += orb; if (NTERMS < MAXTERM) TERMS[NTERMS++] = id; }
}
static void dfs(int t, int a, int b, uint32_t ch, uint64_t flags, int a0, int b0) {
    if (cap_hit) return;
    if (t == P) { if (a == a0 && b == b0) emit(flags, a0, b0, ch); return; }
    if (cur_root && a == 0) return;                        // 先頭の数字は 0 でない
    int s = a + b, v = s + cur_cb[t], cbn = v >= B, nb = v - (cbn ? B : 0);
    n_cb[t] = cbn;
    uint64_t f2 = flags;
    if (NEEDBAD[t] && s >= B) f2 |= 1ull << FB[t];
    if (NEEDNB[t] && s != 0 && s != B + 1) f2 |= 1ull << FN[t];
    for (int c = 0; c <= 1; c++) {
        int w = s + c, co = w >= B;
        if (co != cur_ca[t]) continue;                     // 上側の桁から出る繰り上がりが、外側の要求と一致
        int ya = w - (co ? B : 0), an;
        n_ca[t] = c;
        if (OV[t]) { an = cur_pd[t]; n_pd[t] = ya; } else { an = ya; n_pd[t] = 0; }
        dfs(t + 1, an, nb, ch | ((uint32_t)c << t), f2, a0, b0);
    }
}
static void expand(void) { for (int a0 = 0; a0 < B; a0++) for (int b0 = 0; b0 < B; b0++) dfs(0, a0, b0, 0, cur_flags, a0, b0); }
static void set_root(void) { for (int t = 0; t < P; t++) { cur_cb[t] = 0; cur_ca[t] = OV[t]; cur_pd[t] = OV[t] ? 1 : 0; } cur_flags = 0; cur_root = 1; cur_parent = 0xFFFFFFFFu; }

// ---------------- 窓の復元と、独立な直接シミュレーションによる確認 ----------------
#define MAXK 4096
static int WA[MAXK][MAXP], WB[MAXK][MAXP];
// 親（根なら isroot）から (a0,b0,ch) で出る遷移の、各位相の数字の対
static void replay(int isroot, u128 pk, int a0, int b0, uint32_t ch, int *outa, int *outb) {
    int cb[MAXP], ca[MAXP], pd[MAXP]; uint64_t fl;
    if (isroot) { for (int t = 0; t < P; t++) { cb[t] = 0; pd[t] = OV[t] ? 1 : 0; } } else unpack(pk, cb, ca, pd, &fl);
    int a = a0, b = b0;
    for (int t = 0; t < P; t++) {
        outa[t] = a; outb[t] = b;
        int s = a + b, nb = s + cb[t]; nb -= nb >= B ? B : 0;
        int c = (ch >> t) & 1, ya = s + c; ya -= ya >= B ? B : 0;
        a = OV[t] ? pd[t] : ya; b = nb;
    }
}
// 受理状態 term から窓を復元する。戻り値は窓の長さ K。sig[t] に実際の（回転を戻した）信号 cb_t を入れる
static int reconstruct(int64_t term, int *sig) {
    static int64_t chain[MAXK]; int L = 0; int64_t v = term;
    while (1) { if (L >= MAXK) return -1; chain[L++] = v; if (META[v].isroot) break; v = META[v].parent; }
    int off = 0;                                           // 実際の状態 = 代表を off 位相だけ戻したもの
    for (int i = 0; i < L; i++) {
        int64_t e = chain[L - 1 - i]; int ta[MAXP], tb[MAXP];
        replay(META[e].isroot, META[e].isroot ? 0 : KEYS[META[e].parent], META[e].a0, META[e].b0, META[e].ch, ta, tb);
        for (int t = 0; t < P; t++) { int u = ((t - off) % P + P) % P; WA[i][t] = ta[u]; WB[i][t] = tb[u]; }
        off = (off + META[e].rot * R0) % P;
    }
    int cb[MAXP], ca[MAXP], pd[MAXP]; uint64_t fl; unpack(KEYS[term], cb, ca, pd, &fl);
    for (int t = 0; t < P; t++) sig[t] = cb[((t - off) % P + P) % P];
    return L;
}
// 窓だけを直接シミュレーションする（オートマトンの遷移とは別のコード）。列の向こうからの繰り上がりは信号で与える
static int check_outer(int K, const int *sig) {
    static int top[MAXK], bot[MAXK], nt[MAXK], nb[MAXK];
    for (int i = 0; i < K; i++) { top[i] = WA[i][0]; bot[i] = WB[i][0]; }
    if (top[0] == 0) return 0;
    for (int t = 0; t < P; t++) {
        for (int i = 0; i < K; i++) if (top[i] != WA[i][t] || bot[i] != WB[i][t]) return 0;
        int c = 0;
        for (int i = 0; i < K; i++) { int v = top[i] + bot[i] + c; c = v >= B; nb[i] = v - (c ? B : 0); }
        int H;
        if (!TYPEC) { if (c != sig[t]) return 0; H = 1 - sig[t]; }
        else { if (c != Gg[t]) return 0; H = Gg[t]; }
        for (int i = K - 1; i >= 0; i--) { int v = top[i] + bot[i] + H; H = v >= B; nt[i] = v - (H ? B : 0); }
        if (H != OV[t]) return 0;
        if (OV[t]) {
            int pushed = nt[K - 1], want = TYPEC ? Dg[t + 1] : (B - 1) * sig[t];
            if (pushed != want) return 0;
            for (int i = K - 1; i >= 1; i--) nt[i] = nt[i - 1];
            nt[0] = 1;
        }
        memcpy(top, nt, sizeof(int) * K); memcpy(bot, nb, sizeof(int) * K);
    }
    for (int i = 0; i < K; i++) if (top[i] != WA[i][0] || bot[i] != WB[i][0]) return 0;
    return 1;
}

// ---------------- 1パターンの探索 ----------------
static int setup_pattern(void) {
    NOV = 0; for (int t = 0; t < P; t++) NOV += OV[t];
    NF = 0; for (int t = 0; t < P; t++) { FB[t] = FN[t] = 0; if (NEEDBAD[t]) FB[t] = NF++; if (NEEDNB[t]) FN[t] = NF++; }
    if (2 * P + NOV * DBITS + NF > 128 || NF > 63) return 1;   // キーが128ビットに収まらない（呼び出し側で根だけ調べる）
    R0 = P;
    if (SYM) for (int r = 1; r < P; r++) {
        if (P % r) continue;
        int ok = 1;
        for (int t = 0; t < P && ok; t++) { if (OV[t] != OV[(t + r) % P]) ok = 0; if (TYPEC && Dg[t] != Dg[(t + r) % P]) ok = 0; }
        if (ok) { R0 = r; break; }
    }
    NROT = P / R0; MPD = 0; MFL = 0;
    for (int t = 0; t < R0; t++) { MPD += OV[t]; MFL += NEEDBAD[t] + NEEDNB[t]; }
    return 0;
}
static int MAXDEPTH;
static void run_pattern(void) {
    int toolong = setup_pattern(); table_reset(); NTERMS = 0; NTERM_ALL = 0; NFULL = 1; MAXDEPTH = 0;
    if (toolong) {
        // キーが長すぎるパターン: 根から出る遷移が1つもなければ「到達状態は根だけ」と確定できる。あれば未決として扱う
        COUNT_ONLY = 1; NCOUNT = 0; set_root(); expand(); COUNT_ONLY = 0;
        if (NCOUNT) { cap_hit = 1; fprintf(stderr, "key too long and root has successors: inconclusive\n"); }
        return;
    }
    set_root(); expand();
    uint64_t lend = NK, lstart = 0; int depth = 1;
    if (LAYERS) fprintf(stderr, "layer 0 size 1 total 1\n");
    for (uint64_t q = 0; q < NK && !cap_hit; q++) {
        if (q == lend) { if (LAYERS) fprintf(stderr, "layer %d size %llu total %llu\n", depth, (unsigned long long)(lend - lstart), (unsigned long long)lend + 1); depth++; lstart = lend; lend = NK; }
        u128 k = KEYS[q];
        unpack(k, cur_cb, cur_ca, cur_pd, &cur_flags); cur_root = 0; cur_parent = (uint32_t)q;
        expand();
        if (LAYERS && (q & 0xFFFFFF) == 0xFFFFFF) fprintf(stderr, "  progress: expanded %llu of %llu, depth %d\n", (unsigned long long)q + 1, (unsigned long long)NK, depth);
    }
    MAXDEPTH = depth;
    if (LAYERS) fprintf(stderr, "end depth %d total %llu%s\n", depth, (unsigned long long)NK + 1, cap_hit ? " CAPACITY" : "");
}
static void report(uint64_t minprint) {
    if (NFULL < minprint && !NTERMS && !cap_hit) return;
    char ovs[MAXP + 1]; for (int t = 0; t < P; t++) ovs[t] = '0' + OV[t]; ovs[P] = 0;
    printf("  ov=%s  reachable=%llu canon=%llu sym=%d depth=%d terminals=%lld%s\n", ovs, (unsigned long long)NFULL, (unsigned long long)NK + 1, NROT, MAXDEPTH, (long long)NTERM_ALL, cap_hit ? " (CAPACITY)" : "");
    for (int k = 0; k < NTERMS; k++) {
        int sig[MAXP]; char cbs[MAXP + 1];
        if (WANTW) {
            int K = reconstruct(TERMS[k], sig);
            for (int t = 0; t < P; t++) cbs[t] = '0' + sig[t]; cbs[P] = 0;
            if (K < 0) { printf("    terminal cin=%s  window too long to reconstruct\n", cbs); continue; }
            int ok = check_outer(K, sig);
            char fn[256];
            if (TYPEC) snprintf(fn, sizeof fn, "outerC_B%d_P%d_ov%s_d%d_%d.txt", B, P, ovs, Dg[0], k);
            else snprintf(fn, sizeof fn, "outer_B%d_P%d_ov%s_cin%s.txt", B, P, ovs, cbs);
            printf("    terminal cin=%s  K=%d  direct window simulation: %s  -> %s\n", cbs, K, ok ? "OK" : "FAILED", fn);
            FILE *f = fopen(fn, "w");
            fprintf(f, "%d %d %d %s %s\n", B, P, K, ovs, cbs);
            for (int t = 0; t < P; t++) { for (int i = 0; i < K; i++) fprintf(f, "%d ", WA[i][t]); fprintf(f, "| "); for (int i = 0; i < K; i++) fprintf(f, "%d ", WB[i][t]); fprintf(f, "\n"); }
            fclose(f);
        } else {
            int cb[MAXP], ca[MAXP], pd[MAXP]; uint64_t fl; unpack(KEYS[TERMS[k]], cb, ca, pd, &fl);
            for (int t = 0; t < P; t++) cbs[t] = '0' + cb[t]; cbs[P] = 0;
            printf("    terminal cin=%s (up to rotation by %d)\n", cbs, R0);
        }
    }
    fflush(stdout);
}

int main(int argc, char **argv) {
    int ai = 1, part_i = 0, part_n = 1, rep = 1, oddonly = 0; uint64_t minprint = 0;
    while (ai < argc && argv[ai][0] == '-') {
        if (!strcmp(argv[ai], "-w")) WANTW = 1;
        else if (!strcmp(argv[ai], "-nosym")) SYM = 0;
        else if (!strcmp(argv[ai], "-layers")) LAYERS = 1;
        else if (!strcmp(argv[ai], "-cap")) CAPLOG = atoi(argv[++ai]);
        else if (!strcmp(argv[ai], "-part")) sscanf(argv[++ai], "%d/%d", &part_i, &part_n);
        else if (!strcmp(argv[ai], "-min")) minprint = strtoull(argv[++ai], 0, 10);
        else if (!strcmp(argv[ai], "-rep")) rep = atoi(argv[++ai]);
        else if (!strcmp(argv[ai], "-odd")) oddonly = 1;
        else { fprintf(stderr, "unknown option %s\n", argv[ai]); return 2; }
        ai++;
    }
    if (argc - ai < 3) { fprintf(stderr, "usage: outauto2 [-w] [-nosym] [-cap N] [-layers] [-part i/n] [-min N] A|C B <ov|P> [d0]\n"); return 2; }
    if (argc - ai != (argv[ai][0] == 'C' ? 4 : 3)) { fprintf(stderr, "wrong number of arguments (options must come before A|C)\n"); return 2; }
    if (CAPLOG > 31) CAPLOG = 31;
    TYPEC = argv[ai][0] == 'C'; B = atoi(argv[ai + 1]);
    DBITS = 1; while ((1 << DBITS) < B) DBITS++;
    // 第3引数が「0/1 だけで長さ3以上」ならパターン、そうでなければ周期の数とみなす
    const char *pat = argv[ai + 2]; int single = strlen(pat) >= 3;
    for (const char *p = pat; *p; p++) if (*p != '0' && *p != '1') single = 0;
    P = single ? (int)strlen(pat) : atoi(pat);
    if (P < 1 || P > 32) { fprintf(stderr, "P must be 1..32\n"); return 2; }
    memset(NEEDBAD, 0, sizeof NEEDBAD); memset(NEEDNB, 0, sizeof NEEDNB);
    if (TYPEC) {
        if (argc - ai < 4) { fprintf(stderr, "type C needs d0\n"); return 2; }
        int d0 = atoi(argv[ai + 3]), d = d0;
        for (int t = 0; t <= P; t++) { Dg[t] = d; if (t < P) { Gg[t] = 2 * d >= B; d = 2 * d - (B - 1) * Gg[t]; } }
        if (Dg[P] != d0) { printf("type C, base %d, period %d: run digit %d does not return after %d steps\n", B, P, d0, P); return 0; }
        for (int t = 0; t < P; t++) { NEEDBAD[t] = 2 * Dg[t] <= B - 1; NEEDNB[t] = (2 * Dg[t] == 0 || 2 * Dg[t] == B + 1); }
        printf("type C, base %d, period %d, central run digits:", B, P); for (int t = 0; t < P; t++) printf(" %d", Dg[t]); printf("\n");
    } else printf("type A, base %d, period %d\n", B, P);
    clock_t t0 = clock();
    if (single) {
        for (int t = 0; t < P; t++) OV[t] = pat[t] - '0';
        run_pattern(); report(0);
        printf("single pattern: %s, %.1f s\n", cap_hit ? "INCONCLUSIVE (capacity)" : NTERMS ? "OUTER SOLUTION EXISTS" : "no outer solution of any length", (double)(clock() - t0) / CLOCKS_PER_SEC);
        return 0;
    }
    long npat = 0, nsol = 0, ninc = 0, idx = 0; uint64_t maxreach = 0, sumreach = 0;
    if (rep < 1 || P % rep) { fprintf(stderr, "-rep k: k must divide P\n"); return 2; }
    int h = P / rep;                                          // 語 w の長さ。w の回転の代表だけを調べる
    for (uint32_t mask = 1; mask < (1u << h); mask++) {
        int w = __builtin_popcount(mask) * rep;
        if (!TYPEC && (w % 2) != oddonly) continue;
        if (!(mask >> (h - 1) & 1)) continue;                 // 回転の代表: 最後のステップで桁あふれ
        int minrot = 1;
        for (int k = 1; k < h; k++) { uint32_t r = ((mask >> k) | (mask << (h - k))) & ((1u << h) - 1); if ((r >> (h - 1) & 1) && r < mask) { minrot = 0; break; } }
        if (!minrot) continue;
        if ((idx++ % part_n) != part_i) continue;
        for (int t = 0; t < P; t++) OV[t] = (mask >> (t % h)) & 1;
        npat++;
        run_pattern(); report(minprint);
        if (cap_hit) ninc++; else if (NTERMS) nsol++;
        if (NFULL > maxreach) maxreach = NFULL;
        sumreach += NFULL;
    }
    printf("summary: base=%d P=%d rep=%d part=%d/%d patterns=%ld with_outer_solution=%ld inconclusive=%ld maxreach=%llu sumreach=%llu time=%.1fs\n",
           B, P, rep, part_i, part_n, npat, nsol, ninc, (unsigned long long)maxreach, (unsigned long long)sumreach, (double)(clock() - t0) / CLOCKS_PER_SEC);
    return 0;
}
