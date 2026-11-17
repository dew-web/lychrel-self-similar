# 実際の軌道で「桁が増えた直後」の、繰り上がりなしの対が j 個続く確率 W(j) を測り、減衰率 λ を推定
import numpy as np, math, sys
from wave import step
rng = np.random.default_rng(7)
def measure(B, N=6000, d0=40, edge=5):
    d = rng.integers(0, B, d0); d[-1] = rng.integers(1, B)
    hist = np.zeros(4000, np.int64); pairs = 0; seqs = 0; prev_ov = False
    for t in range(N):
        y, cin, cout, s, ov = step(d, B)
        if prev_ov and t > 50:                                  # 桁が増えた直後の項（状態1）
            m = len(d) // 2
            g = (s[edge:m - edge] <= B - 1).astype(np.int8)
            if len(g) > 40:
                p = np.concatenate(([0], g, [0])); dif = np.diff(p)
                st = np.where(dif == 1)[0]; en = np.where(dif == -1)[0]
                np.add.at(hist, np.minimum(en - st, 3999), 1)
                pairs += len(g); seqs += 1
        d = y; prev_ov = ov
    L = np.arange(4000)
    W = {j: (hist * np.maximum(0, L - j + 1)).sum() / (pairs - (j - 1) * seqs) for j in range(1, 25)}
    js = np.arange(4, 17); lw = np.log([W[j] for j in js])
    lam = math.exp(np.polyfit(js, lw, 1)[0])
    return lam, W[1], len(d)
if __name__ == "__main__":
  print("B  | 実測 λ(桁増加直後) | 簡略モデル 1/(2B sin(π/(2(2B+1)))) | 2/π | 良い対の割合 | 最終桁数")
  for B in (9, 10, 12, 16, 20, 30):
      lam, p1, D = measure(B)
      model = 1 / (2 * B * math.sin(math.pi / (2 * (2 * B + 1))))
      print(f"{B:2d} | {lam:.4f} | {model:.4f} | {2/math.pi:.4f} | {p1:.4f} | {D}")
      sys.stdout.flush()
