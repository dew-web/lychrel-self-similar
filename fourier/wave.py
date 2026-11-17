# wave.py — 逆順加算を「波（数字列の信号）」として見る実験
import numpy as np, math
rng = np.random.default_rng(1)

def to_digits(x, B):
    d = []
    while x: d.append(x % B); x //= B
    return np.array(d, dtype=np.int64)            # 下位桁から

def step(d, B):
    """逆順加算1回（numpy、繰り上がりは G/P の伝播で一括計算）。戻り値: 新しい数字列, 各桁への繰り上がり, 桁あふれ"""
    s = d + d[::-1]
    G = s >= B
    idx = np.where(s != B - 1, np.arange(len(s)), -1)
    last = np.maximum.accumulate(idx)
    cout = np.where(last >= 0, G[np.maximum(last, 0)], False).astype(np.int64)   # 位置 i から出る繰り上がり
    cin = np.concatenate(([0], cout[:-1]))
    y = (s + cin) % B
    if cout[-1]: y = np.append(y, 1)
    return y, cin, cout, s, bool(cout[-1])

# ---- 1. 周波数領域での恒等式 ----
if __name__ == "__main__":
    B, D = 10, 64
    a = rng.integers(0, B, D).astype(float)
    k = np.arange(D)
    A = np.fft.fft(a)
    err_rev = np.max(np.abs(np.fft.fft(a[::-1]) - np.exp(-2j*np.pi*k*(D-1)/D) * np.conj(A)))
    s = a + a[::-1]
    imag_sym = np.max(np.abs((np.fft.fft(s) * np.exp(1j*np.pi*k*(D-1)/D)).imag))
    print(f"[1] 反転 = 位相因子 x 複素共役: 誤差 {err_rev:.1e} / 繰り上がりなしの和は位相をそろえると純実数: 虚部の最大 {imag_sym:.1e}")
    # 繰り上がりありの実際の1ステップで、非対称部分が「繰り上がり項 u_i = cin_i - B*cout_i」だけから来ることを確認
    ok = 0; tot = 0
    for t in range(2000):
        d = rng.integers(0, B, 40); d[-1] = rng.integers(1, B)
        y, cin, cout, s_, ov = step(d, B)
        if ov: continue
        u = cin - B * cout
        tot += 1; ok += np.array_equal(y - y[::-1], u - u[::-1])
    print(f"    実際の逆順加算（桁あふれなし {tot} 例）で「非対称部分 = 繰り上がり項の非対称部分」: {ok}/{tot}")

    # ---- 2. 定在波からのずれ（反対称エネルギーの割合）A = |奇部分|^2 / |平均を引いた信号|^2 ----
    def asym_ratio(d):
        x = d - d.mean(); o = (x - x[::-1]) / 2
        return float((o @ o) / (x @ x)) if x @ x > 0 else 0.0
    def traj(x0, B, n):
        d = to_digits(x0, B); out = []; st = []
        prev_ov = True
        for i in range(n + 1):
            out.append(asym_ratio(d)); st.append(prev_ov)
            if np.array_equal(d, d[::-1]) and i > 0: break
            d, _, _, _, ov = step(d, B); prev_ov = ov
        return out, st
    r89, _ = traj(89, 10, 30)
    r196, st196 = traj(196, 10, 3000)
    print(f"[2] 89: A = " + " ".join(f"{v:.2f}" for v in r89) + f"  (ステップ {len(r89)-1} で回文、A=0)")
    a196 = np.array(r196[1:]); s196 = np.array(st196[1:])
    print(f"    196 (3000ステップ): 桁が増えた直後 A の平均 {a196[s196].mean():.3f} (最小 {a196[s196].min():.3f}), 増えなかった直後 平均 {a196[~s196].mean():.3f} (最小 {a196[~s196][100:].min():.4f})")
    print("    196 最初の30ステップ: " + " ".join(f"{v:.2f}" for v in r196[:31]))
    np.save("r89.npy", np.array(r89)); np.save("r196.npy", np.array(r196[:31]))

    # ---- 3. 回文確率の減衰率 = 転送作用素の最大固有値（固有ベクトルは余弦波） ----
    print("[3] 簡略モデル T_ab = [a+b <= B-1]（隣り合う対が数字を共有）の最大固有値/B と、その固有ベクトルの形")
    for Bm in (9, 10, 12, 16, 20, 100, 1000):
        T = np.array([[1.0 if a_ + b_ <= Bm - 1 else 0.0 for b_ in range(Bm)] for a_ in range(Bm)])
        w, V = np.linalg.eig(T); i = np.argmax(w.real); lam = w[i].real / Bm
        v = np.abs(V[:, i].real); v /= v.max()
        cosv = np.cos(np.pi * (np.arange(Bm) + 0.5) / (2 * Bm + 1)); cosv /= cosv.max()
        formula = 1 / (2 * Bm * math.sin(math.pi / (2 * (2 * Bm + 1))))
        print(f"    B={Bm:4d}: λ_model = {lam:.5f}  (公式 1/(2B sin(π/(2(2B+1)))) = {formula:.5f}),  固有ベクトルと cos(π(a+1/2)/(2B+1)) の最大差 {np.max(np.abs(v - cosv)):.1e}")
    print(f"    B→∞ の極限: 2/π = {2/math.pi:.5f}（固有関数は四分の一周期の余弦波 cos(πu/2)）")
