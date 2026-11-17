# verify3.py — 仕組みの手がかり：桁が増えるステップで、共有される数字の「2つのコピー」が同じになる割合 F
#   子の対 i と対 i+1 は親の列和 s_i を共有するが、実際の数字は (s_i + c_i) mod B と (s_i + c_{D-1-i}) mod B。
#   c_i と c_{D-1-i}（左右の繰り上がり）が等しければ完全に共有（繰り上がりなしモデル）、違えば ±1 のずれ（ディザ）。
import numpy as np, math
from wave import step
def corr1(g):
    g = g.astype(float); g -= g.mean()
    return float((g[:-1] * g[1:]).mean() / (g * g).mean())
print("base | F (trajectory) | F (one step from random) | lag-1 corr of good pairs: trajectory / random")
for B in (9, 10, 15, 16, 17, 31, 32, 33, 63, 64, 65):
    rng = np.random.default_rng(5 * B)
    d = rng.integers(0, B, 40); d[-1] = rng.integers(1, B)
    Ft = []; Ct = []
    for t in range(6000):
        y, cin, cout, s, ov = step(d, B)
        if ov and t > 300:
            D = len(d); m = D // 2
            i = np.arange(5, m - 5)
            Ft.append(np.mean(cin[i] == cin[D - 1 - i]))
            y2, _, _, s2, _ = step(y, B); m2 = len(y) // 2
            Ct.append(corr1(s2[5:m2 - 5] <= B - 1))
        d = y
    Fr = []; Cr = []
    for k in range(200):
        x = rng.integers(0, B, 2000); x[-1] = rng.integers(1, B)
        y, cin, cout, s, ov = step(x, B)
        if not ov: continue
        D = len(x); m = D // 2; i = np.arange(5, m - 5)
        Fr.append(np.mean(cin[i] == cin[D - 1 - i]))
        y2, _, _, s2, _ = step(y, B); m2 = len(y) // 2
        Cr.append(corr1(s2[5:m2 - 5] <= B - 1))
    print(f"{B:3d} | {np.mean(Ft):.4f} | {np.mean(Fr):.4f} | {np.mean(Ct):+.4f} / {np.mean(Cr):+.4f}")
