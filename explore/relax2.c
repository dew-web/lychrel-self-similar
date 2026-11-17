// relax2.c — 定常状態の λ を「直前の履歴」で分けて測る。
//   桁が増えたステップを O、増えなかったステップを N と書く。O の直後の項について、
//   その O の前に N が何回続いたか（n）で標本を分け、n ごとに良い対の連なりの減衰率 λ(n) と良い対の密度 W(1) を出す。
// usage: relax2 B D NSAMP KMAX KSKIP SEED
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#define JMAX 24
#define NMAX 16
static uint64_t s[2];
static inline uint64_t rnd(void) { uint64_t a = s[0], b = s[1]; s[0] = b; a ^= a << 23; s[1] = a ^ b ^ (a >> 17) ^ (b >> 26); return s[1] + b; }
static double lam(const double *cnt, const double *tot) {
    double sx = 0, sy = 0, sxx = 0, sxy = 0; int n = 0;
    for (int j = 4; j <= 16; j++) { if (cnt[j] <= 0) return NAN; double y = log(cnt[j] / tot[j]); sx += j; sy += y; sxx += (double)j * j; sxy += j * y; n++; }
    return exp((n * sxy - sx * sy) / (n * sxx - sx * sx));
}
int main(int argc, char **argv) {
    int B = atoi(argv[1]), D0 = atoi(argv[2]), NS = atoi(argv[3]), KMAX = atoi(argv[4]), KSKIP = atoi(argv[5]);
    s[0] = strtoull(argv[6], 0, 10) * 0x9E3779B97F4A7C15ull + 1; s[1] = 0xD1B54A32D192ED03ull; for (int i = 0; i < 20; i++) rnd();
    static double cnt[NMAX + 2][JMAX + 1], tot[NMAX + 2][JMAX + 1], ns[NMAX + 2], cm[NMAX + 2], cp[NMAX + 2], cs[NMAX + 2], npair[NMAX + 2];
    static double COS[1024]; for (int i = 0; i < B; i++) COS[i] = cos(2 * M_PI * i / B);
    int *x = malloc(sizeof(int) * (D0 + KMAX + 8)), *t = malloc(sizeof(int) * (D0 + KMAX + 8));
    for (int n = 0; n < NS; n++) {
        int D = D0, nrun = -1;                                // nrun = 直前の O からの N の回数（最初の O までは -1）
        for (int i = 0; i < D; i++) x[i] = (int)(rnd() % B);
        while (x[D - 1] == 0) x[D - 1] = (int)(rnd() % B);
        for (int k = 1; k <= KMAX; k++) {
            int c = 0;
            for (int i = 0; i < D; i++) t[i] = x[i] + x[D - 1 - i];
            for (int i = 0; i < D; i++) { int v = t[i] + c; c = v >= B; x[i] = v - (c ? B : 0); }
            if (!c) { if (nrun >= 0) nrun++; continue; }
            x[D++] = 1;
            if (k > KSKIP && nrun >= 0) {
                int g = nrun > NMAX ? NMAX : nrun, H = D / 2, run = 0;
                for (int gg = g; gg <= NMAX + 1; gg += NMAX + 1 - g) {   // g と、全体（NMAX+1）の両方に足す
                    ns[gg]++; run = 0;
                    for (int i = 0; i < H; i++) {
                        if (x[i] + x[D - 1 - i] <= B - 1) run++; else run = 0;
                        int m = run < JMAX ? run : JMAX;
                        for (int j = 1; j <= m; j++) cnt[gg][j]++;
                    }
                    for (int j = 1; j <= JMAX; j++) tot[gg][j] += H - j + 1;
                    // 鏡の位置の2数字 z（上位側）, y（下位側）の関係: 差がそろう度合い cm、和がそろう度合い cp、隣の対との関係 cs
                    for (int i = 1; i < H; i++) { int y = x[i], z = x[D - 1 - i]; cm[gg] += COS[((z - y) % B + B) % B]; cp[gg] += COS[(z + y) % B]; cs[gg] += COS[((x[i] - x[i - 1]) % B + B) % B]; npair[gg]++; }
                    if (gg == NMAX + 1) break;
                }
            }
            nrun = 0;
        }
    }
    printf("# B=%d D=%d NS=%d K=%d..%d\n# n frac lambda W1 coh_diff coh_sum coh_adjacent\n", B, D0, NS, KSKIP + 1, KMAX);
    for (int g = 0; g <= NMAX + 1; g++) if (ns[g] > 0)
        printf("%s%d %.4f %.4f %.4f %.4f %.4f %.4f\n", g == NMAX + 1 ? "all " : "", g == NMAX + 1 ? -1 : g, ns[g] / ns[NMAX + 1], lam(cnt[g], tot[g]), cnt[g][1] / tot[g][1], cm[g] / npair[g], cp[g] / npair[g], cs[g] / npair[g]);
    return 0;
}
