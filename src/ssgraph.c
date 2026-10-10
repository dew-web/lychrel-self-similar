// ssgraph.c — 自己相似な逆順加算サイクルの「窓」力学を列和グラフとして全探索する
//
//  mode core  B r        : 型Aの中心コア（半径 r、長さ 2r+1/2r+2）。入力 (cin, ov) を自由にした
//                          非決定グラフを作り、強連結成分（SCC）を求める。周期解はすべて SCC に含まれる。
//  mode outerA B K       : 型Aの端の窓（各 K 桁）。上端の入力 = 1 - 下端の出力。決定的な関数グラフのサイクル列挙。
//  mode outerC B K       : 型Cの端の窓。中央の一様列の数字 d は d -> 2d-(B-1)[2d>=B] で巡回。
//
// 状態は「直前の窓の列和」で表す（周期軌道上の状態は必ず何かの像なので、像だけ数えれば十分）。
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static int B, R, K;
static int64_t N0, N1, NN;   // core: par0 ノード数, par1 ノード数
static int M;                // 2B-1

// ---------------- core ----------------
static inline int core_len(int par) { return par == 0 ? 2 * R + 1 : 2 * R + 2; }

static void core_decode(int64_t id, int *par, int *S, int *n) {
    if (id < N0) {
        *par = 0; *n = 2 * R + 1;
        int cd = (int)(id % B); id /= B;
        for (int j = 0; j < R; j++) { int s = (int)(id % M); id /= M; S[j] = s; S[*n - 1 - j] = s; }
        S[R] = 2 * cd;
    } else {
        id -= N0; *par = 1; *n = 2 * R + 2;
        for (int j = 0; j <= R; j++) { int s = (int)(id % M); id /= M; S[j] = s; S[*n - 1 - j] = s; }
    }
}

static int64_t core_encode_window(const int *w, int n) {
    int64_t id = 0, mul = 1;
    if (n == 2 * R + 1) {
        id = w[R]; mul = B;
        for (int j = 0; j < R; j++) { id += mul * (w[j] + w[n - 1 - j]); mul *= M; }
        return id;
    } else {
        for (int j = 0; j <= R; j++) { id += mul * (w[j] + w[n - 1 - j]); mul *= M; }
        return N0 + id;
    }
}

static int core_wout[512], core_wlen;   // 直近の core_step が作った窓の数字（下位から）
// 入力 in = cin + 2*ov。戻り値: 次ノード（不正なら -1）
static int64_t core_step(int64_t id, int in) {
    int cin = in & 1, ov = in >> 1, par, n, S[512], y[512], w[512];
    core_decode(id, &par, S, &n);
    int c = cin;
    for (int j = 0; j < n; j++) { int v = S[j] + c; c = v >= B; y[j] = v - (c ? B : 0); }
    if (c != 1 - cin) return -1;                       // コアは繰り上がりを反転する
    if (!ov) { memcpy(core_wout, y, sizeof(int) * n); core_wlen = n; return core_encode_window(y, n); }
    if (par == 0) {                                     // 奇数長 -> 偶数長（上に高い列の数字）
        memcpy(w, y, sizeof(int) * n); w[n] = (B - 1) * cin;
        memcpy(core_wout, w, sizeof(int) * (n + 1)); core_wlen = n + 1;
        return core_encode_window(w, n + 1);
    }
    if (y[0] != (B - 1) * (1 - cin)) return -1;         // 偶数長 -> 奇数長（下端は低い列の数字のはず）
    memcpy(core_wout, y + 1, sizeof(int) * (n - 1)); core_wlen = n - 1;
    return core_encode_window(y + 1, n - 1);
}

