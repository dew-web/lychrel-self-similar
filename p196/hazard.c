// hazard.c — ランダムな初項から逆順加算を回し、
//   「桁数 d の項から次の1ステップで回文が出る確率」q(d) を状態別に実測する。
//   状態 0: 直前のステップで桁が増えなかった / 状態 1: 直前で桁が増えた
// usage: ./hazard <初項の個数> <初項の桁数> <打ち切り桁数> <乱数シード>
// 出力: hazard_<seed>.csv  (d,state,steps,eventsA,eventsB)
//   eventsA = 繰り上がりなし型の回文(定理1 型A), eventsB = 桁増加型の回文(型B)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAXD 128
static uint64_t rng_s;
static inline uint64_t splitmix(void) {
    uint64_t z = (rng_s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL; z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
static inline int is_pal(const uint8_t *a, int d) {
    for (int i = 0, j = d - 1; i < j; i++, j--) if (a[i] != a[j]) return 0;
    return 1;
}

int main(int argc, char **argv) {
    long N = atol(argv[1]); int d0 = atoi(argv[2]); int Dmax = atoi(argv[3]);
    rng_s = (uint64_t)atoll(argv[4]);
    static double steps[MAXD][2], evA[MAXD][2], evB[MAXD][2];
    long survived = 0;
    uint8_t a[MAXD + 2];
    for (long t = 0; t < N; t++) {
        int d;
        do {                                   // 回文でない d0 桁の一様乱数を初項にする
            d = d0;
            for (int i = 0; i < d - 1; i++) a[i] = (uint8_t)(splitmix() % 10);
            a[d - 1] = (uint8_t)(1 + splitmix() % 9);
        } while (is_pal(a, d));
        int st = 1;
        for (;;) {
            if (d >= Dmax) { survived++; break; }
            steps[d][st] += 1;
            int dprev = d, stprev = st;
            uint8_t s[MAXD];
            for (int i = 0; i < d; i++) s[i] = (uint8_t)(a[i] + a[d - 1 - i]);
            unsigned c = 0;
            for (int i = 0; i < d; i++) { unsigned v = s[i] + c; c = v >= 10; a[i] = (uint8_t)(v - 10 * c); }
            if (c) { a[d++] = 1; st = 1; } else st = 0;
            if (is_pal(a, d)) {
                if (d == dprev) evA[dprev][stprev] += 1; else evB[dprev][stprev] += 1;
                break;
            }
        }
    }
    char fn[64]; snprintf(fn, sizeof fn, "hazard_%s.csv", argv[4]);
    FILE *f = fopen(fn, "w");
    fprintf(f, "d,state,steps,eventsA,eventsB\n");
    for (int d = 1; d < MAXD; d++)
        for (int st = 0; st < 2; st++)
            if (steps[d][st] > 0) fprintf(f, "%d,%d,%.0f,%.0f,%.0f\n", d, st, steps[d][st], evA[d][st], evB[d][st]);
    fclose(f);
    printf("seeds=%ld  survived to %d digits: %ld (%.2f%%)\n", N, Dmax, survived, 100.0 * survived / N);
    return 0;
}
