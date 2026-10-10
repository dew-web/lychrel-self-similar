// p196.c  —  196問題: x_{n+1} = x_n + R(x_n) を AVX2 で高速に回し、統計を取る
//
// build : gcc -O3 -march=native -o p196 p196.c
// usage : ./p196 <最大桁数> [初項=196] [数字分布の間隔=1000] [連続長統計の間隔=20]
//         ./p196 2000000 @final_number.txt   (チェックポイントから再開)
//         ./p196 --selftest            (スカラー参照実装と突き合わせ)
//
// 1桁=1バイト、a[0] が一の位。1回の反復は
//   (1) 列和  s_i = a_i + a_{d-1-i}（対称なので左右に同じ値を書く）  … 反転シャッフル付きSIMD
//   (2) 繰り上がり伝播                                             … 64桁ずつビットマスク化し、
//       G = [s>=10]（生成）, P = [s==9]（伝播）として (G|P) + G を64bit加算すれば
//       全桁の繰り上がりが一発で求まる（2進加算器のキャリー連鎖と同じ構造）。
// 停止判定は「x_n が回文か」。定理1より、回文になるのは「全ての列和 <= 9」(型A) か
// 「全ての列和が 0 か 11」(型B) のときに限られる。
#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define RUNMAX 256

static inline __m256i rev32(__m256i x) {
    const __m256i r = _mm256_setr_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
                                        15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
    x = _mm256_shuffle_epi8(x, r);
    return _mm256_permute2x128_si256(x, x, 0x01);
}

static inline __m256i bits_to_bytes(uint32_t m) {
    const __m256i shuf = _mm256_setr_epi8(0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
                                          2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3);
    const __m256i sel = _mm256_set1_epi64x(0x8040201008040201LL);
    __m256i v = _mm256_shuffle_epi8(_mm256_set1_epi32((int)m), shuf);
    v = _mm256_cmpeq_epi8(_mm256_and_si256(v, sel), sel);
    return _mm256_and_si256(v, _mm256_set1_epi8(1));
}

typedef struct { long K, eq, adj; int midbad; uint32_t first_bad; int have_first; } colstat_t;

// (1) 列和。統計: K = 列和>=10 の対の数, eq/adj = 鏡像位置の数字が等しい/差が±1(mod10) の対の数
static void column_sums(uint8_t *a, long d, uint32_t *badbits, colstat_t *cs) {
    const long m = d / 2;
    long i = 0, K = 0, eq = 0, adj = 0, blk = 0;
    const __m256i nine = _mm256_set1_epi8(9), zero = _mm256_setzero_si256();
    const __m256i p1 = _mm256_set1_epi8(1), m1 = _mm256_set1_epi8(-1);
    const __m256i p9 = _mm256_set1_epi8(9), m9 = _mm256_set1_epi8(-9);
    cs->have_first = 0;
    for (; i + 32 <= m; i += 32, blk++) {
        __m256i lo = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i hr = rev32(_mm256_loadu_si256((const __m256i *)(a + d - 32 - i)));
        __m256i df = _mm256_sub_epi8(lo, hr);
        uint32_t e = (uint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(df, zero));
        __m256i av = _mm256_or_si256(_mm256_or_si256(_mm256_cmpeq_epi8(df, p1), _mm256_cmpeq_epi8(df, m1)),
                                     _mm256_or_si256(_mm256_cmpeq_epi8(df, p9), _mm256_cmpeq_epi8(df, m9)));
        uint32_t ad = (uint32_t)_mm256_movemask_epi8(av);
        __m256i s = _mm256_add_epi8(lo, hr);
        uint32_t bad = (uint32_t)_mm256_movemask_epi8(_mm256_cmpgt_epi8(s, nine));
        _mm256_storeu_si256((__m256i *)(a + i), s);
        _mm256_storeu_si256((__m256i *)(a + d - 32 - i), rev32(s));
        if (badbits) badbits[blk] = bad;
        if (blk == 0) { cs->first_bad = bad; cs->have_first = 1; }
        K += __builtin_popcount(bad); eq += __builtin_popcount(e); adj += __builtin_popcount(ad);
    }
    uint32_t tail = 0; int tb = 0;
    for (; i < m; i++, tb++) {
        int x = a[i], y = a[d - 1 - i], dd = (x - y + 10) % 10;
        eq += (dd == 0); adj += (dd == 1 || dd == 9);
        uint8_t s = (uint8_t)(x + y);
        a[i] = s; a[d - 1 - i] = s;
        int b = s >= 10; K += b; tail |= (uint32_t)b << tb;
    }
    if (badbits) badbits[blk] = tail;
    cs->midbad = 0;
    if (d & 1) { a[m] = (uint8_t)(2 * a[m]); cs->midbad = a[m] >= 10; }
    cs->K = K; cs->eq = eq; cs->adj = adj;
}

