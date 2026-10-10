// coreauto2.c — 中心コアを「外側（長い 0/ω 列）から中心へ」1層ずつ組み立てるオートマトン。
// 列の中の状態 RUN から出発するので、周期の閉条件（位相P=位相0、δ層ずれ）が最初から効き、到達集合が小さい。
// 各状態から「ここで中心に到達して閉じる」δ+1 層の閉じ込み列を試し、成功すればコアが存在する。
// 到達集合を全部調べて閉じ込みが一度も成功しなければ、長さに関係なくコアは存在しない。
//
// 位相 t の距離（中心から）: d_t = d_0 + OO[t],  OO[t+1] = OO[t] + [ov_t=1 かつ 偶数長->奇数長]
// usage: ./coreauto2 B ovbits cinbits [outerfile]
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
typedef unsigned __int128 u128;

static int B, P, CIN[64], OV[64], PAR[65], OO[65], DELTA, PAR0;
typedef struct { int lam[64], mu[64], ph[64]; int lag[8][2]; } st_t;

static u128 pack(const st_t *s) {
    u128 k = 0; int sh = 0;
    for (int t = 0; t < P; t++) k |= (u128)(s->lam[t] & 1) << sh++;
    for (int t = 0; t < P; t++) k |= (u128)(s->mu[t] & 1) << sh++;
    for (int t = 0; t < P; t++) if (OV[t]) { k |= (u128)(s->ph[t] & 31) << sh; sh += 5; }
    for (int i = 0; i < DELTA; i++) { k |= (u128)(s->lag[i][0] & 31) << sh; sh += 5; k |= (u128)(s->lag[i][1] & 31) << sh; sh += 5; }
    return k;
}
static void unpack(u128 k, st_t *s) {
    memset(s, 0, sizeof *s); int sh = 0;
    for (int t = 0; t < P; t++) s->lam[t] = (int)(k >> sh++ & 1);
    for (int t = 0; t < P; t++) s->mu[t] = (int)(k >> sh++ & 1);
    for (int t = 0; t < P; t++) if (OV[t]) { s->ph[t] = (int)(k >> sh & 31); sh += 5; }
    for (int i = 0; i < DELTA; i++) { s->lag[i][0] = (int)(k >> sh & 31); sh += 5; s->lag[i][1] = (int)(k >> sh & 31); sh += 5; }
}

typedef struct { u128 key; int32_t parent; uint8_t lo0, hi0; } entry;
static entry *E; static int64_t NE, ECAP = 1 << 24;
static int32_t *H; static uint64_t HCAP = 1ull << 25;
static int cap_hit;
static int64_t insert(u128 key, int32_t parent, int lo0, int hi0) {
    uint64_t h = (uint64_t)((key * (u128)0x9E3779B97F4A7C15ull) >> 64) & (HCAP - 1);
    h ^= (uint64_t)(key >> 64) * 0x632BE59BD9B4E019ull & (HCAP - 1);
    while (H[h] >= 0) { if (E[H[h]].key == key) return -2; h = (h + 1) & (HCAP - 1); }
    if (NE >= ECAP) { cap_hit = 1; return -1; }
    H[h] = (int32_t)NE; E[NE].key = key; E[NE].parent = parent; E[NE].lo0 = (uint8_t)lo0; E[NE].hi0 = (uint8_t)hi0;
    return NE++;
}

// ---- 通常の層（全位相が距離1以上）----
static st_t S, N; static int64_t curidx;
static int lowout[66], highout[66], ch_lo0, ch_hi0;
static void norm_dfs(int t, int lo, int hi) {
    if (cap_hit) return;
    if (t == P) {
        if (lo != S.lag[0][0] || hi != S.lag[0][1]) return;          // 位相P = δ層前の位相0
        for (int i = 0; i + 1 < DELTA; i++) { N.lag[i][0] = S.lag[i + 1][0]; N.lag[i][1] = S.lag[i + 1][1]; }
        N.lag[DELTA - 1][0] = ch_lo0; N.lag[DELTA - 1][1] = ch_hi0;
        insert(pack(&N), (int32_t)curidx, ch_lo0, ch_hi0);
        return;
    }
    int s = lo + hi, v = s + S.lam[t];
    lowout[t] = v % B; N.lam[t] = v >= B;
    for (int g = 0; g <= 1; g++) {
        if (((s + g) >= B) != S.mu[t]) continue;                    // 上側の桁から出る繰り上がりが要求どおり
        highout[t] = (s + g) % B; N.mu[t] = g;
        if (OV[t]) N.ph[t] = highout[t];
        int nlo = lowout[t], nhi = OV[t] ? S.ph[t] : highout[t];    // 桁あふれ位相は「今の下 + 1層前の上」
        norm_dfs(t + 1, nlo, nhi);
    }
}

