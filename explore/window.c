// window.c — 両端の窓モデル。下位 k 桁と上位 k 桁だけを追い、真ん中から上の窓へ入る繰り上がりを
// 確率 p の独立な乱数とみなす（window.h を参照）。1ステップの計算量は k に比例し、数の桁数によらない。
//
// usage: window B k[,k2,...] steps seed [p=0.5] [burn=100000] [rho=0]
//   rho: 真ん中の繰り上がりの1ステップ前との相関（0 なら独立）。m_t は確率 p + rho·(m_{t-1} − p) で 1。
// 出力（k ごと）:
//   - 桁あふれの頻度（= 1ステップあたりの桁の増え方。196 の軌道では 0.41393）
//   - O（桁が増える）の間に N（増えない）が何回続いたかの分布
//   - 最上位の数字の分布と、ベンフォードの法則 log_B(1 + 1/d) との比較
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "window.h"

#define NRUN 40

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: window B k[,k2,...] steps seed [p=0.5] [burn=100000]\n"); return 2; }
    int B = atoi(argv[1]); long steps = atol(argv[3]); uint64_t seed = strtoull(argv[4], 0, 10);
    if (B < 2 || B > 256) { fprintf(stderr, "B must be 2..256 (digits are stored in one byte)\n"); return 2; }
    double p = argc > 5 ? atof(argv[5]) : 0.5; long burn = argc > 6 ? atol(argv[6]) : 100000;
    double rho = argc > 7 ? atof(argv[7]) : 0.0;
    printf("# window model: base %d, middle carry ~ Bernoulli(%.3f), lag-1 correlation %.3f, %ld steps after %ld burn-in, seed %llu\n",
           B, p, rho, steps, burn, (unsigned long long)seed);
    printf("# log_B 2 = %.5f (the overflow rate if the ends were mirror images and each step doubled the number)\n", log(2.0) / log((double)B));
    char *ks = argv[2];
    while (*ks) {
        int k = atoi(ks); while (*ks && *ks != ',') ks++; if (*ks == ',') ks++;
        if (k < 1 || k > KMAX) { fprintf(stderr, "k must be 1..%d\n", KMAX); return 2; }
        rng_state = seed * 0x9E3779B97F4A7C15ull + (uint64_t)k * 1000003ull + 12345;
        uint8_t bot[KMAX], top[KMAX];
        for (int j = 0; j < k; j++) { bot[j] = (uint8_t)(rng64() % B); top[j] = (uint8_t)(rng64() % B); }
        if (!top[0]) top[0] = 1;
        double nO = 0, lead[256] = {0}, runh[NRUN + 1] = {0}; long since = -1;
        double hend[2][KMAX + 2] = {{0}}, nst[2] = {0}; int prevO = 1;   // 外側から良い対（和 <= B-1）が何個続くか（直前が O か N かで分ける）
        int mprev = 0;
        for (long t = 0; t < burn + steps; t++) {
            double q = p + rho * (mprev - p);
            int m = rng01() < q; mprev = m;
            if (t >= burn) { int L = 0; while (L < k && bot[L] + top[L] <= B - 1) L++; hend[prevO][L]++; nst[prevO]++; }
            int o = wstep(B, k, bot, top, m, 0);
            prevO = o;
            if (t < burn) { if (o) since = 0; else if (since >= 0) since++; continue; }
            nO += o; lead[top[0]]++;
            if (o) { if (since >= 0) runh[since < NRUN ? since : NRUN]++; since = 0; }
            else if (since >= 0) since++;
        }
        double nr = 0; for (int i = 0; i <= NRUN; i++) nr += runh[i];
        printf("k=%2d  overflow rate (digits per step) = %.5f\n", k, nO / steps);
        printf("      N-runs between O's:"); for (int i = 0; i <= 8; i++) printf(" n%d:%.4f", i, runh[i] / nr); printf("\n");
        printf("      leading digit (window / Benford):");
        for (int d = 1; d < B && d < 10; d++) printf(" %d:%.3f/%.3f", d, lead[d] / steps, log(1.0 + 1.0 / d) / log((double)B));
        printf("\n");
        for (int st = 1; st >= 0; st--) {
            printf("      E(j) after %s (P(outermost j pairs all have sum <= B-1)):", st ? "O" : "N");
            double c = nst[st];
            for (int j = 1; j <= k && j <= 24; j++) { c -= hend[st][j - 1]; if (j <= 12 || j % 4 == 0) printf(" %d:%.3e", j, c / nst[st]); }
            printf("\n");
        }
        fflush(stdout);
    }
    return 0;
}
