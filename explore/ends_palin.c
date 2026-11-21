// ends_palin.c — 回文になるための条件が、数の両端と真ん中でどれだけ満たされやすいかを、本物の軌道で比べる。
//
// 次の1ステップで回文になる（補題の型A、繰り上がりなし）には、すべての鏡の対 (a_j, a_{D-1-j}) の和が B-1 以下でなければならない。
// 各ステップで、次の2つの「良い対の連なり」の長さを数える:
//   L_end: 最も外側の対（j = 0）から内側へ、良い対が何個続くか
//   L_mid: 真ん中寄りの対（j = D/4）から内側へ、良い対が何個続くか（比較用）
// E(j) = P(L_end >= j)、M(j) = P(L_mid >= j) を、直前のステップで桁が増えたか（O）増えなかったか（N）に分けて出す。
// 型B（すべての列和が 0 か B+1 で、最も外側が B+1）についても、外側から何対その形が続くかを数える。
//
// usage: ends_palin B start steps [burn=3000]      start: "196"（10進のみ）か "rand:桁数:シード"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "window.h"

#define JM 64

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: ends_palin B start steps [burn]\n"); return 2; }
    int B = atoi(argv[1]); const char *start = argv[2]; long steps = atol(argv[3]); long burn = argc > 4 ? atol(argv[4]) : 3000;
    if (B < 2 || B > 127) { fprintf(stderr, "B must be 2..127\n"); return 2; }
    long d0; uint64_t seed = 1;
    if (!strncmp(start, "rand:", 5)) { d0 = atol(start + 5); const char *q = strchr(start + 5, ':'); if (q) seed = strtoull(q + 1, 0, 10); }
    else if (B == 10) d0 = (long)strlen(start); else { fprintf(stderr, "decimal start needs B=10\n"); return 2; }
    uint8_t *a = calloc((size_t)(d0 + burn + steps + 16), 1); long D = d0;
    rng_state = seed * 0x9E3779B97F4A7C15ull + 777;
    if (!strncmp(start, "rand:", 5)) { for (long i = 0; i < d0; i++) a[i] = (uint8_t)(rng64() % B); if (!a[d0 - 1]) a[d0 - 1] = 1; }
    else for (long i = 0; i < d0; i++) a[i] = (uint8_t)(start[d0 - 1 - i] - '0');
    static double hend[2][JM + 2], hmid[2][JM + 2], hB[2][JM + 2], nst[2];
    int prevO = 1;
    for (long t = 0; t < burn + steps; t++) {
        if (t >= burn && D >= 4 * JM + 8) {
            int st = prevO;
            int L = 0; while (L < JM + 1 && a[L] + a[D - 1 - L] <= B - 1) L++;
            int Lm = 0; long p = D / 4; while (Lm < JM + 1 && a[p + Lm] + a[D - 1 - p - Lm] <= B - 1) Lm++;
            int LB = 0;
            if (a[0] + a[D - 1] == B + 1) { LB = 1; while (LB < JM + 1) { int s = a[LB] + a[D - 1 - LB]; if (s == 0 || s == B + 1) LB++; else break; } }
            hend[st][L]++; hmid[st][Lm]++; hB[st][LB]++; nst[st]++;
        }
        long h = D / 2;
        for (long i = 0; i < h; i++) { uint8_t s = (uint8_t)(a[i] + a[D - 1 - i]); a[i] = s; a[D - 1 - i] = s; }
        if (D & 1) a[h] = (uint8_t)(2 * a[h]);
        int c = 0;
        for (long i = 0; i < D; i++) { int v = a[i] + c; c = v >= B; a[i] = (uint8_t)(v - (c ? B : 0)); }
        if (c) a[D++] = 1;
        prevO = c;
    }
    printf("# ends_palin: base %d, start %s, %ld steps after %ld burn-in, final %ld digits\n", B, start, steps, burn, D);
    printf("# E(j) = P(outermost j pairs all have sum <= B-1), M(j) = same for j pairs starting at pair D/4, typeB(j) = P(outermost j pairs in {0,B+1}, outer = B+1)\n");
    for (int st = 1; st >= 0; st--) {
        printf("## after %s step (%.0f samples)\n", st ? "a digit-gaining (O)" : "a non-gaining (N)", nst[st]);
        printf("# j   E(j)        M(j)        E/M      typeB(j)\n");
        double ce = nst[st], cm = nst[st], cb = nst[st];
        for (int j = 0; j <= 40; j++) {
            if (j > 0) { ce -= hend[st][j - 1]; cm -= hmid[st][j - 1]; cb -= hB[st][j - 1]; }
            if (j == 0) continue;
            if (j <= 12 || j % 4 == 0)
                printf("%3d  %.4e  %.4e  %7.3f  %.4e\n", j, ce / nst[st], cm / nst[st], cm > 0 ? ce / cm : NAN, cb / nst[st]);
        }
    }
    return 0;
}
