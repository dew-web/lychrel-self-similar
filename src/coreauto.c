// coreauto.c — 中心コアを「中心から外側へ」1層ずつ組み立てる有限オートマトン。
// コアの長さを固定せずに、信号 (cin_t, ov_t) を反転し続ける周期的なコアが存在するかを完全に判定する。
//
// 桁あふれのたびに中心が半桁ずれる効果は、位相ごとの層のずらし O[t] で吸収する
//   （奇数長→偶数長の遷移で O が1増える。1周期で O は δ = Σov/2 増えるので、
//     位相P(=位相0)の層 l は位相0の層 l-δ と一致しなければならない → 直近 δ 層の位相0の対を状態に保持）
// 受理 = 全位相が「長い 0/ω 列の中」の状態（RUN 状態）に到達すること。
//
// usage: ./coreauto B ovbits cinbits [outerfile]
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
typedef unsigned __int128 u128;

static int B, P, CIN[64], OV[64], PAR[65], O[65], DELTA, PAR0;
typedef struct { int c; int rho[64], eta[64], prevlow[64]; int lag[8][2]; } st_t;

static u128 pack(const st_t *s) {
    u128 k = 0; int sh = 0;
    k |= (u128)s->c << sh; sh += 3;
    for (int t = 0; t < P; t++) k |= (u128)(s->rho[t] & 1) << sh++;
    for (int t = 0; t < P; t++) k |= (u128)(s->eta[t] & 1) << sh++;
    for (int t = 0; t < P; t++) if (OV[t]) { k |= (u128)(s->prevlow[t] & 31) << sh; sh += 5; }
    int nl = s->c < DELTA ? s->c : DELTA;
    for (int i = 0; i < DELTA; i++) {
        int a = i < nl ? s->lag[i][0] : 0, b = i < nl ? s->lag[i][1] : 0;
        k |= (u128)(a & 31) << sh; sh += 5; k |= (u128)(b & 31) << sh; sh += 5;
    }
    return k;
}
static void unpack(u128 k, st_t *s) {
    memset(s, 0, sizeof *s); int sh = 0;
    s->c = (int)(k >> sh & 7); sh += 3;
    for (int t = 0; t < P; t++) s->rho[t] = (int)(k >> sh++ & 1);
    for (int t = 0; t < P; t++) s->eta[t] = (int)(k >> sh++ & 1);
    for (int t = 0; t < P; t++) if (OV[t]) { s->prevlow[t] = (int)(k >> sh & 31); sh += 5; }
    for (int i = 0; i < DELTA; i++) { s->lag[i][0] = (int)(k >> sh & 31); sh += 5; s->lag[i][1] = (int)(k >> sh & 31); sh += 5; }
}

// ---- 状態表 ----
typedef struct { u128 key; int32_t parent; uint8_t lo0, hi0; } entry;
static entry *E; static int64_t NE, ECAP = 1 << 24;
static int32_t *H; static uint64_t HCAP = 1ull << 25;
static int64_t accept_idx = -1; static int cap_hit = 0; static u128 RUNKEY;
static int64_t insert(u128 key, int32_t parent, int lo0, int hi0) {
    uint64_t h = (uint64_t)((key * (u128)0x9E3779B97F4A7C15ull) >> 64) & (HCAP - 1);
    h ^= (uint64_t)(key >> 64) * 0x632BE59BD9B4E019ull & (HCAP - 1);
    while (H[h] >= 0) { if (E[H[h]].key == key) return -2; h = (h + 1) & (HCAP - 1); }
    if (NE >= ECAP) { cap_hit = 1; return -1; }
    H[h] = (int32_t)NE; E[NE].key = key; E[NE].parent = parent; E[NE].lo0 = (uint8_t)lo0; E[NE].hi0 = (uint8_t)hi0;
    return NE++;
}


