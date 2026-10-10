// outauto.c — 端の窓（外側の文字列）を「外側から1対ずつ内側へ」組み立てる有限オートマトン。
// 窓の長さ K を固定せずに、周期 P・桁あふれパターン ov について外側解が存在するかを完全に判定する。
//   状態 = 各位相 t の (下側の繰り上がり cb_t, 上側に要求される繰り上がり ca_t, 桁あふれ時の保留数字 pd_t)
//   遷移 = 次の1対 (位相0の数字 a,b を選ぶと、他の位相の数字は繰り上がりの選択で決まる)
//   受理 = 型A: ca_t = 1-cb_t かつ pd_t = ω·cb_t（長い 0/ω 列へ素通し、コアが反転）
//          型C: cb_t = ca_t = g_t かつ pd_t = d_{t+1}（中央一様列 d_t）
// 受理状態が見つかれば窓を復元し、型Aならコアの周期点を探して、実際の数を直接シミュレーションで検証する。
//
// usage: ./outauto A B P [rmax]        型A（全ての ov パターン, 偶数個）
//        ./outauto C B P d0             型C（中央列の初期数字 d0）
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static int B, P, OV[32], TYPEC, Dg[33], Gg[33], NEEDBAD[32], NEEDNB[32];
static uint64_t HCAP = 1ull << 24;
static int32_t *hv;
typedef struct { uint64_t w[4]; } skey_t;   // 256bit の状態キー（64bit では 2P+5·#ov ビットが溢れていた）
static inline int keq(const skey_t *a, const skey_t *b) { return a->w[0]==b->w[0] && a->w[1]==b->w[1] && a->w[2]==b->w[2] && a->w[3]==b->w[3]; }
static inline void setb(skey_t *k, int *sh, uint64_t v, int nb) { for (int i = 0; i < nb; i++, (*sh)++) if ((v >> i) & 1) k->w[*sh >> 6] |= 1ull << (*sh & 63); }
static inline uint64_t getb(const skey_t *k, int *sh, int nb) { uint64_t v = 0; for (int i = 0; i < nb; i++, (*sh)++) v |= ((k->w[*sh >> 6] >> (*sh & 63)) & 1ull) << i; return v; }
typedef struct { skey_t key; int32_t parent; uint8_t a0, b0; uint32_t ch; } entry;
static uint32_t *SLOT;
#ifndef ECAP_LOG
#define ECAP_LOG 23   /* 状態数の上限 = 2^ECAP_LOG（-DECAP_LOG=21 で小さく） */
#endif
static entry *E; static int64_t NE, ECAP = 1 << ECAP_LOG;

static int nflagbits(void) { int n = 0; for (int t = 0; t < P; t++) n += NEEDBAD[t] + NEEDNB[t]; return n; }

