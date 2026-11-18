// orbit_record.c — 実際の逆順加算の軌道を回して、両端の窓モデルの仮定を確かめる。
//
// usage: orbit_record B start steps [burn=3000] [H=2000] [L=3000] [R=8] [bitsfile]
//   start: "196"（10進のときだけ）か "rand:桁数:シード"
//
// burn 以降の steps ステップについて、次を記録する:
//   1. 桁あふれ（O）の頻度 = 1ステップあたりの桁の増え方。O の間に N（増えない）が何回続いたかの分布。
//   2. 最上位の数字の分布（ベンフォードの法則と比べる）。
//   3. k = 4, 8, 12, 16, 24, 32 について、真ん中から上の窓へ入る繰り上がり m = c_{D-k} の統計:
//      1 の割合、1〜4 ステップ前の m との相関、下の窓から出る繰り上がり c_k との一致、O との関係。
//   4. 厳密さの検査: 本物の m を与えた窓モデル（window.h）が、本物の両端の数字と毎ステップ一致すること。
//   5. 予測の効く長さ: H ステップごとに、m を確率 1/2 の乱数にした窓モデルを R 個ずつ（k = 8, 16, 32）走らせ、
//      外側 j 桁（j = 1, 2, 4, 8）と桁あふれの列が、本物とずれるまでのステップ数を測る（最大 L）。
// bitsfile を与えると、1ステップ1バイト（bit0 = O、bit1〜6 = 各 k の m）で書き出す。
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "window.h"

#define NKS 6
static const int KS[NKS] = {4, 8, 12, 16, 24, 32};
#define NPK 3
static const int PK[NPK] = {8, 16, 32};           // 予測の長さを測る k
#define NJ 4
static const int JS[NJ] = {1, 2, 4, 8};
#define NRUN 40
#define MAXCOPY 4096

static uint8_t *a; static long D;

// 本物の1ステップ。qpos（昇順）の各位置へ入る繰り上がりを cq に入れる。戻り値 = 桁あふれ
static int real_step(int B, const long *qpos, int nq, int *cq) {
    long h = D / 2;
    for (long i = 0; i < h; i++) { uint8_t s = (uint8_t)(a[i] + a[D - 1 - i]); a[i] = s; a[D - 1 - i] = s; }
    if (D & 1) a[h] = (uint8_t)(2 * a[h]);
    int c = 0; long i = 0;
    for (int e = 0; e <= nq; e++) {
        long end = e < nq ? qpos[e] : D;
        for (; i < end; i++) { int v = a[i] + c; c = v >= B; a[i] = (uint8_t)(v - (c ? B : 0)); }
        if (e < nq) cq[e] = c;
    }
    if (c) a[D++] = 1;
    return c;
}

