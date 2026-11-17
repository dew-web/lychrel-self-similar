// outauto2p.c — outauto2.c の並列版（OpenMP）。1パターンの判定専用（窓の復元はしない）。
//
// 探索の論理（状態、遷移、受理条件、回転対称性による商）は outauto2.c と同じ。違いは次の点だけ:
//   - BFS を段ごとに並列化する。1つの段の状態を複数スレッドで展開し、新しい状態は共有の表に追加する。
//   - 表は最初に上限ぶん確保する。追加は compare-and-swap で行う。状態の番号は 64 ビットなので、2^32 個を超えてもよい。
//     メモリは 1 状態あたり 26 バイト（キー 16 ＋ ハッシュ表 上限の 1.25 倍 × 8）。
//     例: -capm 6900（69億状態）で約 180GB、-capm 28000 で約 730GB。
//     2つのスレッドが同じ新しい状態を同時に追加しようとしたときは、片方の場所を「無効」にする（キーを全ビット1にする）。
//   - 型A の探索中に、型C の受理条件（旗を除く）に合う状態も数える。0 個なら、このパターンに型C の解もない。
// 到達状態数（対称性なしで数えた値）は outauto2 / outauto と一致するはず（検証済みの値と比べること）。
//
// usage: outauto2p [-cap N | -capm 百万単位] [-nosym] [-t スレッド数] A B <ov文字列>
//        outauto2p [-cap N | -capm 百万単位] [-nosym] [-t スレッド数] C B <ov文字列> d0
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <omp.h>
typedef unsigned __int128 u128;

#define MAXP 40
static int B, P, OV[MAXP], NOV, TYPEC, DBITS;
static int Dg[MAXP + 1], Gg[MAXP + 1], NEEDBAD[MAXP], NEEDNB[MAXP], FB[MAXP], FN[MAXP], NF;
static int R0, NROT = 1, MPD, MFL, SYM = 1, CAPLOG = 28;

static u128 *KEYS; static uint64_t *SLOT;
#define DEADKEY (~(u128)0)
static uint64_t NK, KCAP, HSIZE, NDEAD; static volatile int cap_hit;