// 反復版 Tarjan
static int32_t *succ;  // NN*4
static void core_scc(void) {
    int32_t *idx = malloc(sizeof(int32_t) * NN), *low = malloc(sizeof(int32_t) * NN), *comp = malloc(sizeof(int32_t) * NN);
    int32_t *stk = malloc(sizeof(int32_t) * NN), *cs = malloc(sizeof(int32_t) * NN);
    uint8_t *onst = calloc(NN, 1), *ei = malloc(NN);
    for (int64_t i = 0; i < NN; i++) idx[i] = -1;
    int32_t counter = 0, sp = 0, ncomp = 0;
    for (int64_t s0 = 0; s0 < NN; s0++) {
        if (idx[s0] >= 0) continue;
        int32_t csp = 0; cs[csp++] = (int32_t)s0; idx[s0] = low[s0] = counter++; stk[sp++] = (int32_t)s0; onst[s0] = 1; ei[s0] = 0;
        while (csp) {
            int32_t v = cs[csp - 1];
            if (ei[v] < 4) {
                int32_t u = succ[(int64_t)v * 4 + ei[v]]; ei[v]++;
                if (u < 0) continue;
                if (idx[u] < 0) { idx[u] = low[u] = counter++; stk[sp++] = u; onst[u] = 1; ei[u] = 0; cs[csp++] = u; }
                else if (onst[u] && idx[u] < low[v]) low[v] = idx[u];
            } else {
                csp--;
                if (csp) { int32_t p = cs[csp - 1]; if (low[v] < low[p]) low[p] = low[v]; }
                if (low[v] == idx[v]) {
                    int32_t u;
                    do { u = stk[--sp]; onst[u] = 0; comp[u] = ncomp; } while (u != v);
                    ncomp++;
                }
            }
        }
    }
    // 非自明な SCC（サイズ>1 か自己ループ）で、内部に ov=1 の辺を含むもの
    int64_t *csize = calloc(ncomp, sizeof(int64_t)); uint8_t *hasov = calloc(ncomp, 1), *hasedge = calloc(ncomp, 1);
    for (int64_t v = 0; v < NN; v++) csize[comp[v]]++;
    for (int64_t v = 0; v < NN; v++)
        for (int in = 0; in < 4; in++) {
            int32_t u = succ[v * 4 + in];
            if (u >= 0 && comp[u] == comp[v]) { hasedge[comp[v]] = 1; if (in >> 1) hasov[comp[v]] = 1; }
        }
    int64_t ncyc = 0, ncycov = 0, nodes_in = 0, nodes_ov = 0; int32_t ex = -1;
    for (int32_t c = 0; c < ncomp; c++) if (hasedge[c]) { ncyc++; nodes_in += csize[c]; if (hasov[c]) { ncycov++; nodes_ov += csize[c]; if (ex < 0) ex = c; } }
    printf("core B=%d r=%d: nodes=%lld  cyclic SCCs=%lld (nodes %lld)  SCCs with digit growth=%lld (nodes %lld)\n",
           B, R, (long long)NN, (long long)ncyc, (long long)nodes_in, (long long)ncycov, (long long)nodes_ov);
    if (ex >= 0) {   // 例: SCC 内で ov=1 を含む閉路を1つ探して表示
        int32_t v0 = -1;
        for (int64_t v = 0; v < NN && v0 < 0; v++) if (comp[v] == ex) for (int in = 2; in < 4; in++) { int32_t u = succ[v * 4 + in]; if (u >= 0 && comp[u] == ex) v0 = (int32_t)v; }
        // BFS で v0 から v0 に戻る閉路（最初の辺は ov=1）
        int32_t *prev = malloc(sizeof(int32_t) * NN); int8_t *pin = malloc(NN);
        for (int64_t i = 0; i < NN; i++) prev[i] = -2;
        int32_t *q = malloc(sizeof(int32_t) * NN); int64_t qh = 0, qt = 0; int32_t first = -1; int fin = -1;
        for (int in = 2; in < 4 && first < 0; in++) { int32_t u = succ[(int64_t)v0 * 4 + in]; if (u >= 0 && comp[u] == ex) { first = u; fin = in; } }
        prev[first] = -1; q[qt++] = first; int found = (first == v0);
        while (qh < qt && !found) {
            int32_t v = q[qh++];
            for (int in = 0; in < 4; in++) { int32_t u = succ[(int64_t)v * 4 + in]; if (u < 0 || comp[u] != ex || prev[u] != -2) continue; if (qt >= NN) break; prev[u] = v; pin[u] = (int8_t)in; if (u == v0) { found = 1; break; } q[qt++] = u; }
        }
        int seq[4096], ns = 0; int32_t v = v0;
        if (first != v0) { while (v != first && ns < 4000) { seq[ns++] = pin[v]; v = prev[v]; } }
        seq[ns++] = fin;
        printf("  example cycle (length %d), inputs (cin,ov) in order:", ns);
        for (int i = ns - 1; i >= 0; i--) printf(" (%d,%d)", seq[i] & 1, seq[i] >> 1);
        printf("\n");
        // 先頭ノードの列和を表示
        int par, n, S[512]; core_decode(v0, &par, S, &n);
        printf("  start node: par=%d column sums:", par); for (int j = 0; j < n; j++) printf(" %d", S[j]); printf("\n");
    }
}