static skey_t pack(const int *cb, const int *ca, const int *pd, uint64_t flags, int root) {
    skey_t k = {{0, 0, 0, 0}}; int sh = 0;
    for (int t = 0; t < P; t++) setb(&k, &sh, (uint64_t)cb[t], 1);
    for (int t = 0; t < P; t++) setb(&k, &sh, (uint64_t)ca[t], 1);
    for (int t = 0; t < P; t++) if (OV[t]) setb(&k, &sh, (uint64_t)pd[t], 5);
    setb(&k, &sh, flags, nflagbits());
    setb(&k, &sh, (uint64_t)root, 1);
    if (sh > 256) { fprintf(stderr, "key too long\n"); exit(1); }
    return k;
}
static void unpack(skey_t k, int *cb, int *ca, int *pd, uint64_t *flags, int *root) {
    int sh = 0;
    for (int t = 0; t < P; t++) cb[t] = (int)getb(&k, &sh, 1);
    for (int t = 0; t < P; t++) ca[t] = (int)getb(&k, &sh, 1);
    for (int t = 0; t < P; t++) pd[t] = OV[t] ? (int)getb(&k, &sh, 5) : 0;
    *flags = getb(&k, &sh, nflagbits());
    *root = (int)getb(&k, &sh, 1);
}
static int64_t hfind_or_add(skey_t key, int32_t parent, int a0, int b0, uint32_t ch, int *isnew) {
    uint64_t x = key.w[0] * 0x9E3779B97F4A7C15ull ^ key.w[1] * 0xC2B2AE3D27D4EB4Full ^ key.w[2] * 0x165667B19E3779F9ull ^ key.w[3] * 0x27D4EB2F165667C5ull;
    uint64_t h = (x ^ (x >> 31)) & (HCAP - 1);
    while (hv[h] >= 0) { if (keq(&E[hv[h]].key, &key)) { *isnew = 0; return hv[h]; } h = (h + 1) & (HCAP - 1); }
    if (NE >= ECAP) { *isnew = -1; return -1; }
    hv[h] = (int32_t)NE; SLOT[NE] = (uint32_t)h;
    E[NE].key = key; E[NE].parent = parent; E[NE].a0 = (uint8_t)a0; E[NE].b0 = (uint8_t)b0; E[NE].ch = ch;
    *isnew = 1; return NE++;
}

// 1対ぶんの遷移（DFS で各位相の上側繰り上がりの選択を列挙）
static int cur_cb[32], cur_ca[32], cur_pd[32], cur_root; static uint64_t cur_flags;
static int n_cb[32], n_ca[32], n_pd[32], seqa[33], seqb[33];
static int ALLMODE = 0; static int64_t TERMS[100000]; static int NTERMS = 0;
static int64_t found_terminal = -1; static int64_t cur_parent; static int cap_hit;

static int is_terminal(const int *cb, const int *ca, const int *pd, uint64_t flags) {
    if (!TYPEC) {
        for (int t = 0; t < P; t++) { if (ca[t] != 1 - cb[t]) return 0; if (OV[t] && pd[t] != (B - 1) * cb[t]) return 0; }
        return 1;
    }
    for (int t = 0; t < P; t++) { if (cb[t] != Gg[t] || ca[t] != Gg[t]) return 0; if (OV[t] && pd[t] != Dg[t + 1]) return 0; }
    int nf = nflagbits(); return flags == (nf >= 64 ? ~0ull : ((1ull << nf) - 1));
}

static void dfs(int t, int a, int b, uint32_t ch, uint64_t flags, int a0, int b0) {
    if (found_terminal >= 0 || cap_hit) return;
    if (t == P) {
        if (a != a0 || b != b0) return;
        int isnew; int64_t id = hfind_or_add(pack(n_cb, n_ca, n_pd, flags, 0), (int32_t)cur_parent, a0, b0, ch, &isnew);
        if (isnew < 0) { cap_hit = 1; return; }
        if (isnew == 1 && is_terminal(n_cb, n_ca, n_pd, flags)) { if (ALLMODE) { if (NTERMS < 100000) TERMS[NTERMS++] = id; } else found_terminal = id; }
        return;
    }
    if (cur_root && a == 0) return;                         // 先頭の数字は 0 でない
    int s = a + b;
    int nb = s + cur_cb[t]; n_cb[t] = nb >= B; nb -= n_cb[t] ? B : 0;
    uint64_t f2 = flags; int fi = 0;
    for (int u = 0; u < t; u++) fi += NEEDBAD[u] + NEEDNB[u];
    if (NEEDBAD[t]) { if (s >= B) f2 |= 1ull << fi; fi++; }
    if (NEEDNB[t]) { if (s != 0 && s != B + 1) f2 |= 1ull << fi; fi++; }
    for (int c = 0; c <= 1; c++) {
        if (((s + c) >= B) != cur_ca[t]) continue;           // 上側の桁から出る繰り上がりが要求と一致
        int ya = s + c - ((s + c) >= B ? B : 0);
        n_ca[t] = c;
        int an;
        if (OV[t]) { an = cur_pd[t]; n_pd[t] = ya; } else { an = ya; n_pd[t] = 0; }
        seqa[t + 1] = an; seqb[t + 1] = nb;
        dfs(t + 1, an, nb, ch | ((uint32_t)c << t), f2, a0, b0);
    }
}