// ---- 閉じ込み（中心に到達する δ+1 層）----
static int close_lo0, close_hi0; static int close_found = 0;
static int cl_mid0, cl_pair0[2];
static void close_layer(int j, st_t *Sx);
static st_t CS[10];   // 閉じ込みの各層の状態
static int cl_low[66], cl_high[66], cl_mid[66];
static void close_dfs(int j, int t, st_t *Sx, st_t *Nx, int in_lo, int in_hi, int in_mid, int p0lo, int p0hi) {
    if (close_found) return;
    if (t == P) {                                                    // 位相P のチェック
        if (OO[P] > j) { if (Sx->lag[0][1] == 31 || in_lo != Sx->lag[0][0] || in_hi != Sx->lag[0][1]) return; }
        else if (OO[P] == j) {
            if (PAR[0] == 0) { if (Sx->lag[0][1] != 31 || in_mid != Sx->lag[0][0]) return; }
            else { if (Sx->lag[0][1] == 31 || in_lo != Sx->lag[0][0] || in_hi != Sx->lag[0][1]) return; }
            close_found = 1; return;                                // 全位相が中心に到達して整合
        }
        for (int i = 0; i + 1 < DELTA; i++) { Nx->lag[i][0] = Sx->lag[i + 1][0]; Nx->lag[i][1] = Sx->lag[i + 1][1]; }
        Nx->lag[DELTA - 1][0] = p0lo; Nx->lag[DELTA - 1][1] = p0hi;
        st_t nn = *Nx; CS[j + 1] = nn;
        close_layer(j + 1, &CS[j + 1]);
        return;
    }
    if (OO[t] < j) {                                                 // この位相は終了済み
        // 次の位相の入力: 偶数長->奇数長 なら中央の数字 = 1層前の上の出力（中心の上）
        close_dfs(j, t + 1, Sx, Nx, -1, -1, Sx->ph[t], p0lo, p0hi);
        return;
    }
    if (OO[t] == j) {                                                // この位相は中心
        if (PAR[t] == 0) {
            int m = in_mid, v = 2 * m + Sx->lam[t];
            if ((v >= B) != Sx->mu[t]) return;
            cl_mid[t] = v % B;
            // 次の位相: 桁あふれなし -> 中央; 奇数長->偶数長 -> (中央の出力, 1層前の上)
            if (!OV[t]) close_dfs(j, t + 1, Sx, Nx, -1, -1, cl_mid[t], p0lo, p0hi);
            else close_dfs(j, t + 1, Sx, Nx, cl_mid[t], Sx->ph[t], -1, p0lo, p0hi);
        } else {
            int s = in_lo + in_hi, v = s + Sx->lam[t], c = v >= B, w = s + c;
            if ((w >= B) != Sx->mu[t]) return;
            cl_low[t] = v % B; cl_high[t] = w % B;
            Nx->ph[t] = cl_high[t];                                  // 偶数長->奇数長 の次位相は次の層でこれを中央に使う
            if (!OV[t]) close_dfs(j, t + 1, Sx, Nx, cl_low[t], cl_high[t], -1, p0lo, p0hi);
            else close_dfs(j, t + 1, Sx, Nx, cl_low[t], Sx->ph[t], -1, p0lo, p0hi);   // 次の位相は距離1（中心でない）
        }
        return;
    }
    // 通常の対
    int s = in_lo + in_hi, v = s + Sx->lam[t];
    int lowo = v % B; Nx->lam[t] = v >= B;
    for (int g = 0; g <= 1; g++) {
        if (((s + g) >= B) != Sx->mu[t]) continue;
        int higho = (s + g) % B; Nx->mu[t] = g;
        int oldph = Sx->ph[t];
        if (OV[t]) Nx->ph[t] = higho;
        close_dfs(j, t + 1, Sx, Nx, lowo, OV[t] ? oldph : higho, -1, p0lo, p0hi);
        if (close_found) return;
    }
}
static void close_layer(int j, st_t *Sx) {
    if (close_found || j > DELTA) return;
    st_t Nx; memset(&Nx, 0, sizeof Nx);
    if (j == 0) {
        if (PAR[0] == 0) for (int m = 0; m < B && !close_found; m++) { cl_mid0 = m; memset(&Nx, 0, sizeof Nx); close_dfs(0, 0, Sx, &Nx, -1, -1, m, m, 31); }
        else for (int lo = 0; lo < B && !close_found; lo++) for (int hi = 0; hi < B && !close_found; hi++) { cl_pair0[0] = lo; cl_pair0[1] = hi; memset(&Nx, 0, sizeof Nx); close_dfs(0, 0, Sx, &Nx, lo, hi, -1, lo, hi); }
    } else {
        // 位相0 は終了済み。位相0 からの入力は「中央 = 1層前の上の出力」（偶数長->奇数長のときのみ意味を持つ）
        close_dfs(j, 0, Sx, &Nx, -1, -1, -1, 0, 0);
    }
}

