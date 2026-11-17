# 逆順加算の「良い対」の確率を、繰り上がりまで含めて正確に計算する転送作用素
#  親の数字が一様乱数のとき、列和 s_k は独立（三角分布）。
#  下半分の繰り上がり L は外側→内側へ、上半分の繰り上がり H は内側→外側へ流れる（向きが逆の2本の Holte 連鎖）。
#  状態（位置 k）: (s_k, L_k, H_{k+1})。子の対 k の数字は ((s_k+L_k) mod B, (s_{k-1}+H_k) mod B)。
import numpy as np, sys, argparse
def holte(B, n):
    # n 個の一様乱数を足すときの繰り上がりの推移行列（Holte の行列）を数え上げで作る
    from itertools import product
    P = np.zeros((n, n))
    for i in range(n):
        cnt = np.zeros(n)
        for digs in product(range(B), repeat=n):
            cnt[(i + sum(digs)) // B] += 1
        P[i] = cnt / B**n
    return P
def operator(B, carries=True):
    S = np.arange(2 * B - 1)
    p = np.where(S <= B - 1, S + 1, 2 * B - 1 - S) / B**2
    f = lambda s, c: 1 if s + c >= B else 0
    st = [(s, L, H) for s in S for L in (0, 1) for H in (0, 1)] if carries else [(s, 0, 0) for s in S]
    idx = {x: i for i, x in enumerate(st)}
    Mall = np.zeros((len(st), len(st))); Mgood = np.zeros_like(Mall)
    for (s1, L1, H1) in st:
        for (s2, L2, H2) in st:
            if carries and (L2 != f(s1, L1) or H1 != f(s2, H2)): continue
            w = p[s2]
            Mall[idx[(s1, L1, H1)], idx[(s2, L2, H2)]] = w
            if ((s2 + L2) % B + (s1 + H1) % B) <= B - 1:
                Mgood[idx[(s1, L1, H1)], idx[(s2, L2, H2)]] = w
    return Mall, Mgood
def perron(M):
    w, V = np.linalg.eig(M); i = np.argmax(w.real); return w[i].real
def W_curve(B, carries=True, jmax=20):
    Mall, Mg = operator(B, carries)
    wl, Vl = np.linalg.eig(Mall.T); a = np.abs(Vl[:, np.argmax(wl.real)].real)
    wr, Vr = np.linalg.eig(Mall); b = np.abs(Vr[:, np.argmax(wr.real)].real)
    a /= a @ b
    W = []; v = b.copy()
    for j in range(1, jmax + 1):
        v = Mg @ v; W.append(a @ v)
    return np.array(W), perron(Mall), perron(Mg)
if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="繰り上がり込みの厳密な転送作用素で、良い対の減衰率 λ を計算する")
    ap.add_argument("--bases", default="3,4,5,6,7,8,9,10,12,15,16,17,24,31,32,33,63,64,65")
    ap.add_argument("--quick", action="store_true", help="動作確認: 10進と16進だけ計算し、既知の値と比べる")
    a = ap.parse_args()
    if a.quick:
        ok = True
        for B, v in {10: 0.6565, 16: 0.6491}.items():
            _, _, l1 = W_curve(B, carries=True)
            print(f"B={B}: lambda={l1:.4f} (expected {v})"); ok &= abs(l1 - v) < 5e-4
        sys.exit(0 if ok else 1)
    np.set_printoptions(precision=4, suppress=True)
    print("== Holte の行列（10進、2個の和）==\n", holte(10, 2), " 固有値", np.sort(np.linalg.eigvals(holte(10, 2)).real)[::-1])
    print("== Holte の行列（10進、3個の和）==\n", holte(10, 3), " 固有値", np.sort(np.linalg.eigvals(holte(10, 3)).real)[::-1])
    print("\n B | 全体の最大固有値 | 繰り上がり無視 λ | 繰り上がり込み λ（漸近） | 同・j=4..16の傾き")
    js = np.arange(4, 17)
    for B in [int(b) for b in a.bases.split(",")]:
        W0, _, l0 = W_curve(B, carries=False)
        W1, top, l1 = W_curve(B, carries=True)
        fit = np.exp(np.polyfit(js, np.log(W1[js - 1]), 1)[0])
        print(f"{B:3d} | {top:.6f} | {l0:.4f} | {l1:.4f} | {fit:.4f}")