// 経路の復元: 各対 i について全位相の (a_t[i], b_t[i])
static int WA[4096][32], WB[4096][32], KW;
static int replay(int64_t from, int a0, int b0, uint32_t ch, int *outa, int *outb) {
    int cb[32], ca[32], pd[32], root; uint64_t fl;
    unpack(E[from].key, cb, ca, pd, &fl, &root);
    int a = a0, b = b0;
    for (int t = 0; t < P; t++) {
        outa[t] = a; outb[t] = b;
        int s = a + b, nb = s + cb[t]; nb -= nb >= B ? B : 0;
        int c = (ch >> t) & 1, ya = s + c; ya -= ya >= B ? B : 0;
        a = OV[t] ? pd[t] : ya; b = nb;
    }
    return 1;
}

// ---------------- コア（ssgraph と同じ） ----------------
static int R, M; static int64_t N0, N1, NN; static int32_t *succ;
static int core_wout[64], core_wlen;
static void core_decode(int64_t id, int *par, int *S, int *n) {
    if (id < N0) { *par = 0; *n = 2 * R + 1; int cd = (int)(id % B); id /= B;
        for (int j = 0; j < R; j++) { int s = (int)(id % M); id /= M; S[j] = s; S[*n - 1 - j] = s; } S[R] = 2 * cd; }
    else { id -= N0; *par = 1; *n = 2 * R + 2;
        for (int j = 0; j <= R; j++) { int s = (int)(id % M); id /= M; S[j] = s; S[*n - 1 - j] = s; } }
}
static int64_t core_enc(const int *w, int n) {
    int64_t id = 0, mul = 1;
    if (n == 2 * R + 1) { id = w[R]; mul = B; for (int j = 0; j < R; j++) { id += mul * (w[j] + w[n - 1 - j]); mul *= M; } return id; }
    for (int j = 0; j <= R; j++) { id += mul * (w[j] + w[n - 1 - j]); mul *= M; }
    return N0 + id;
}
static int64_t core_step(int64_t id, int in) {
    int cin = in & 1, ov = in >> 1, par, n, S[64], y[64], w[64];
    core_decode(id, &par, S, &n);
    int c = cin;
    for (int j = 0; j < n; j++) { int v = S[j] + c; c = v >= B; y[j] = v - (c ? B : 0); }
    if (c != 1 - cin) return -1;
    if (!ov) { memcpy(core_wout, y, sizeof(int) * n); core_wlen = n; return core_enc(y, n); }
    if (par == 0) { memcpy(w, y, sizeof(int) * n); w[n] = (B - 1) * cin; memcpy(core_wout, w, sizeof(int) * (n + 1)); core_wlen = n + 1; return core_enc(w, n + 1); }
    if (y[0] != (B - 1) * (1 - cin)) return -1;
    memcpy(core_wout, y + 1, sizeof(int) * (n - 1)); core_wlen = n - 1;
    return core_enc(y + 1, n - 1);
}