// ---- 検証 ----
static int window_period_ok(const int *w0, int n0) {
    static int w[8192], start[8192], y[8192]; int n = n0, pad = 40, D = 0;
    int L0 = (B - 1) * (1 - CIN[P - 1]), H0 = (B - 1) * CIN[P - 1];
    for (int i = 0; i < pad; i++) w[D++] = L0; for (int i = 0; i < n; i++) w[D++] = w0[i]; for (int i = 0; i < pad; i++) w[D++] = H0;
    int Ds = D; memcpy(start, w, sizeof(int) * D);
    for (int t = 0; t < P; t++) {
        int c = CIN[t];
        for (int j = 0; j < D; j++) { int v = w[j] + w[D - 1 - j] + c; c = v >= B; y[j] = v - (c ? B : 0); }
        if (c != 1 - CIN[t]) return 0;
        int Lp = (B - 1) * (1 - CIN[t]), Hp = (B - 1) * CIN[t];
        if (!OV[t]) memcpy(w, y, sizeof(int) * D);
        else if (D % 2 == 1) { memcpy(w, y, sizeof(int) * D); w[D++] = Hp; }
        else { if (y[0] != Lp) return 0; memcpy(w, y + 1, sizeof(int) * (D - 1)); D--; }
        while (D > Ds + 1 && w[0] == Lp && w[D - 1] == Hp) { memmove(w, w + 1, sizeof(int) * (D - 2)); D -= 2; }
        while (D < Ds - 1) { memmove(w + 1, w, sizeof(int) * D); w[0] = Lp; w[D + 1] = Hp; D += 2; }
    }
    return D == Ds && memcmp(w, start, sizeof(int) * D) == 0;
}
static int rev_add(int *x, int D) {
    static int t[1 << 16]; int c = 0;
    for (int i = 0; i < D; i++) t[i] = x[i] + x[D - 1 - i];
    for (int i = 0; i < D; i++) { int v = t[i] + c; c = v >= B; x[i] = v - (c ? B : 0); }
    if (c) x[D++] = 1;
    return D;
}
static int is_pal(const int *x, int D) { for (int i = 0, j = D - 1; i < j; i++, j--) if (x[i] != x[j]) return 0; return 1; }