typedef struct { int k, active; long age, div[NJ], divo; uint8_t bot[KMAX], top[KMAX]; } copy_t;
static copy_t CP[MAXCOPY];

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: orbit_record B start steps [burn] [H] [L] [R] [bitsfile]\n"); return 2; }
    int B = atoi(argv[1]); const char *start = argv[2]; long steps = atol(argv[3]);
    long burn = argc > 4 ? atol(argv[4]) : 3000, H = argc > 5 ? atol(argv[5]) : 2000, L = argc > 6 ? atol(argv[6]) : 3000;
    int R = argc > 7 ? atoi(argv[7]) : 8; const char *bitsfn = argc > 8 ? argv[8] : 0;
    if (B < 2 || B > 127) { fprintf(stderr, "B must be 2..127\n"); return 2; }
    long d0 = 0; uint64_t seed = 1;
    if (!strncmp(start, "rand:", 5)) { d0 = atol(start + 5); const char *q = strchr(start + 5, ':'); if (q) seed = strtoull(q + 1, 0, 10); }
    else if (B == 10) d0 = (long)strlen(start);
    else { fprintf(stderr, "a decimal start number needs B = 10\n"); return 2; }
    a = calloc((size_t)(d0 + burn + steps + 16), 1);
    rng_state = seed * 0x9E3779B97F4A7C15ull + 777;
    if (!strncmp(start, "rand:", 5)) { for (long i = 0; i < d0; i++) a[i] = (uint8_t)(rng64() % B); if (!a[d0 - 1]) a[d0 - 1] = 1; }
    else for (long i = 0; i < d0; i++) a[i] = (uint8_t)(start[d0 - 1 - i] - '0');
    D = d0;
    FILE *bf = bitsfn ? fopen(bitsfn, "wb") : 0;

    // 統計
    double nO = 0, lead[128] = {0}, runh[NRUN + 1] = {0}; long since = -1;
    double m1[NKS] = {0}, mm[NKS][5] = {{0}}, magree[NKS] = {0}, m1O[NKS] = {0}, m1N[NKS] = {0};
    int mhist[NKS][5] = {{0}};                     // 直近の m（相関用）
    long mism[NKS] = {0};
    uint8_t xb[NKS][KMAX], xt[NKS][KMAX];          // 厳密さの検査用の窓（本物の m で動かす）
    double divsum[NPK][NJ + 1] = {{0}}, divcens[NPK][NJ + 1] = {{0}}, divn[NPK] = {0};
    long divhist[NPK][NJ + 1][64] = {{{0}}};       // 2倍ごとの区切りの度数（中央値の目安）
    int ncopy = 0;
    clock_t t0 = clock();

    for (long t = 0; t < burn + steps; t++) {
        int post = t >= burn;
        if (post && D < 2 * KMAX + 4) { fprintf(stderr, "number too short (%ld digits) after burn-in\n", D); return 2; }
        if (t == burn) for (int i = 0; i < NKS; i++) for (int j = 0; j < KS[i]; j++) { xb[i][j] = a[j]; xt[i][j] = a[D - 1 - j]; }
        if (post && (t - burn) % H == 0)
            for (int pk = 0; pk < NPK; pk++) for (int r = 0; r < R; r++) {
                int slot = -1; for (int s = 0; s < ncopy; s++) if (!CP[s].active) { slot = s; break; }
                if (slot < 0) { if (ncopy >= MAXCOPY) break; slot = ncopy++; }
                copy_t *c = &CP[slot]; c->k = PK[pk]; c->active = 1; c->age = 0; c->divo = -1;
                for (int j = 0; j < NJ; j++) c->div[j] = -1;
                for (int j = 0; j < c->k; j++) { c->bot[j] = a[j]; c->top[j] = a[D - 1 - j]; }
            }
        // 本物の1ステップ（位置 k と D-k へ入る繰り上がりを記録）
        long qpos[2 * NKS]; int cq[2 * NKS], nq = 0;
        if (D >= 2 * KMAX + 4) { for (int i = 0; i < NKS; i++) qpos[nq++] = KS[i]; for (int i = NKS - 1; i >= 0; i--) qpos[nq++] = D - KS[i]; }
        long Dold = D;
        int O = real_step(B, qpos, nq, cq);
        if (!post) { if (O) since = 0; else if (since >= 0) since++; continue; }
        (void)Dold;
        int mk[NKS], ck[NKS];
        for (int i = 0; i < NKS; i++) { ck[i] = cq[i]; mk[i] = cq[2 * NKS - 1 - i]; }
        nO += O; lead[a[D - 1]]++;
        if (O) { if (since >= 0) runh[since < NRUN ? since : NRUN]++; since = 0; } else if (since >= 0) since++;
        uint8_t byte = (uint8_t)O;
        for (int i = 0; i < NKS; i++) {
            byte |= (uint8_t)(mk[i] << (i + 1));
            m1[i] += mk[i]; magree[i] += (mk[i] == ck[i]); if (O) m1O[i] += mk[i]; else m1N[i] += mk[i];
            for (int l = 1; l <= 4; l++) mm[i][l] += mk[i] * mhist[i][l - 1];
            for (int l = 4; l >= 1; l--) mhist[i][l] = mhist[i][l - 1];
            mhist[i][0] = mk[i];
            // 厳密さの検査
            int cb; int o2 = wstep(B, KS[i], xb[i], xt[i], mk[i], &cb);
            int bad = (o2 != O) || (cb != ck[i]);
            for (int j = 0; j < KS[i] && !bad; j++) if (xb[i][j] != a[j] || xt[i][j] != a[D - 1 - j]) bad = 1;
            if (bad) { mism[i]++; for (int j = 0; j < KS[i]; j++) { xb[i][j] = a[j]; xt[i][j] = a[D - 1 - j]; } }
        }
        if (bf) fputc(byte, bf);
        // 乱数の m で動かす窓モデル（予測の効く長さ）
        for (int s = 0; s < ncopy; s++) {
            copy_t *c = &CP[s]; if (!c->active) continue;
            int o2 = wstep(B, c->k, c->bot, c->top, (int)(rng64() & 1), 0);
            c->age++;
            if (c->divo < 0 && o2 != O) c->divo = c->age;
            for (int jj = 0; jj < NJ; jj++) {
                if (c->div[jj] >= 0 || JS[jj] > c->k) continue;
                for (int j = 0; j < JS[jj]; j++) if (c->bot[j] != a[j] || c->top[j] != a[D - 1 - j]) { c->div[jj] = c->age; break; }
            }
            int done = c->divo >= 0; for (int jj = 0; jj < NJ; jj++) if (JS[jj] <= c->k && c->div[jj] < 0) done = 0;
            if (done || c->age >= L) {
                int pk = 0; while (PK[pk] != c->k) pk++;
                divn[pk]++;
                for (int jj = 0; jj <= NJ; jj++) {
                    long v = jj < NJ ? c->div[jj] : c->divo;
                    if (jj < NJ && JS[jj] > c->k) continue;
                    if (v < 0) { divcens[pk][jj]++; v = L; }
                    divsum[pk][jj] += v;
                    int b = 0; while ((1L << (b + 1)) <= v && b < 62) b++;
                    divhist[pk][jj][b]++;
                }
                c->active = 0;
            }
        }
    }
    double cpu = (double)(clock() - t0) / CLOCKS_PER_SEC;
    if (bf) fclose(bf);

    printf("# orbit_record: base %d, start %s, %ld steps after %ld burn-in; final length %ld digits; cpu %.1f s\n", B, start, steps, burn, D, cpu);
    printf("overflow rate (digits per step) = %.5f   [log_B 2 = %.5f]\n", nO / steps, log(2.0) / log((double)B));
    double nr = 0; for (int i = 0; i <= NRUN; i++) nr += runh[i];
    printf("N-runs between O's:"); for (int i = 0; i <= 8; i++) printf(" n%d:%.4f", i, runh[i] / nr); printf("\n");
    printf("leading digit (orbit / Benford):");
    for (int d = 1; d < B && d < 10; d++) printf(" %d:%.3f/%.3f", d, lead[d] / steps, log(1.0 + 1.0 / d) / log((double)B));
    printf("\n");
    printf("middle carry m into the top window (carry into position D-k):\n");
    for (int i = 0; i < NKS; i++) {
        double p = m1[i] / steps, var = p * (1 - p);
        printf("  k=%2d  P(m=1)=%.4f  corr with m 1..4 steps before:", KS[i], p);
        for (int l = 1; l <= 4; l++) printf(" %+.4f", (mm[i][l] / steps - p * p) / var);
        printf("  P(m = carry out of bottom window)=%.4f  P(m=1|O)=%.4f  P(m=1|N)=%.4f  exactness mismatches=%ld\n",
               magree[i] / steps, m1O[i] / nO, m1N[i] / (steps - nO), mism[i]);
    }
    printf("steps until a window model with random m (p=1/2) disagrees with the real orbit (mean, censored at L=%ld; share not yet disagreeing):\n", L);
    for (int pk = 0; pk < NPK; pk++) {
        printf("  k=%2d (%4.0f runs):", PK[pk], divn[pk]);
        for (int jj = 0; jj < NJ; jj++) if (JS[jj] <= PK[pk]) printf("  outer %d digits %.1f (%.3f)", JS[jj], divsum[pk][jj] / divn[pk], divcens[pk][jj] / divn[pk]);
        printf("  overflow sequence %.1f (%.3f)\n", divsum[pk][NJ] / divn[pk], divcens[pk][NJ] / divn[pk]);
        printf("        overflow-sequence agreement length, histogram by powers of 2:");
        for (int b = 0; b < 14; b++) printf(" [%ld,%ld):%ld", 1L << b, 1L << (b + 1), divhist[pk][NJ][b]);
        printf("\n");
    }
    return 0;
}
