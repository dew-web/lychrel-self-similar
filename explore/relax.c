// relax.c — 緩和の実験: 一様乱数の D 桁の数から逆順加算を k 回行い、k 回目の直後の「良い対の連なり」の減衰率 λ を測る。
//   良い対 = 鏡の位置の2数字の和が B-1 以下（繰り上がりを生まない）。
//   W(j) = 対 i..i+j-1 がすべて良い割合（i は全位置で平均）。λ は ln W(j) の j=4..16 での傾きの指数。
//   「桁が増えた直後の項」と「増えなかった直後の項」を分けて集計する。
// usage: relax B D NSAMP KMAX SEED      出力: k, 桁が増えた直後の λ と標本数, 増えなかった直後の λ と標本数, 桁が増えた割合
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#define JMAX 24
static uint64_t s[2];
static inline uint64_t rnd(void) { uint64_t a = s[0], b = s[1]; s[0] = b; a ^= a << 23; s[1] = a ^ b ^ (a >> 17) ^ (b >> 26); return s[1] + b; }
static double lam(const double *cnt, const double *tot) {
    double sx = 0, sy = 0, sxx = 0, sxy = 0; int n = 0;
    for (int j = 4; j <= 16; j++) { if (cnt[j] <= 0) return NAN; double y = log(cnt[j] / tot[j]); sx += j; sy += y; sxx += (double)j * j; sxy += j * y; n++; }
    return exp((n * sxy - sx * sy) / (n * sxx - sx * sx));
}
int main(int argc, char **argv) {
    int B = atoi(argv[1]), D0 = atoi(argv[2]), NS = atoi(argv[3]), KMAX = atoi(argv[4]); s[0] = strtoull(argv[5], 0, 10) * 0x9E3779B97F4A7C15ull + 1; s[1] = 0xD1B54A32D192ED03ull;
    for (int i = 0; i < 20; i++) rnd();
    static double cnt[2][64][JMAX + 1], tot[2][64][JMAX + 1], nsamp[2][64];
    int *x = malloc(sizeof(int) * (D0 + KMAX + 8)), *t = malloc(sizeof(int) * (D0 + KMAX + 8));
    for (int n = 0; n < NS; n++) {
        int D = D0;
        for (int i = 0; i < D; i++) x[i] = (int)(rnd() % B);
        while (x[D - 1] == 0) x[D - 1] = (int)(rnd() % B);
        for (int k = 1; k <= KMAX; k++) {
            int c = 0;
            for (int i = 0; i < D; i++) t[i] = x[i] + x[D - 1 - i];
            for (int i = 0; i < D; i++) { int v = t[i] + c; c = v >= B; x[i] = v - (c ? B : 0); }
            int inc = c; if (c) x[D++] = 1;
            int g = inc ? 0 : 1, H = D / 2, run = 0;       // run = いま続いている良い対の数
            nsamp[g][k]++;
            for (int i = 0; i < H; i++) {
                if (x[i] + x[D - 1 - i] <= B - 1) run++; else run = 0;
                // 位置 i で終わる長さ j の窓がすべて良い ⇔ run >= j
                int m = run < JMAX ? run : JMAX;
                for (int j = 1; j <= m; j++) cnt[g][k][j]++;
            }
            for (int j = 1; j <= JMAX; j++) if (H - j + 1 > 0) tot[g][k][j] += H - j + 1;
        }
    }
    printf("# B=%d D=%d NS=%d\n# k lam_inc n_inc lam_noinc n_noinc frac_inc\n", B, D0, NS);
    for (int k = 1; k <= KMAX; k++)
        printf("%d %.4f %.0f %.4f %.0f %.3f\n", k, lam(cnt[0][k], tot[0][k]), nsamp[0][k], lam(cnt[1][k], tot[1][k]), nsamp[1][k], nsamp[0][k] / NS);
    return 0;
}
