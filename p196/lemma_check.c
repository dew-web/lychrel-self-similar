// lemma_check.c
// 1 <= x < LIM の全ての x について y = x + R(x) を計算し、
//   定理1(b): y が桁上がりなしで回文  <=>  全ての列和 s_i = a_i + a_{d-1-i} <= 9
//   定理1(c): y が桁が1つ増えて回文  =>   s_0 = 11
// を検証する。桁数 d ごとに理論値 45*55^(m-1)*(奇数なら5) と照合。
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv) {
    long LIM = argc > 1 ? atol(argv[1]) : 100000000L;
    long total[20] = {0}, carryfree[20] = {0}, pal_same[20] = {0}, pal_grow[20] = {0};
    long viol_b = 0, viol_c = 0, viol_B = 0;
    uint8_t a[20], s[20], y[21];
    for (long x = 1; x < LIM; x++) {
        long t = x; int d = 0;
        while (t) { a[d++] = (uint8_t)(t % 10); t /= 10; }
        total[d]++;
        int cf = 1;
        for (int i = 0; i < d; i++) { s[i] = a[i] + a[d - 1 - i]; if (s[i] > 9) cf = 0; }
        int c = 0;
        for (int i = 0; i < d; i++) { int v = s[i] + c; c = v >= 10; y[i] = (uint8_t)(v - 10 * c); }
        int dy = d; if (c) y[dy++] = 1;
        int pal = 1;
        for (int i = 0, j = dy - 1; i < j; i++, j--) if (y[i] != y[j]) { pal = 0; break; }
        if (cf) carryfree[d]++;
        if (pal && dy == d) pal_same[d]++;
        if (pal && dy == d + 1) { pal_grow[d]++; if (s[0] != 11) viol_c++; }
        if ((pal && dy == d) != cf) viol_b++;
        { int tb = (s[0] == 11);  /* 型B: 全ての s_i が {0,11}、s_0 = 11 */
          for (int i = 0; i < d; i++) if (s[i] != 0 && s[i] != 11) tb = 0;
          if ((pal && dy == d + 1) != tb) viol_B++; }
    }
    printf("d  total        carry-free   theory       pal(same len) pal(+1 digit)\n");
    for (int d = 1; d < 20; d++) {
        if (!total[d]) continue;
        long m = d / 2;
        double th;
        if (d == 1) th = 4;
        else { th = 45; for (long k = 1; k < m; k++) th *= 55; if (d & 1) th *= 5; }
        printf("%-2d %-12ld %-12ld %-12.0f %-13ld %ld\n", d, total[d], carryfree[d], th, pal_same[d], pal_grow[d]);
    }
    printf("violations of Thm1(b): %ld, Thm1(c): %ld, typeB-iff: %ld\n", viol_b, viol_c, viol_B);
    return 0;
}