// ---------------- outer ----------------
static int Cq, Cd[16], Cg[16];   // 型C: 列の数字の巡回
static int outer_a[64], outer_b[64];
static int64_t outer_step(int64_t id, int typeC, int *ov_out, int *cbo_out) {
    int s[64], yb[64], ya[64], a2[64], ph = 0;
    if (typeC) { ph = (int)(id % Cq); id /= Cq; }
    for (int i = 0; i < K; i++) { s[i] = (int)(id % M); id /= M; }
    int c = 0;
    for (int i = 0; i < K; i++) { int v = s[i] + c; c = v >= B; yb[i] = v - (c ? B : 0); }
    int cbo = c, cti;
    if (typeC) {
        int d = Cd[ph], g = Cg[ph];
        if (cbo != g) return -1;                         // 列の最下端に欠陥が出ない
        cti = g;
        if (2 * d <= B - 1) { int bad = 0; for (int i = 0; i < K; i++) if (s[i] >= B) bad = 1; if (!bad) return -1; }   // 次が型A回文
        if (2 * d == 0 || 2 * d == B + 1) { int nb = 0; for (int i = 0; i < K; i++) if (s[i] != 0 && s[i] != B + 1) nb = 1; if (!nb) return -1; } // 型B回文
    } else cti = 1 - cbo;
    c = cti;
    for (int i = K - 1; i >= 0; i--) { int v = s[i] + c; c = v >= B; ya[i] = v - (c ? B : 0); }
    int ov = c;
    int high_new = typeC ? Cd[(ph + 1) % Cq] : (B - 1) * cbo;
    if (ov) { if (ya[K - 1] != high_new) return -1; a2[0] = 1; for (int i = 1; i < K; i++) a2[i] = ya[i - 1]; }
    else for (int i = 0; i < K; i++) a2[i] = ya[i];
    if (a2[0] == 0) return -1;
    memcpy(outer_a, a2, sizeof(int) * K); memcpy(outer_b, yb, sizeof(int) * K);
    int64_t nid = 0, mul = 1;
    for (int i = 0; i < K; i++) { nid += mul * (a2[i] + yb[i]); mul *= M; }
    if (typeC) nid = nid * Cq + (ph + 1) % Cq;
    *ov_out = ov; *cbo_out = cbo;
    return nid;
}

static void outer_cycles(int typeC) {
    int64_t N = 1; for (int i = 0; i < K; i++) N *= M;
    if (typeC) N *= Cq;
    uint8_t *col = calloc(N, 1);       // 0=未訪問, 1=探索中, 2=済
    int32_t *path = malloc(sizeof(int32_t) * N);
    int64_t ncyc = 0, ncyc_grow = 0;
    for (int64_t s0 = 0; s0 < N; s0++) {
        if (col[s0]) continue;
        int64_t v = s0; int len = 0;
        while (v >= 0 && !col[v]) { col[v] = 1; path[len++] = (int32_t)v; int ov, cbo; v = outer_step(v, typeC, &ov, &cbo); }
        if (v >= 0 && col[v] == 1) {      // 新しいサイクル
            int64_t u = v; int L = 0, gro = 0; char sig[4096]; int ns = 0;
            do { int ov, cbo; int64_t w = outer_step(u, typeC, &ov, &cbo); gro += ov; if (ns < 4000) { sig[ns++] = '0' + cbo; sig[ns++] = ov ? '^' : '.'; } u = w; L++; } while (u != v);
            sig[ns] = 0; ncyc++;
            if (gro > 0) { ncyc_grow++; if (ncyc_grow <= 20) printf("  cycle len %d, growth %d per cycle, signals(cbo,ov): %s\n", L, gro, sig); }
        }
        for (int i = 0; i < len; i++) col[path[i]] = 2;
    }
    printf("outer%s B=%d K=%d: states=%lld cycles=%lld, cycles with growth=%lld\n", typeC ? "C" : "A", B, K, (long long)N, (long long)ncyc, (long long)ncyc_grow);
}


