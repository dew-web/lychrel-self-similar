// window.h — 両端の窓モデルの1ステップ（window.c と orbit_record.c で共有）
//
// 数 x（D 桁、a_0 が最下位）の両端 k 桁だけを持つ:
//   bot[j] = a_j            （下の窓。j = 0 が最も外側 = 一の位）
//   top[j] = a_{D-1-j}      （上の窓。j = 0 が最も外側 = 最上位の数字）
// 鏡の位置どうし（j と D-1-j）が同じ添字 j になるので、列和は s_j = bot[j] + top[j]。
//
// 逆順加算 x + rev(x) の1ステップで:
//   - 下の窓は、一の位から上へ繰り上がりを流すだけで決まる（外からの入力はない）
//   - 上の窓は、真ん中から入る繰り上がり m（位置 D-k への繰り上がり）が1ビットだけ必要
//   - 最上位から繰り上がりが出たら桁が1つ増える（桁あふれ）。このとき上の窓は1つ内側へずれ、
//     最も外側に新しい 1 が入る（新しい長さ D+1 での鏡の位置 j と D-j が、また同じ添字 j になる）
// m に本物の値を与えれば、この更新は本物の両端の数字と厳密に一致する（orbit_record.c で検査する）。
#ifndef WINDOW_H
#define WINDOW_H
#include <stdint.h>
#include <string.h>

#define KMAX 64

// 1ステップ。戻り値 = 桁あふれ（1 なら桁が1つ増えた）。cb_out があれば下の窓から真ん中へ出る繰り上がりを入れる
static inline int wstep(int B, int k, uint8_t *bot, uint8_t *top, int m, int *cb_out) {
    uint8_t nb[KMAX], nt[KMAX];
    int c = 0;
    for (int j = 0; j < k; j++) { int v = bot[j] + top[j] + c; c = v >= B; nb[j] = (uint8_t)(v - (c ? B : 0)); }
    if (cb_out) *cb_out = c;
    c = m;
    for (int j = k - 1; j >= 0; j--) { int v = bot[j] + top[j] + c; c = v >= B; nt[j] = (uint8_t)(v - (c ? B : 0)); }
    memcpy(bot, nb, k);
    if (c) { top[0] = 1; memcpy(top + 1, nt, k - 1); }
    else memcpy(top, nt, k);
    return c;
}

static uint64_t rng_state = 0x9E3779B97F4A7C15ull;
static inline uint64_t rng64(void) {
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull; z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}
static inline double rng01(void) { return (rng64() >> 11) * (1.0 / 9007199254740992.0); }
#endif