// ---- 層ごとの BFS ----
static int LAYERED = 0; static u128 *LH[2]; static uint64_t LCAP = 1ull << 25; static int64_t LN[2]; static u128 *LA[2];
static int curL = 0, layer_found = 0, layer_cap = 0;
static const u128 EMPTYK = ~(u128)0;
static int lh_insert(int which, u128 key) {   // 1: 新規, 0: 既存, -1: 容量超過
    uint64_t h = (uint64_t)((key * (u128)0x9E3779B97F4A7C15ull) >> 64) & (LCAP - 1);
    h ^= (uint64_t)(key >> 64) * 0x632BE59BD9B4E019ull & (LCAP - 1);
    while (LH[which][h] != EMPTYK) { if (LH[which][h] == key) return 0; h = (h + 1) & (LCAP - 1); }
    if (LN[which] >= (int64_t)(LCAP / 2)) return -1;
    LH[which][h] = key; LA[which][LN[which]++] = key; return 1;
}
static int lh_has(int which, u128 key) {
    uint64_t h = (uint64_t)((key * (u128)0x9E3779B97F4A7C15ull) >> 64) & (LCAP - 1);
    h ^= (uint64_t)(key >> 64) * 0x632BE59BD9B4E019ull & (LCAP - 1);
    while (LH[which][h] != EMPTYK) { if (LH[which][h] == key) return 1; h = (h + 1) & (LCAP - 1); }
    return 0;
}

// ---- 1層ぶんの遷移 ----
static st_t S, N; static int64_t curidx;
static int in_lo[66], in_hi[66], in_mid[66], lowout[66], highout[66], midout[66], ch_lo0, ch_hi0;

static void derive_input(int t) {          // 位相 t の入力（位相 t-1 の出力から）
    int tp = t - 1, center = (S.c == O[t]);
    if (OV[tp] == 0) {
        if (center && PAR[t] == 0) in_mid[t] = midout[tp];
        else { in_lo[t] = lowout[tp]; in_hi[t] = highout[tp]; }
    } else if (PAR[tp] == 0) {               // 奇数長 -> 偶数長
        in_lo[t] = S.prevlow[tp]; in_hi[t] = highout[tp];
    } else {                                 // 偶数長 -> 奇数長
        if (center) in_mid[t] = highout[tp];
        else { in_lo[t] = S.prevlow[tp]; in_hi[t] = highout[tp]; }
    }
}
static void finalize(void) {
    if (S.c >= DELTA) {                      // 位相P（=位相0）の対が位相0の δ 層前の対と一致するか
        derive_input(P);
        int center = (S.c == DELTA);
        if (center && PAR[0] == 0) { if (S.lag[0][1] != 31 || S.lag[0][0] != in_mid[P]) return; }
        else { if (S.lag[0][1] == 31 || S.lag[0][0] != in_lo[P] || S.lag[0][1] != in_hi[P]) return; }
    }
    int nl = S.c < DELTA ? S.c : DELTA;
    if (S.c < DELTA) { for (int i = 0; i < nl; i++) { N.lag[i][0] = S.lag[i][0]; N.lag[i][1] = S.lag[i][1]; } N.lag[nl][0] = ch_lo0; N.lag[nl][1] = ch_hi0; }
    else { for (int i = 0; i + 1 < DELTA; i++) { N.lag[i][0] = S.lag[i + 1][0]; N.lag[i][1] = S.lag[i + 1][1]; } N.lag[DELTA - 1][0] = ch_lo0; N.lag[DELTA - 1][1] = ch_hi0; }
    N.c = S.c + 1 > DELTA + 1 ? DELTA + 1 : S.c + 1;
    u128 key = pack(&N);
    if (LAYERED) { int r = lh_insert(1 - curL, key); if (r < 0) layer_cap = 1; if (key == RUNKEY) layer_found = 1; return; }
    int64_t id = insert(key, (int32_t)curidx, ch_lo0, ch_hi0);
    if (id >= 0 && key == RUNKEY && accept_idx < 0) accept_idx = id;
}
static void dfs(int t) {
    if (cap_hit || accept_idx >= 0 || layer_cap) return;
    if (t == P || S.c < O[t]) { finalize(); return; }   // 未開始の位相が残るなら位相Pも未開始
    if (t > 0) derive_input(t);
    int center = (S.c == O[t]);
    if (center && PAR[t] == 0) {
        int m = in_mid[t];
        for (int g = 0; g <= 1; g++) {
            int v = 2 * m + g; midout[t] = v % B; N.eta[t] = v >= B; N.rho[t] = g;
            if (OV[t]) N.prevlow[t] = midout[t];
            dfs(t + 1);
        }
    } else {
        int s = in_lo[t] + in_hi[t];
        for (int g = 0; g <= 1; g++) {
            if (center) {
                int v = s + g; lowout[t] = v % B; int cl = v >= B; int w = s + cl;
                highout[t] = w % B; N.eta[t] = w >= B; N.rho[t] = g;
            } else {
                if (((s + g) >= B) != S.rho[t]) continue;
                lowout[t] = (s + g) % B; N.rho[t] = g;
                int w = s + S.eta[t]; highout[t] = w % B; N.eta[t] = w >= B;
            }
            if (OV[t]) N.prevlow[t] = lowout[t];
            dfs(t + 1);
        }
    }
}
static void expand_key(u128 key) {
    unpack(key, &S); curidx = -1;
    if (S.c == 0 && PAR[0] == 0) {
        for (int m = 0; m < B; m++) { memset(&N, 0, sizeof N); in_mid[0] = m; ch_lo0 = m; ch_hi0 = 31; dfs(0); }
    } else {
        for (int lo = 0; lo < B; lo++) for (int hi = 0; hi < B; hi++) { memset(&N, 0, sizeof N); in_lo[0] = lo; in_hi[0] = hi; ch_lo0 = lo; ch_hi0 = hi; dfs(0); }
    }
}
static void expand(int64_t idx) {
    unpack(E[idx].key, &S); curidx = idx;
    if (S.c == 0 && PAR[0] == 0) {
        for (int m = 0; m < B; m++) { memset(&N, 0, sizeof N); in_mid[0] = m; ch_lo0 = m; ch_hi0 = 31; dfs(0); }
    } else {
        for (int lo = 0; lo < B; lo++) for (int hi = 0; hi < B; hi++) {
            memset(&N, 0, sizeof N); in_lo[0] = lo; in_hi[0] = hi; ch_lo0 = lo; ch_hi0 = hi; dfs(0);
        }
    }
}