// ---------------- match（型Aの完全なサイクル）----------------
static int rev_add(int *x, int D) {           // x[0..D-1] 下位から。戻り値: 新しい桁数
    int t[4096], c = 0;
    for (int i = 0; i < D; i++) t[i] = x[i] + x[D - 1 - i];
    for (int i = 0; i < D; i++) { int v = t[i] + c; c = v >= B; x[i] = v - (c ? B : 0); }
    if (c) x[D++] = 1;
    return D;
}
static int is_pal(const int *x, int D) { for (int i = 0, j = D - 1; i < j; i++, j--) if (x[i] != x[j]) return 0; return 1; }
static int build_number(int *x, const int *ob, const int *oa, const int *cw, int cl, int Lrun, int Hrun, int n) {
    int D = 0;
    for (int i = 0; i < K; i++) x[D++] = ob[i];
    for (int i = 0; i < n; i++) x[D++] = Lrun;
    for (int i = 0; i < cl; i++) x[D++] = cw[i];
    for (int i = 0; i < n; i++) x[D++] = Hrun;
    for (int i = K - 1; i >= 0; i--) x[D++] = oa[i];
    return D;
}
static const char *DG = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static void match_mode(void) {
    int64_t NO = 1; for (int i = 0; i < K; i++) NO *= M;
    uint8_t *col = calloc(NO, 1); int32_t *path = malloc(sizeof(int32_t) * NO);
    uint8_t *ccol = malloc(NN); int32_t *cpath = malloc(sizeof(int32_t) * NN);
    int64_t nouter = 0, nmatch = 0;
    for (int64_t s0 = 0; s0 < NO; s0++) {
        if (col[s0]) continue;
        int64_t v = s0; int len = 0;
        while (v >= 0 && !col[v]) { col[v] = 1; path[len++] = (int32_t)v; int ov, cbo; v = outer_step(v, 0, &ov, &cbo); }
        if (v >= 0 && col[v] == 1) {
            int L = 0, gro = 0, lab[8192]; int64_t onode[8192]; int64_t u = v;
            do { int ov, cbo; int64_t w = outer_step(u, 0, &ov, &cbo); if (L < 8192) { onode[L] = u; lab[L] = cbo + 2 * ov; } gro += ov; u = w; L++; } while (u != v);
            if (gro > 0 && L < 8192) {
                nouter++;
                int reps = (gro % 2) ? 2 : 1;     // コアの偶奇を戻すには桁あふれの総数が偶数でなければならない
                int LL = L * reps;
                memset(ccol, 0, NN);
                for (int64_t c0 = 0; c0 < NN; c0++) {
                    if (ccol[c0]) continue;
                    int64_t cv = c0; int cl = 0;
                    while (cv >= 0 && !ccol[cv]) {
                        ccol[cv] = 1; cpath[cl++] = (int32_t)cv;
                        int64_t z = cv;
                        for (int t = 0; t < LL && z >= 0; t++) z = succ[z * 4 + lab[t % L]];
                        cv = z;
                    }
                    if (cv >= 0 && ccol[cv] == 1) {          // コアの周期点 cv
                        nmatch++;
                        if (nmatch <= 5) {
                            // 位相0の窓の数字を作る: 外側は onode[L-1] から、コアは周期の最後のステップから
                            int ov, cbo; outer_step(onode[L - 1], 0, &ov, &cbo);
                            int oa[64], ob[64]; memcpy(oa, outer_a, sizeof(int) * K); memcpy(ob, outer_b, sizeof(int) * K);
                            // コア: cv から周期 m*LL ステップ進め、最後の1ステップの出力窓を取る
                            int64_t z = cv; int m = 0; int cw[512], cwl = 0;
                            do { for (int t = 0; t < LL; t++) { int64_t nz = core_step(z, lab[t % L]); if (t == LL - 1) { memcpy(cw, core_wout, sizeof(int) * core_wlen); cwl = core_wlen; } z = nz; } m++; } while (z != cv && m < 1000);
                            int cin_last = lab[(LL - 1) % L] & 1;
                            int Lrun = (B - 1) * (1 - cin_last), Hrun = (B - 1) * cin_last;
                            static int x[4096], xe[4096]; int n = 12;
                            int D = build_number(x, ob, oa, cw, cwl, Lrun, Hrun, n);
                            printf("  MATCH outer cycle len %d (growth %d), core period x%d. x0 (base %d, %d digits) = ", L, gro, m * reps, B, D);
                            for (int i = D - 1; i >= 0; i--) putchar(DG[x[i]]); putchar('\n');
                            // 直接シミュレーションで検証: 周期 P=m*LL 後に run が delta 伸びた同じ数になるか、回文が出ないか
                            int P = m * LL, delta = gro * reps * m / 2, ok = 1;
                            for (int rep = 1; rep <= 4 && ok; rep++) {
                                for (int t = 0; t < P; t++) { D = rev_add(x, D); if (is_pal(x, D)) { ok = 0; break; } }
                                int De = build_number(xe, ob, oa, cw, cwl, Lrun, Hrun, n + rep * delta);
                                if (De != D || memcmp(x, xe, sizeof(int) * D)) ok = 0;
                            }
                            printf("     verified by direct simulation (4 periods, run +%d per period): %s\n", delta, ok ? "YES" : "NO");
                        }
                    }
                    for (int i = 0; i < cl; i++) ccol[cpath[i]] = 2;
                }
            }
        }
        for (int i = 0; i < len; i++) col[path[i]] = 2;
    }
    printf("match B=%d r=%d K=%d: outer cycles with growth=%lld, matched core periodic points=%lld\n", B, R, K, (long long)nouter, (long long)nmatch);
}