int main(int argc, char **argv) {
    B = atoi(argv[1]); P = (int)strlen(argv[2]);
    for (int t = 0; t < P; t++) { OV[t] = argv[2][t] - '0'; CIN[t] = argv[3][t] - '0'; }
    int nov = 0; for (int t = 0; t < P; t++) nov += OV[t];
    E = malloc(sizeof(entry) * ECAP); H = malloc(sizeof(int32_t) * HCAP);
    for (PAR0 = 0; PAR0 <= 1; PAR0++) {
        PAR[0] = PAR0; OO[0] = 0;
        for (int t = 0; t < P; t++) { PAR[t + 1] = PAR[t] ^ OV[t]; OO[t + 1] = OO[t] + (OV[t] && PAR[t] == 1); }
        DELTA = OO[P];
        int L0 = (B - 1) * (1 - CIN[P - 1]), H0 = (B - 1) * CIN[P - 1];
        st_t R; memset(&R, 0, sizeof R);
        for (int t = 0; t < P; t++) { R.lam[t] = CIN[t]; R.mu[t] = 1 - CIN[t]; if (OV[t]) R.ph[t] = (B - 1) * CIN[t]; }
        for (int i = 0; i < DELTA; i++) { R.lag[i][0] = L0; R.lag[i][1] = H0; }
        for (uint64_t i = 0; i < HCAP; i++) H[i] = -1;
        NE = 0; cap_hit = 0;
        insert(pack(&R), -1, L0, H0);
        // 自己検査: RUN から列の対を選ぶと RUN に戻る
        unpack(E[0].key, &S); curidx = 0; memset(&N, 0, sizeof N); ch_lo0 = L0; ch_hi0 = H0; norm_dfs(0, L0, H0);
        int selfloop = (NE == 1);
        int64_t acc = -1; int acc_m = -1, acc_lo = -1, acc_hi = -1;
        for (int64_t q = 0; q < NE && !cap_hit; q++) {
            // まず、この状態から中心で閉じられるか
            st_t Sx; unpack(E[q].key, &Sx); close_found = 0; CS[0] = Sx; close_layer(0, &CS[0]);
            if (close_found) { acc = q; acc_m = cl_mid0; acc_lo = cl_pair0[0]; acc_hi = cl_pair0[1]; break; }
            close_found = 0;
            // 次の層へ
            unpack(E[q].key, &S); curidx = q;
            for (int lo = 0; lo < B; lo++) for (int hi = 0; hi < B; hi++) { memset(&N, 0, sizeof N); ch_lo0 = lo; ch_hi0 = hi; norm_dfs(0, lo, hi); }
        }
        printf("B=%d P=%d par0=%d delta=%d  RUN self-loop=%s  reachable states=%lld  ", B, P, PAR0, DELTA, selfloop ? "ok" : "NG", (long long)NE);
        if (acc < 0) { printf(cap_hit ? "CAPACITY EXCEEDED (inconclusive)\n" : "=> no periodic core of ANY length\n"); fflush(stdout); continue; }
        // 復元: RUN からの層（外側から）＋中心
        int lay[8192][2], Q = 0; int64_t v = acc;
        while (E[v].parent >= 0) { lay[Q][0] = E[v].lo0; lay[Q][1] = E[v].hi0; Q++; v = E[v].parent; }
        // lay[0] が最も内側（中心の1つ外）、lay[Q-1] が最も外側
        int w[8192], n = 0;
        for (int k = Q - 1; k >= 0; k--) w[n++] = lay[k][0];
        if (PAR0 == 0) w[n++] = acc_m; else { w[n++] = acc_lo; w[n++] = acc_hi; }
        for (int k = 0; k < Q; k++) w[n++] = lay[k][1];
        printf("=> CORE FOUND (%d layers + center, window %d digits)\n   core (low->high): ", Q, n);
        for (int j = 0; j < n; j++) printf("%d", w[j]); printf("\n");
        int okw = window_period_ok(w, n);
        printf("   independent window simulation over one period: %s\n", okw ? "PERIODIC (ok)" : "FAILED");
        if (argc > 4 && okw) {
            FILE *f = fopen(argv[4], "r"); int bb, pp, K; char ovs[64], cbs[64], bar[4]; static int OA[512], OBw[512];
            if (fscanf(f, "%d %d %d %s %s", &bb, &pp, &K, ovs, cbs) != 5) return 1;
            for (int i = 0; i < K; i++) if (fscanf(f, "%d", &OA[i]) != 1) return 1;
            if (fscanf(f, "%3s", bar) != 1) return 1;
            for (int i = 0; i < K; i++) if (fscanf(f, "%d", &OBw[i]) != 1) return 1;
            fclose(f);
            static int x[1 << 16], xe[1 << 16];
            int nn = 20, D;
            #define BUILD(arr, runs) ({ int d_ = 0; for (int i = 0; i < K; i++) arr[d_++] = OBw[i]; for (int i = 0; i < (runs); i++) arr[d_++] = L0; \
                for (int i = 0; i < n; i++) arr[d_++] = w[i]; for (int i = 0; i < (runs); i++) arr[d_++] = H0; for (int i = K - 1; i >= 0; i--) arr[d_++] = OA[i]; d_; })
            D = BUILD(x, nn); int ok = 1;
            printf("   FULL NUMBER x0 (%d digits): ", D); for (int i = D - 1; i >= 0; i--) putchar("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[x[i]]); printf("\n");
            for (int rep = 1; rep <= 6 && ok; rep++) {
                for (int t = 0; t < P; t++) { D = rev_add(x, D); if (is_pal(x, D)) { ok = 0; break; } }
                int De = BUILD(xe, nn + rep * (nov / 2));
                if (De != D || memcmp(x, xe, sizeof(int) * D)) ok = 0;
            }
            printf("   direct reverse-and-add simulation, 6 periods: %s\n", ok ? "SELF-SIMILAR, NO PALINDROME (VERIFIED)" : "FAILED");
        }
        fflush(stdout);
    }
    return 0;
}