// ---- 独立な検証: 窓を直接シミュレーション ----
static int window_period_ok(const int *w0, int n0) {
    int w[8192], n = n0, pad = 40, D;
    int L0 = (B - 1) * (1 - CIN[P - 1]), H0 = (B - 1) * CIN[P - 1];
    // 両側に列の数字を pad 個ずつ付ける（中心は保つ）
    D = 0; for (int i = 0; i < pad; i++) w[D++] = L0; for (int i = 0; i < n; i++) w[D++] = w0[i]; for (int i = 0; i < pad; i++) w[D++] = H0;
    int start[8192], Ds = D; memcpy(start, w, sizeof(int) * D);
    for (int t = 0; t < P; t++) {
        int y[8192], c = CIN[t];
        for (int j = 0; j < D; j++) { int v = w[j] + w[D - 1 - j] + c; c = v >= B; y[j] = v - (c ? B : 0); }
        if (c != 1 - CIN[t]) return 0;
        int Lp = (B - 1) * (1 - CIN[t]), Hp = (B - 1) * CIN[t];
        if (!OV[t]) { memcpy(w, y, sizeof(int) * D); }
        else if (D % 2 == 1) { memcpy(w, y, sizeof(int) * D); w[D++] = Hp; }   // 奇数長 -> 上に1桁
        else { if (y[0] != Lp) return 0; memcpy(w, y + 1, sizeof(int) * (D - 1)); D--; }
        // 窓を長さ Ds 付近に保つため、両端の列の数字を対で調整（対称性は保つ）
        while (D > Ds + 1 && w[0] == Lp && w[D - 1] == Hp) { memmove(w, w + 1, sizeof(int) * (D - 2)); D -= 2; }
        while (D < Ds - 1) { memmove(w + 1, w, sizeof(int) * D); w[0] = Lp; w[D + 1] = Hp; D += 2; }
    }
    if (D != Ds) return 0;
    return memcmp(w, start, sizeof(int) * D) == 0;
}