int main(int argc, char **argv) {
    const char *mode = argv[1]; B = atoi(argv[2]); M = 2 * B - 1;
    if (!strcmp(mode, "core")) {
        R = atoi(argv[3]);
        N0 = B; for (int j = 0; j < R; j++) N0 *= M;
        N1 = 1; for (int j = 0; j <= R; j++) N1 *= M;
        NN = N0 + N1;
        succ = malloc(sizeof(int32_t) * NN * 4);
        for (int64_t v = 0; v < NN; v++) for (int in = 0; in < 4; in++) { int64_t u = core_step(v, in); succ[v * 4 + in] = (int32_t)u; }
        core_scc();
    } else if (!strcmp(mode, "match")) {
        R = atoi(argv[3]); K = atoi(argv[4]);
        N0 = B; for (int j = 0; j < R; j++) N0 *= M;
        N1 = 1; for (int j = 0; j <= R; j++) N1 *= M;
        NN = N0 + N1;
        succ = malloc(sizeof(int32_t) * NN * 4);
        for (int64_t v = 0; v < NN; v++) for (int in = 0; in < 4; in++) succ[v * 4 + in] = (int32_t)core_step(v, in);
        match_mode();
    } else if (!strcmp(mode, "outerA")) {
        K = atoi(argv[3]); outer_cycles(0);
    } else if (!strcmp(mode, "outerC")) {
        K = atoi(argv[3]); int d0 = atoi(argv[4]);
        Cq = 0; int d = d0;
        do { Cd[Cq] = d; Cg[Cq] = 2 * d >= B; int nd = 2 * d - (B - 1) * Cg[Cq]; Cq++; d = nd; } while (d != d0 && Cq < 16);
        printf("run digit cycle:"); for (int i = 0; i < Cq; i++) printf(" %d", Cd[i]); printf("\n");
        outer_cycles(1);
    }
    return 0;
}