static int rev_add(int *x, int D) {
    static int t[1 << 16]; int c = 0;
    for (int i = 0; i < D; i++) t[i] = x[i] + x[D - 1 - i];
    for (int i = 0; i < D; i++) { int v = t[i] + c; c = v >= B; x[i] = v - (c ? B : 0); }
    if (c) x[D++] = 1;
    return D;
}
static int is_pal(const int *x, int D) { for (int i = 0, j = D - 1; i < j; i++, j--) if (x[i] != x[j]) return 0; return 1; }
static const char *DG = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static int buildA(int *x, int ph, const int *cw, int cl, int Lr, int Hr, int n) {
    int D = 0;
    for (int i = 0; i < KW; i++) x[D++] = WB[i][ph];
    for (int i = 0; i < n; i++) x[D++] = Lr;
    for (int i = 0; i < cl; i++) x[D++] = cw[i];
    for (int i = 0; i < n; i++) x[D++] = Hr;
    for (int i = KW - 1; i >= 0; i--) x[D++] = WA[i][ph];
    return D;
}
static int buildC(int *x, int ph, int n) {
    int D = 0;
    for (int i = 0; i < KW; i++) x[D++] = WB[i][ph];
    for (int i = 0; i < n; i++) x[D++] = Dg[ph];
    for (int i = KW - 1; i >= 0; i--) x[D++] = WA[i][ph];
    return D;
}

// 外側解 + コアの周期点 → 実際の数を作って検証
static void finish(int64_t term, int rmax) {
    // 窓の復元
    int64_t chain[4096]; int L = 0; int64_t v = term;
    while (v > 0 && L < 4096) { chain[L++] = v; v = E[v].parent; }
    KW = L;
    for (int i = 0; i < L; i++) { int64_t e = chain[L - 1 - i]; replay(E[e].parent, E[e].a0, E[e].b0, E[e].ch, WA[i], WB[i]); }
    int cb[32], ca[32], pd[32], root; uint64_t fl; unpack(E[term].key, cb, ca, pd, &fl, &root);
    printf("    outer windows K=%d; carry into run per phase (cin):", KW); for (int t = 0; t < P; t++) printf("%d", cb[t]); printf("\n");
    static int x[1 << 16], xe[1 << 16];
    int n = 15, Dsum = 0; for (int t = 0; t < P; t++) Dsum += OV[t];
    if (TYPEC) {
        int D = buildC(x, 0, n);
        printf("    x0 = "); for (int i = D - 1; i >= 0; i--) putchar(DG[x[i]]); printf("  (%d digits)\n", D);
        int ok = 1;
        for (int rep = 1; rep <= 4 && ok; rep++) {
            for (int t = 0; t < P; t++) { D = rev_add(x, D); if (is_pal(x, D)) { ok = 0; break; } }
            int De = buildC(xe, 0, n + rep * Dsum);
            if (De != D || memcmp(x, xe, sizeof(int) * D)) ok = 0;
        }
        printf("    direct simulation, 4 periods (central run +%d per period, no palindrome): %s\n", Dsum, ok ? "VERIFIED" : "FAILED");
        return;
    }
    for (R = 1; R <= rmax; R++) {
        M = 2 * B - 1; N0 = B; for (int j = 0; j < R; j++) N0 *= M; N1 = 1; for (int j = 0; j <= R; j++) N1 *= M; NN = N0 + N1;
        succ = realloc(succ, sizeof(int32_t) * NN * 4);
        for (int64_t u = 0; u < NN; u++) for (int in = 0; in < 4; in++) succ[u * 4 + in] = (int32_t)core_step(u, in);
        uint8_t *col = calloc(NN, 1); int32_t *path = malloc(sizeof(int32_t) * NN);
        int64_t pp = -1;
        for (int64_t c0 = 0; c0 < NN && pp < 0; c0++) {
            if (col[c0]) continue;
            int64_t cv = c0; int64_t cl = 0;
            while (cv >= 0 && !col[cv]) { col[cv] = 1; path[cl++] = (int32_t)cv; int64_t z = cv; for (int t = 0; t < P && z >= 0; t++) z = succ[z * 4 + cb[t] + 2 * OV[t]]; cv = z; }
            if (cv >= 0 && col[cv] == 1) pp = cv;
            for (int64_t i = 0; i < cl; i++) col[path[i]] = 2;
        }
        free(col); free(path);
        if (pp < 0) { printf("    core radius %d: no periodic core for this signal\n", R); continue; }
        // コアの位相0の窓
        int64_t z = pp; int m = 0, cw[64], cwl = 0;
        do { for (int t = 0; t < P; t++) { int64_t nz = core_step(z, cb[t] + 2 * OV[t]); if (t == P - 1) { memcpy(cw, core_wout, sizeof(int) * core_wlen); cwl = core_wlen; } z = nz; } m++; } while (z != pp && m < 10000);
        int Lr = (B - 1) * (1 - cb[P - 1]), Hr = (B - 1) * cb[P - 1];
        int D = buildA(x, 0, cw, cwl, Lr, Hr, n);
        printf("    core radius %d: periodic core found (core period = %d x P)\n    x0 = ", R, m);
        for (int i = D - 1; i >= 0; i--) putchar(DG[x[i]]); printf("  (%d digits, base %d)\n", D, B);
        int PP = m * P, delta = m * Dsum / 2, ok = 1;
        for (int rep = 1; rep <= 4 && ok; rep++) {
            for (int t = 0; t < PP; t++) { D = rev_add(x, D); if (is_pal(x, D)) { ok = 0; break; } }
            int De = buildA(xe, 0, cw, cwl, Lr, Hr, n + rep * delta);
            if (De != D || memcmp(x, xe, sizeof(int) * D)) ok = 0;
        }
        printf("    direct simulation, 4 periods (each run +%d per period, no palindrome): %s\n", delta, ok ? "VERIFIED" : "FAILED");
        return;
    }
}