// (2) 繰り上がり伝播。戻り値 = 最上位からの繰り上がり（=桁が1つ増える）
static int carry_pass(uint8_t *a, long d) {
    const __m256i nine = _mm256_set1_epi8(9), ten = _mm256_set1_epi8(10);
    uint64_t cin = 0; long i = 0;
    for (; i + 64 <= d; i += 64) {
        __m256i s0 = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i s1 = _mm256_loadu_si256((const __m256i *)(a + i + 32));
        uint64_t G = (uint64_t)(uint32_t)_mm256_movemask_epi8(_mm256_cmpgt_epi8(s0, nine))
                   | ((uint64_t)(uint32_t)_mm256_movemask_epi8(_mm256_cmpgt_epi8(s1, nine)) << 32);
        uint64_t P = (uint64_t)(uint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(s0, nine))
                   | ((uint64_t)(uint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(s1, nine)) << 32);
        uint64_t A = G | P;
        unsigned __int128 sum = (unsigned __int128)A + G + cin;
        uint64_t C = (uint64_t)sum ^ A ^ G;          // 各桁への繰り上がり入力
        cin = (uint64_t)(sum >> 64);
        __m256i t0 = _mm256_add_epi8(s0, bits_to_bytes((uint32_t)C));
        __m256i t1 = _mm256_add_epi8(s1, bits_to_bytes((uint32_t)(C >> 32)));
        t0 = _mm256_sub_epi8(t0, _mm256_and_si256(_mm256_cmpgt_epi8(t0, nine), ten));
        t1 = _mm256_sub_epi8(t1, _mm256_and_si256(_mm256_cmpgt_epi8(t1, nine), ten));
        _mm256_storeu_si256((__m256i *)(a + i), t0);
        _mm256_storeu_si256((__m256i *)(a + i + 32), t1);
    }
    unsigned c = (unsigned)cin;
    for (; i < d; i++) { unsigned t = a[i] + c; c = t >= 10; a[i] = (uint8_t)(t - 10 * c); }
    return (int)c;
}

static long pal_depth(const uint8_t *a, long d) {   // 外側から一致する対の数。回文なら -1
    long i = 0, j = d - 1;
    while (i < j && a[i] == a[j]) { i++; j--; }
    return (i >= j) ? -1 : i;
}

static long step_scalar(uint8_t *b, long d) {        // 参照実装
    static uint8_t t[1 << 20];
    for (long i = 0; i < d; i++) t[i] = (uint8_t)(b[i] + b[d - 1 - i]);
    unsigned c = 0;
    for (long i = 0; i < d; i++) { unsigned v = t[i] + c; c = v >= 10; b[i] = (uint8_t)(v - 10 * c); }
    if (c) b[d++] = 1;
    return d;
}