// ---- 実際の数を作って直接シミュレーション ----
static int rev_add(int *x, int D) {
    static int t[1 << 16]; int c = 0;
    for (int i = 0; i < D; i++) t[i] = x[i] + x[D - 1 - i];
    for (int i = 0; i < D; i++) { int v = t[i] + c; c = v >= B; x[i] = v - (c ? B : 0); }
    if (c) x[D++] = 1;
    return D;
}
static int is_pal(const int *x, int D) { for (int i = 0, j = D - 1; i < j; i++, j--) if (x[i] != x[j]) return 0; return 1; }
static int OA[256], OBw[256], OK_; 
static int build(int *x, const int *cw, int cl, int n) {
    int D = 0, L0 = (B - 1) * (1 - CIN[P - 1]), H0 = (B - 1) * CIN[P - 1];
    for (int i = 0; i < OK_; i++) x[D++] = OBw[i];
    for (int i = 0; i < n; i++) x[D++] = L0;
    for (int i = 0; i < cl; i++) x[D++] = cw[i];
    for (int i = 0; i < n; i++) x[D++] = H0;
    for (int i = OK_ - 1; i >= 0; i--) x[D++] = OA[i];
    return D;
}

int main(int argc, char **argv) {
    B = atoi(argv[1]); P = (int)strlen(argv[2]);
    for (int t = 0; t < P; t++) { OV[t] = argv[2][t] - '0'; CIN[t] = argv[3][t] - '0'; }
    int Lmode = (argc > 4 && argv[4][0] == 'L'); int Lmax = Lmode ? atoi(argv[5]) : 0;
    if (Lmode) { ECAP = 4; HCAP = 4; LAYERED = 0; }
    E = malloc(sizeof(entry) * ECAP); H = malloc(sizeof(int32_t) * HCAP);
    if (Lmode) for (int w2 = 0; w2 < 2; w2++) { LH[w2] = malloc(sizeof(u128) * LCAP); LA[w2] = malloc(sizeof(u128) * (LCAP / 2)); }
    int nov = 0; for (int t = 0; t < P; t++) nov += OV[t];
    if (nov % 2) { printf("odd number of overflows\n"); return 0; }
    for (PAR0 = 0; PAR0 <= 1; PAR0++) {
        PAR[0] = PAR0; O[0] = 0;
        for (int t = 0; t < P; t++) { PAR[t + 1] = PAR[t] ^ OV[t]; O[t + 1] = O[t] + (OV[t] && PAR[t] == 0); }
        DELTA = O[P];
        // RUN 状態
        st_t R; memset(&R, 0, sizeof R); R.c = DELTA + 1;
        int L0 = (B - 1) * (1 - CIN[P - 1]), H0 = (B - 1) * CIN[P - 1];
        for (int t = 0; t < P; t++) { R.rho[t] = CIN[t]; R.eta[t] = 1 - CIN[t]; if (OV[t]) R.prevlow[t] = (B - 1) * (1 - CIN[t]); }
        for (int i = 0; i < DELTA; i++) { R.lag[i][0] = L0; R.lag[i][1] = H0; }
        RUNKEY = pack(&R);
        // 自己検査: RUN 状態は (L0,H0) を選べば RUN 状態に戻るはず
        for (uint64_t i = 0; i < HCAP; i++) H[i] = -1;
        NE = 0; accept_idx = -1; cap_hit = 0;
        insert(RUNKEY, -1, 0, 0);
        unpack(RUNKEY, &S); curidx = 0; memset(&N, 0, sizeof N); in_lo[0] = L0; in_hi[0] = H0; ch_lo0 = L0; ch_hi0 = H0;
        accept_idx = -1; dfs(0);
        int selfloop = (NE == 1);   // 新しい状態が増えなければ RUN -> RUN
        if (Lmode) {
            LAYERED = 1; curL = 0; layer_found = 0; layer_cap = 0;
            for (int w2 = 0; w2 < 2; w2++) { for (uint64_t i = 0; i < LCAP; i++) LH[w2][i] = EMPTYK; LN[w2] = 0; }
            st_t I0; memset(&I0, 0, sizeof I0); lh_insert(0, pack(&I0));
            int fix = 0;
            for (int L = 0; L < Lmax && !layer_found && !layer_cap; L++) {
                int nx = 1 - curL;
                for (uint64_t i = 0; i < LCAP; i++) LH[nx][i] = EMPTYK; LN[nx] = 0;
                for (int64_t i = 0; i < LN[curL] && !layer_cap; i++) expand_key(LA[curL][i]);
                int same = (LN[nx] == LN[curL]);
                for (int64_t i = 0; same && i < LN[nx]; i++) if (!lh_has(curL, LA[nx][i])) same = 0;
                printf("  par0=%d layer %2d -> %lld states%s\n", PAR0, L + 1, (long long)LN[nx], same ? " (same set as previous layer: fixpoint)" : "");
                fflush(stdout);
                curL = nx;
                if (same) { fix = 1; break; }
            }
            printf("B=%d P=%d par0=%d delta=%d: %s\n", B, P, PAR0, DELTA,
                   layer_found ? "RUN STATE REACHED (core exists)" : layer_cap ? "layer capacity exceeded" :
                   fix ? "fixpoint without RUN => no periodic core of ANY length" : "no core up to the depth limit");
            fflush(stdout);
            LAYERED = 0;
            continue;
        }
        // 本探索
        for (uint64_t i = 0; i < HCAP; i++) H[i] = -1;
        NE = 0; accept_idx = -1; cap_hit = 0;
        st_t I; memset(&I, 0, sizeof I);
        insert(pack(&I), -1, 0, 0);
        for (int64_t q = 0; q < NE && accept_idx < 0 && !cap_hit; q++) expand(q);
        printf("B=%d P=%d par0=%d delta=%d  RUN self-loop check=%s  reachable states=%lld  ",
               B, P, PAR0, DELTA, selfloop ? "ok" : "NG", (long long)NE);
        if (cap_hit) { printf("CAPACITY EXCEEDED (inconclusive)\n"); continue; }
        if (accept_idx < 0) { printf("=> no periodic core of ANY length\n"); continue; }
        // 復元
        int lay[8192][2], Q = 0; int64_t v = accept_idx;
        while (E[v].parent >= 0) { lay[Q][0] = E[v].lo0; lay[Q][1] = E[v].hi0; Q++; v = E[v].parent; }
        for (int i = 0; i < Q / 2; i++) { int a0 = lay[i][0], a1 = lay[i][1]; lay[i][0] = lay[Q - 1 - i][0]; lay[i][1] = lay[Q - 1 - i][1]; lay[Q - 1 - i][0] = a0; lay[Q - 1 - i][1] = a1; }
        int w[8192], n;
        if (PAR0 == 0) { n = 2 * Q - 1; int c0 = Q - 1; w[c0] = lay[0][0]; for (int q = 1; q < Q; q++) { w[c0 - q] = lay[q][0]; w[c0 + q] = lay[q][1]; } }
        else { n = 2 * Q; int c0 = Q - 1; for (int q = 0; q < Q; q++) { w[c0 - q] = lay[q][0]; w[c0 + 1 + q] = lay[q][1]; } }
        printf("=> CORE FOUND (layers=%d, window %d digits)\n   core (low->high): ", Q, n);
        for (int j = 0; j < n; j++) printf("%d", w[j]); printf("\n");
        printf("   independent window simulation over one period: %s\n", window_period_ok(w, n) ? "PERIODIC (ok)" : "FAILED");
        if (argc > 4) {                      // 外側の窓と組み合わせて実際の数で検証
            FILE *f = fopen(argv[4], "r"); int bb, pp; char ovs[64], cbs[64];
            if (fscanf(f, "%d %d %d %s %s", &bb, &pp, &OK_, ovs, cbs) != 5) return 1;
            for (int i = 0; i < OK_; i++) if (fscanf(f, "%d", &OA[i]) != 1) return 1;
            char bar[4]; if (fscanf(f, "%3s", bar) != 1) return 1;
            for (int i = 0; i < OK_; i++) if (fscanf(f, "%d", &OBw[i]) != 1) return 1;
            fclose(f);
            static int x[1 << 16], xe[1 << 16]; int nn = 20;
            int D = build(x, w, n, nn), ok = 1;
            printf("   FULL NUMBER x0 (%d digits): ", D); for (int i = D - 1; i >= 0; i--) putchar('0' + x[i]); printf("\n");
            int delta = nov / 2;
            for (int rep = 1; rep <= 6 && ok; rep++) {
                for (int t = 0; t < P; t++) { D = rev_add(x, D); if (is_pal(x, D)) { ok = 0; break; } }
                int De = build(xe, w, n, nn + rep * delta);
                if (De != D || memcmp(x, xe, sizeof(int) * D)) ok = 0;
            }
            printf("   direct reverse-and-add simulation, 6 periods: %s\n", ok ? "SELF-SIMILAR, NO PALINDROME (VERIFIED)" : "FAILED");
        }
    }
    return 0;
}