static int run_pattern(int rmax) {
    for (int64_t i = 0; i < NE; i++) hv[SLOT[i]] = -1;
    NE = 0;
    found_terminal = -1; cap_hit = 0;
    int cb[32], ca[32], pd[32];
    for (int t = 0; t < P; t++) { cb[t] = 0; ca[t] = OV[t]; pd[t] = OV[t] ? 1 : 0; }
    int isnew; hfind_or_add(pack(cb, ca, pd, 0, 1), -1, 0, 0, 0, &isnew);
    for (int64_t q = 0; q < NE && found_terminal < 0 && !cap_hit; q++) {
        uint64_t fl; unpack(E[q].key, cur_cb, cur_ca, cur_pd, &fl, &cur_root);
        cur_flags = fl; cur_parent = q;
        for (int a0 = 0; a0 < B; a0++) for (int b0 = 0; b0 < B; b0++) dfs(0, a0, b0, 0, fl, a0, b0);
    }
    if (ALLMODE) {
        printf("  ov="); for (int t = 0; t < P; t++) putchar('0' + OV[t]);
        printf("  reachable=%lld terminals=%d%s\n", (long long)NE, NTERMS, cap_hit ? " (CAPACITY)" : "");
        uint32_t seen[4096]; int ns = 0;
        for (int k = 0; k < NTERMS; k++) {
            int cb[32], ca[32], pd[32], root; uint64_t fl; unpack(E[TERMS[k]].key, cb, ca, pd, &fl, &root);
            uint32_t v = 0; for (int t = 0; t < P; t++) v |= (uint32_t)cb[t] << t;
            int dup = 0; for (int i = 0; i < ns; i++) if (seen[i] == v) dup = 1;
            if (dup || ns >= 4096) continue;
            seen[ns++] = v;
            int64_t chain[4096]; int L = 0; int64_t u = TERMS[k];
            while (u > 0 && L < 4096) { chain[L++] = u; u = E[u].parent; }
            KW = L;
            for (int i = 0; i < L; i++) { int64_t e = chain[L - 1 - i]; replay(E[e].parent, E[e].a0, E[e].b0, E[e].ch, WA[i], WB[i]); }
            char fn[256]; char ovs[40], cbs[40];
            for (int t = 0; t < P; t++) { ovs[t] = '0' + OV[t]; cbs[t] = '0' + cb[t]; } ovs[P] = cbs[P] = 0;
            snprintf(fn, sizeof fn, "outer_B%d_P%d_ov%s_cin%s.txt", B, P, ovs, cbs);
            FILE *f = fopen(fn, "w");
            fprintf(f, "%d %d %d %s %s\n", B, P, KW, ovs, cbs);
            for (int t = 0; t < P; t++) { for (int i = 0; i < KW; i++) fprintf(f, "%d ", WA[i][t]); fprintf(f, "| "); for (int i = 0; i < KW; i++) fprintf(f, "%d ", WB[i][t]); fprintf(f, "\n"); }
            fclose(f);
            printf("    distinct cin=%s  (first K=%d) -> %s\n", cbs, KW, fn);
        }
        NTERMS = 0;
        return found_terminal >= 0;
    }
    printf("  ov=");
    for (int t = 0; t < P; t++) putchar('0' + OV[t]);
    printf("  reachable states=%lld  ", (long long)NE);
    if (cap_hit) { printf("CAPACITY EXCEEDED (inconclusive)\n"); return -1; }
    if (found_terminal < 0) { printf("no outer solution of any length\n"); return 0; }
    printf("OUTER SOLUTION FOUND\n");
    finish(found_terminal, rmax);
    return 1;
}

