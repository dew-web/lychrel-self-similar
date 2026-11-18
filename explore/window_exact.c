// window_exact.c — 両端の窓モデル（window.h）を有限のマルコフ連鎖として厳密に解く。
// 状態 = 下の窓 k 桁 + 上の窓 k 桁（B^(2k) 通り）。真ん中から入る繰り上がり m は確率 p で 1。
// 定常分布をべき乗法（遅延つき: π ← (π + πP)/2）で求め、桁あふれの頻度と最上位の数字の分布を出す。
//
// usage: window_exact B k [p=0.5] [tol=1e-13] [maxit=200000]
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "window.h"

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: window_exact B k [p=0.5] [tol] [maxit]\n"); return 2; }
    int B = atoi(argv[1]), k = atoi(argv[2]);
    if (B < 2 || B > 256) { fprintf(stderr, "B must be 2..256\n"); return 2; }
    double p = argc > 3 ? atof(argv[3]) : 0.5, tol = argc > 4 ? atof(argv[4]) : 1e-13;
    long maxit = argc > 5 ? atol(argv[5]) : 200000;
    long N = 1; for (int i = 0; i < 2 * k; i++) N *= B;
    if (N > 400000000L) { fprintf(stderr, "too many states (%ld)\n", N); return 2; }
    // 遷移先（m = 0, 1）と桁あふれを前もって計算する
    uint32_t *nx0 = malloc(sizeof(uint32_t) * N), *nx1 = malloc(sizeof(uint32_t) * N);
    uint8_t *ov = malloc(N);                       // bit0: m=0 で桁あふれ, bit1: m=1 で桁あふれ
    for (long s = 0; s < N; s++) {
        for (int m = 0; m <= 1; m++) {
            uint8_t bot[KMAX], top[KMAX]; long x = s;
            for (int j = 0; j < k; j++) { bot[j] = (uint8_t)(x % B); x /= B; }
            for (int j = 0; j < k; j++) { top[j] = (uint8_t)(x % B); x /= B; }
            int o = wstep(B, k, bot, top, m, 0);
            long y = 0;
            for (int j = k - 1; j >= 0; j--) y = y * B + top[j];
            for (int j = k - 1; j >= 0; j--) y = y * B + bot[j];
            if (m) { nx1[s] = (uint32_t)y; ov[s] |= (uint8_t)(o << 1); } else { nx0[s] = (uint32_t)y; ov[s] = (uint8_t)o; }
        }
    }
    double *pi = malloc(sizeof(double) * N), *nw = malloc(sizeof(double) * N);
    long ok0 = 0; long topstride = 1; for (int i = 0; i < k; i++) topstride *= B;   // 上の窓の最も外側（最上位の数字）
    long topmul = 1; for (int i = 0; i < k; i++) topmul *= B;                        // 状態の中で top[0] の重み
    (void)topstride;
    for (long s = 0; s < N; s++) { int t0 = (int)((s / topmul) % B); pi[s] = t0 ? 1.0 : 0.0; ok0 += t0 != 0; }
    for (long s = 0; s < N; s++) pi[s] /= ok0;
    double rate = 0, prev = -1; long it;
    for (it = 0; it < maxit; it++) {
        for (long s = 0; s < N; s++) nw[s] = 0.5 * pi[s];
        for (long s = 0; s < N; s++) { double w = 0.5 * pi[s]; if (w == 0) continue; nw[nx0[s]] += w * (1 - p); nw[nx1[s]] += w * p; }
        double diff = 0; for (long s = 0; s < N; s++) { diff += fabs(nw[s] - pi[s]); pi[s] = nw[s]; }
        if (it % 50 == 0 || diff < tol) {
            rate = 0; for (long s = 0; s < N; s++) rate += pi[s] * ((1 - p) * (ov[s] & 1) + p * ((ov[s] >> 1) & 1));
            if (diff < tol) break;
            prev = rate;
        }
    }
    (void)prev;
    double lead[256] = {0}, live = 0;
    for (long s = 0; s < N; s++) { lead[(s / topmul) % B] += pi[s]; live += pi[s] > 1e-15; }
    printf("base %d, k=%d, p=%.3f: overflow rate = %.10f   (%ld states, %.0f with positive weight, %ld iterations)\n", B, k, p, rate, N, live, it);
    printf("   leading digit:"); for (int d = 1; d < B && d < 10; d++) printf(" %d:%.4f", d, lead[d]); printf("\n");
    return 0;
}