static inline uint64_t hash128(u128 k) {
    uint64_t lo = (uint64_t)k, hi = (uint64_t)(k >> 64);
    uint64_t x = lo * 0x9E3779B97F4A7C15ull ^ (hi + 0x632BE59BD9B4E019ull) * 0xC2B2AE3D27D4EB4Full;
    x ^= x >> 29; x *= 0xBF58476D1CE4E5B9ull; x ^= x >> 32;
    return x;
}
// 戻り値: 1 = 新規に追加した, 0 = 既にあった, -1 = 上限超過
static int insert(u128 key, int orb) {
    uint64_t h = (uint64_t)(((u128)hash128(key) * HSIZE) >> 64); int64_t mine = -1;   // 0..HSIZE-1 に写す（HSIZE は 2 のべきでなくてよい）
    for (;;) {
        uint64_t s = __atomic_load_n(&SLOT[h], __ATOMIC_ACQUIRE);
        if (s == 0) {
            if (mine < 0) {
                mine = (int64_t)__atomic_fetch_add(&NK, 1, __ATOMIC_RELAXED);
                if ((uint64_t)mine >= KCAP) { cap_hit = 1; return -1; }
                KEYS[mine] = key;
            }
            uint64_t expect = 0;
            if (__atomic_compare_exchange_n(&SLOT[h], &expect, (uint64_t)(mine + 1), 0, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) return 1;
            s = expect;                                    // 他のスレッドが先にこの場所を取った
        }
        if (KEYS[s - 1] == key) { if (mine >= 0) { KEYS[mine] = DEADKEY; __atomic_fetch_add(&NDEAD, 1, __ATOMIC_RELAXED); } return 0; }   // 同じ状態が既にある（自分の場所は無効にする）
        if (++h == HSIZE) h = 0;
    }
}

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
static u128 rotkey(u128 k, int j) {
    int wpd = NOV * DBITS;
    u128 cb = k & maskw(P), ca = (k >> P) & maskw(P), pd = (k >> (2 * P)) & maskw(wpd), fl = NF ? (k >> (2 * P + wpd)) & maskw(NF) : 0;
    cb = rotr(cb, P, j * R0); ca = rotr(ca, P, j * R0); pd = rotr(pd, wpd, j * MPD * DBITS); fl = rotr(fl, NF, j * MFL);
    u128 r = cb | (ca << P) | (pd << (2 * P));
    if (NF) r |= fl << (2 * P + wpd);
    return r;
}

// スレッドごとの作業領域
typedef struct {
    int cur_cb[MAXP], cur_ca[MAXP], cur_pd[MAXP], cur_root; uint64_t cur_flags;
    int n_cb[MAXP], n_ca[MAXP], n_pd[MAXP];
    uint64_t nfull, nterm, nccand;
} work_t;
#define MAXTERM 4096
static u128 TERMK[MAXTERM]; static int NTERMS;

static int is_terminal(const int *cb, const int *ca, const int *pd, uint64_t flags) {
    if (!TYPEC) {
        for (int t = 0; t < P; t++) { if (ca[t] != 1 - cb[t]) return 0; if (OV[t] && pd[t] != (B - 1) * cb[t]) return 0; }
        return 1;
    }
    for (int t = 0; t < P; t++) { if (cb[t] != Gg[t] || ca[t] != Gg[t]) return 0; if (OV[t] && pd[t] != Dg[t + 1]) return 0; }
    return flags == (NF >= 64 ? ~0ull : ((1ull << NF) - 1));
}
// 型A の探索中に数える「型C の候補」: 中央列の数字の周期 (Dg, Gg) のどれかについて、旗を除く型C の受理条件に合う状態
static int NCC, CCD[64][MAXP + 1], CCG[64][MAXP];
static int is_ccand(const int *cb, const int *ca, const int *pd) {
    for (int i = 0; i < NCC; i++) {
        int ok = 1;
        for (int t = 0; t < P && ok; t++) { if (cb[t] != CCG[i][t] || ca[t] != CCG[i][t]) ok = 0; else if (OV[t] && pd[t] != CCD[i][t + 1]) ok = 0; }
        if (ok) return 1;
    }
    return 0;
}
static void emit(work_t *W, uint64_t flags) {
    u128 k = pack(W->n_cb, W->n_ca, W->n_pd, flags), kc = k; int orb = 1;
    if (NROT > 1) {
        u128 seen[MAXP]; seen[0] = k;
        for (int j = 1; j < NROT; j++) {
            u128 kr = rotkey(k, j); int dup = 0;
            for (int i = 0; i < orb; i++) if (seen[i] == kr) { dup = 1; break; }
            if (!dup) seen[orb++] = kr;
            if (kr < kc) kc = kr;
        }
    }
    int r = insert(kc, orb);
    if (r <= 0) return;
    W->nfull += orb;
    if (NCC && is_ccand(W->n_cb, W->n_ca, W->n_pd)) W->nccand += orb;
    if (is_terminal(W->n_cb, W->n_ca, W->n_pd, flags)) {
        W->nterm += orb;
        #pragma omp critical(term)
        { if (NTERMS < MAXTERM) TERMK[NTERMS++] = kc; }
    }
}
static void dfs(work_t *W, int t, int a, int b, uint64_t flags, int a0, int b0) {
    if (t == P) { if (a == a0 && b == b0) emit(W, flags); return; }
    if (W->cur_root && a == 0) return;
    int s = a + b, v = s + W->cur_cb[t], cbn = v >= B, nb = v - (cbn ? B : 0);
    W->n_cb[t] = cbn;
    uint64_t f2 = flags;
    if (NEEDBAD[t] && s >= B) f2 |= 1ull << FB[t];
    if (NEEDNB[t] && s != 0 && s != B + 1) f2 |= 1ull << FN[t];
    for (int c = 0; c <= 1; c++) {
        int w = s + c, co = w >= B;
        if (co != W->cur_ca[t]) continue;
        int ya = w - (co ? B : 0), an;
        W->n_ca[t] = c;
        if (OV[t]) { an = W->cur_pd[t]; W->n_pd[t] = ya; } else { an = ya; W->n_pd[t] = 0; }
        dfs(W, t + 1, an, nb, f2, a0, b0);
    }
}
static void expand(work_t *W) { for (int a0 = 0; a0 < B; a0++) for (int b0 = 0; b0 < B; b0++) dfs(W, 0, a0, b0, W->cur_flags, a0, b0); }

int main(int argc, char **argv) {
    int ai = 1, nth = 0; uint64_t capm = 0;
    while (ai < argc && argv[ai][0] == '-') {
        if (!strcmp(argv[ai], "-nosym")) SYM = 0;
        else if (!strcmp(argv[ai], "-cap")) CAPLOG = atoi(argv[++ai]);
        else if (!strcmp(argv[ai], "-capm")) capm = strtoull(argv[++ai], 0, 10);
        else if (!strcmp(argv[ai], "-t")) nth = atoi(argv[++ai]);
        else { fprintf(stderr, "unknown option %s\n", argv[ai]); return 2; }
        ai++;
    }
    if (argc - ai < 3 || argc - ai != (argv[ai][0] == 'C' ? 4 : 3)) { fprintf(stderr, "usage: outauto2p [-cap N] [-nosym] [-t n] A|C B <ov> [d0]\n"); return 2; }
    if (nth > 0) omp_set_num_threads(nth);
    TYPEC = argv[ai][0] == 'C'; B = atoi(argv[ai + 1]);
    DBITS = 1; while ((1 << DBITS) < B) DBITS++;
    const char *pat = argv[ai + 2]; P = (int)strlen(pat);
    if (P < 1 || P > 32) { fprintf(stderr, "P must be 1..32\n"); return 2; }
    for (int t = 0; t < P; t++) OV[t] = pat[t] - '0';
    if (TYPEC) {
        int d0 = atoi(argv[ai + 3]), d = d0;
        for (int t = 0; t <= P; t++) { Dg[t] = d; if (t < P) { Gg[t] = 2 * d >= B; d = 2 * d - (B - 1) * Gg[t]; } }
        if (Dg[P] != d0) { printf("run digit %d does not return after %d steps\n", d0, P); return 0; }
        for (int t = 0; t < P; t++) { NEEDBAD[t] = 2 * Dg[t] <= B - 1; NEEDNB[t] = (2 * Dg[t] == 0 || 2 * Dg[t] == B + 1); }
    }
    NOV = 0; for (int t = 0; t < P; t++) NOV += OV[t];
    NF = 0; for (int t = 0; t < P; t++) { if (NEEDBAD[t]) FB[t] = NF++; if (NEEDNB[t]) FN[t] = NF++; }
    if (2 * P + NOV * DBITS + NF > 128 || NF > 63) { fprintf(stderr, "key too long\n"); return 2; }
    R0 = P;
    if (SYM) for (int r = 1; r < P; r++) {
        if (P % r) continue;
        int ok = 1;
        for (int t = 0; t < P && ok; t++) { if (OV[t] != OV[(t + r) % P]) ok = 0; if (TYPEC && Dg[t] != Dg[(t + r) % P]) ok = 0; }
        if (ok) { R0 = r; break; }
    }
    NROT = P / R0; MPD = 0; MFL = 0;
    for (int t = 0; t < R0; t++) { MPD += OV[t]; MFL += NEEDBAD[t] + NEEDNB[t]; }

    if (CAPLOG > 40) CAPLOG = 40;
    KCAP = capm ? capm * 1000000ull : (1ull << CAPLOG);
    HSIZE = KCAP + KCAP / 4 + 64;                              // ハッシュ表は上限の 1.25 倍（埋まり具合は最大 0.8）
    KEYS = malloc(sizeof(u128) * KCAP); SLOT = calloc(HSIZE, sizeof(uint64_t));
    if (!KEYS || !SLOT) { fprintf(stderr, "out of memory\n"); return 2; }
    if (!TYPEC) for (int d0 = 1; d0 < B - 1 && NCC < 64; d0++) {    // 型C の候補の表（0 と B-1 は周期的になれない）
        int d = d0;
        for (int t = 0; t <= P; t++) { CCD[NCC][t] = d; if (t < P) { CCG[NCC][t] = 2 * d >= B; d = 2 * d - (B - 1) * CCG[NCC][t]; } }
        if (CCD[NCC][P] == d0) NCC++;
    }
    double t0 = omp_get_wtime();
    fprintf(stderr, "type %c base %d ov=%s sym=%d threads=%d cap=%llu states (%.0f GB)\n", TYPEC ? 'C' : 'A', B, pat, NROT, omp_get_max_threads(), (unsigned long long)KCAP, (16.0 * KCAP + 8.0 * HSIZE) / 1e9);

    uint64_t nfull = 1, nterm = 0, nccand = 0;
    { work_t W; memset(&W, 0, sizeof W); for (int t = 0; t < P; t++) { W.cur_cb[t] = 0; W.cur_ca[t] = OV[t]; W.cur_pd[t] = OV[t] ? 1 : 0; } W.cur_root = 1; expand(&W); nfull += W.nfull; nterm += W.nterm; nccand += W.nccand; }
    uint64_t lstart = 0, lend = NK; int depth = 1;
    while (lstart < lend && !cap_hit) {
        #pragma omp parallel reduction(+:nfull,nterm,nccand)
        {
            work_t W; memset(&W, 0, sizeof W);
            #pragma omp for schedule(dynamic, 2048)
            for (uint64_t q = lstart; q < lend; q++) {
                if (cap_hit || KEYS[q] == DEADKEY) continue;
                unpack(KEYS[q], W.cur_cb, W.cur_ca, W.cur_pd, &W.cur_flags); W.cur_root = 0;
                expand(&W);
            }
            nfull += W.nfull; nterm += W.nterm; nccand += W.nccand;
        }
        fprintf(stderr, "layer %d size %llu total %llu  %.0fs\n", depth, (unsigned long long)(lend - lstart), (unsigned long long)lend + 1, omp_get_wtime() - t0); fflush(stderr);
        depth++; lstart = lend; lend = NK < KCAP ? NK : KCAP;
    }
    uint64_t nk = NK < KCAP ? NK : KCAP, ncanon = nk - NDEAD;
    printf("  ov=%s  reachable=%llu canon=%llu sym=%d depth=%d terminals=%llu%s\n", pat, (unsigned long long)nfull, (unsigned long long)ncanon + 1, NROT, depth, (unsigned long long)nterm, cap_hit ? " (CAPACITY)" : "");
    for (int k = 0; k < NTERMS; k++) {
        int cb[MAXP], ca[MAXP], pd[MAXP]; uint64_t fl; char cbs[MAXP + 1]; unpack(TERMK[k], cb, ca, pd, &fl);
        for (int t = 0; t < P; t++) cbs[t] = '0' + cb[t]; cbs[P] = 0;
        printf("    terminal cin=%s (up to rotation by %d)\n", cbs, R0);
    }
    if (!TYPEC) printf("    type-C candidates among reachable states (flags ignored, %d digit cycles): %llu%s\n", NCC, (unsigned long long)nccand, nccand ? "" : "  => no type-C solution for this pattern either");
    printf("single pattern: %s, %.1f s wall, %d threads\n", cap_hit ? "INCONCLUSIVE (capacity)" : NTERMS ? "OUTER SOLUTION EXISTS" : "no outer solution of any length", omp_get_wtime() - t0, omp_get_max_threads());
    return 0;
}