int main(int argc, char **argv) {
    TYPEC = argv[1][0] == 'C'; ALLMODE = argv[1][1] == 'a'; B = atoi(argv[2]); P = atoi(argv[3]);
    int rmax = 4;
    hv = malloc(sizeof(int32_t) * HCAP); E = malloc(sizeof(entry) * ECAP); SLOT = malloc(sizeof(uint32_t) * ECAP);
    for (uint64_t i = 0; i < HCAP; i++) hv[i] = -1; NE = 0;
    if (TYPEC) {
        int d0 = atoi(argv[4]), d = d0;
        for (int t = 0; t <= P; t++) { Dg[t] = d; if (t < P) { Gg[t] = 2 * d >= B; d = 2 * d - (B - 1) * Gg[t]; } }
        if (Dg[P] != d0) { printf("run digit %d does not return after %d steps\n", d0, P); return 0; }
        for (int t = 0; t < P; t++) { NEEDBAD[t] = 2 * Dg[t] <= B - 1; NEEDNB[t] = (2 * Dg[t] == 0 || 2 * Dg[t] == B + 1); }
        printf("type C, base %d, period %d, central run digits:", B, P); for (int t = 0; t < P; t++) printf(" %d", Dg[t]); printf("\n");
    } else {
        if (argc > 4) rmax = atoi(argv[4]);
        printf("type A, base %d, period %d\n", B, P);
    }
    if (argc > 5 && argv[5][0] == 'p') {          // 1つのパターンだけ調べる
        for (int t = 0; t < P; t++) OV[t] = argv[5][1 + t] - '0';
        int r = run_pattern(rmax);
        printf("single pattern result=%d\n", r);
        return 0;
    }
    int nsol = 0, ninc = 0, npat = 0;
    for (uint32_t mask = 1; mask < (1u << P); mask++) {
        int w = __builtin_popcount(mask);
        if (!TYPEC && (w % 2)) continue;
        if (!(mask >> (P - 1) & 1)) continue;                 // 回転の代表: 最後のステップで桁あふれ
        int minrot = 1;
        for (int k = 1; k < P; k++) { uint32_t r = ((mask >> k) | (mask << (P - k))) & ((1u << P) - 1); if ((r >> (P - 1) & 1) && r < mask) { minrot = 0; break; } }
        if (!minrot) continue;
        for (int t = 0; t < P; t++) OV[t] = (mask >> t) & 1;
        npat++;
        int r = run_pattern(rmax);
        if (r > 0) nsol++; else if (r < 0) ninc++;
    }
    printf("summary: patterns=%d, with outer solution=%d, inconclusive=%d\n", npat, nsol, ninc);
    return 0;
}