static int selftest(void) {
    const long N = 40000; long cap = N + 256;
    uint8_t *a = calloc(cap, 1), *b = calloc(cap, 1);
    long d = 3, db = 3; a[0] = b[0] = 6; a[1] = b[1] = 9; a[2] = b[2] = 1;
    colstat_t cs;
    for (long n = 1; n < N; n++) {
        column_sums(a, d, NULL, &cs);
        if (carry_pass(a, d)) a[d++] = 1;
        db = step_scalar(b, db);
        if (d != db || memcmp(a, b, d)) { printf("MISMATCH at n=%ld\n", n); return 1; }
        if (n <= 12) { printf("x_%ld = ", n + 1); for (long i = d - 1; i >= 0; i--) putchar('0' + a[i]); putchar('\n'); }
    }
    printf("selftest OK: SIMD == scalar for %ld iterations (%ld digits)\n", N, d);
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--selftest")) return selftest();
    long maxd = argc > 1 ? atol(argv[1]) : 100000;
    const char *seed = argc > 2 ? argv[2] : "196";
    long H = argc > 3 ? atol(argv[3]) : 1000;   // 数字分布のサンプリング間隔
    long HR = argc > 4 ? atol(argv[4]) : 20;   // 連続長統計のサンプリング間隔
    long cap = maxd + 256;
    uint8_t *a = aligned_alloc(64, (size_t)((cap + 63) / 64 * 64)); memset(a, 0, (size_t)((cap + 63) / 64 * 64));
    uint32_t *badbits = calloc((size_t)(cap / 64 + 8), sizeof(uint32_t));
    char *buf = NULL;
    if (seed[0] == '@') {                        // "@ファイル名" ならチェックポイントから再開
        FILE *cf = fopen(seed + 1, "r"); if (!cf) { perror(seed + 1); return 1; }
        buf = malloc((size_t)cap + 2); size_t k = fread(buf, 1, (size_t)cap + 1, cf); fclose(cf);
        while (k && (buf[k - 1] < '0' || buf[k - 1] > '9')) k--;
        buf[k] = 0; seed = buf;
    }
    long d = (long)strlen(seed);
    if (d >= maxd) { fprintf(stderr, "start number already has %ld >= max digits\n", d); return 1; }
    for (long i = 0; i < d; i++) a[i] = (uint8_t)(seed[d - 1 - i] - '0');

    // 状態 st: 0 = 直前のステップで桁が増えなかった（構造あり）, 1 = 直前で桁が増えた（または初項）
    double sumK[2] = {0}, sumM[2] = {0}, sumEq[2] = {0}, sumAdj[2] = {0}, nIt[2] = {0};
    static unsigned long long runh[2][RUNMAX + 1];
    double runIt[2] = {0}, runPairs[2] = {0};
    unsigned long long dig[10] = {0}; double digIt = 0;
    double posbad[2][32] = {{0}}, posn[2] = {0};
    long overflow_total = 0; int prev_over = 1;
    double minratio = 1e9; long minratio_n = 0, minratio_d = 0;
    long maxdepth = -1, maxdepth_n = 0, maxdepth_d = 0;
    long next_mark = 10;
    FILE *lg = fopen("trajectory_log.csv", "w");
    fprintf(lg, "n,digits,K,m,bad_ratio,state,overflow_frac\n");
    clock_t t0 = clock();
    colstat_t cs;
    long n;
    for (n = 1;; n++) {
        long dep = pal_depth(a, d);
        if (dep < 0) { printf("STOP: x_%ld is a palindrome (%ld digits)\n", n, d); break; }
        if (d >= 1000 && dep > maxdepth) { maxdepth = dep; maxdepth_n = n; maxdepth_d = d; }
        if (d >= next_mark) {
            printf("digits reach %-9ld at x_%-9ld (passes = %ld)  cpu=%.1fs\n", next_mark, n, n - 1,
                   (double)(clock() - t0) / CLOCKS_PER_SEC);
            fflush(stdout); next_mark *= 10;
        }
        if (d >= maxd) break;
        int st = prev_over, heavy = (n % H) == 0;
        if (heavy) {                                   // x_n の数字分布
            for (long i = 0; i < d; i++) dig[a[i]]++;
            digIt += 1;
        }
        column_sums(a, d, badbits, &cs);
        long m = d / 2;
        sumK[st] += cs.K; sumM[st] += m; sumEq[st] += cs.eq; sumAdj[st] += cs.adj; nIt[st] += 1;
        if (cs.have_first && m >= 64) { for (int k = 0; k < 32; k++) posbad[st][k] += (cs.first_bad >> k) & 1; posn[st] += 1; }
        if (d >= 1000) {
            double r = (double)cs.K / (double)m;
            if (r < minratio) { minratio = r; minratio_n = n; minratio_d = d; }
        }
        if ((n % HR) == 0 && m >= 64) {               // 良い対(列和<=9)の連続長ヒストグラム（ctzで高速化）
            long run = 0, nw = (m + 31) / 32;
            for (long w = 0; w < nw; w++) {
                uint32_t bits = badbits[w];
                int nbits = (w == nw - 1 && (m & 31)) ? (int)(m & 31) : 32;
                if (nbits < 32) bits &= (1u << nbits) - 1u;
                int pos = 0;
                while (bits) {
                    int t = __builtin_ctz(bits);
                    run += t - pos;
                    if (run) runh[st][run < RUNMAX ? run : RUNMAX]++;
                    run = 0; pos = t + 1; bits &= bits - 1;
                }
                run += nbits - pos;
            }
            if (run) runh[st][run < RUNMAX ? run : RUNMAX]++;
            runIt[st] += 1; runPairs[st] += m;
        }
        int over = carry_pass(a, d);
        if (over) { a[d++] = 1; overflow_total++; }
        if (cs.K == 0 && !cs.midbad && over) printf("BUG: carry-free but overflow at n=%ld\n", n);
        prev_over = over;
        if (n % 100 == 0)
            fprintf(lg, "%ld,%ld,%ld,%ld,%.6f,%d,%.6f\n", n, d, cs.K, m, (double)cs.K / (m ? m : 1), st,
                    (double)overflow_total / n);
    }
    fclose(lg);
    double cpu = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("\n== summary ==\nlast term x_%ld : %ld digits, passes = %ld, cpu = %.1f s\n", n, d, n - 1, cpu);
    printf("digits gained per pass (overflow freq) = %.6f\n", (double)overflow_total / (double)(n - 1));
    for (int st = 0; st < 2; st++)
        printf("state %d (%s): iters=%.0f  bad-pair frac=%.4f  mirror-equal=%.4f  mirror-adjacent=%.4f  other=%.6f\n",
               st, st == 0 ? "after no-growth step" : "after growth step", nIt[st], sumK[st] / sumM[st],
               sumEq[st] / sumM[st], sumAdj[st] / sumM[st], 1.0 - (sumEq[st] + sumAdj[st]) / sumM[st]);
    printf("min bad-pair ratio (d>=1000) = %.4f at n=%ld (d=%ld)\n", minratio, minratio_n, minratio_d);
    printf("max outer palindromic depth (d>=1000) = %ld at n=%ld (d=%ld)\n", maxdepth, maxdepth_n, maxdepth_d);
    printf("digit freq of x_n (sampled):");
    double tot = 0; for (int k = 0; k < 10; k++) tot += (double)dig[k];
    for (int k = 0; k < 10; k++) printf(" %d:%.4f", k, dig[k] / tot);
    printf("\n");
    FILE *rf = fopen("runlen_hist.csv", "w");
    fprintf(rf, "state,iters,pairs,L,count\n");
    for (int st = 0; st < 2; st++)
        for (int L = 1; L <= RUNMAX; L++)
            if (runh[st][L]) fprintf(rf, "%d,%.0f,%.0f,%d,%llu\n", st, runIt[st], runPairs[st], L, runh[st][L]);
    fclose(rf);
    FILE *pf = fopen("posbad.csv", "w");
    fprintf(pf, "pair_index,bad_frac_state0,bad_frac_state1\n");
    for (int k = 0; k < 32; k++) fprintf(pf, "%d,%.5f,%.5f\n", k, posbad[0][k] / posn[0], posbad[1][k] / posn[1]);
    fclose(pf);
    FILE *of = fopen("final_number.txt", "w");
    for (long i = d - 1; i >= 0; i--) fputc('0' + a[i], of);
    fputc('\n', of); fclose(of);
    return 0;
}
