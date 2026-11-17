# verify2.py — 2/π 則と「2の冪だけ例外」の検証
#  E1: 多くの基数で減衰率 λ（桁が増えた直後）を測る（各3本の軌道）
#  E2: 推定方法を変えても結論が変わらないか（j の範囲・端の除外）
#  E3: 一様乱数の数から1回だけ逆順加算した数で λ を測る（軌道の履歴の影響を切り分け）
#  E4: 軌道上の数の数字の分布（一様か）
import numpy as np, math, sys, time
from wave import step
out = open("verify2.txt", "w")
def P(*a):
    out.write(" ".join(str(x) for x in a) + "\n"); out.flush()
model = lambda B: 1 / (2 * B * math.sin(math.pi / (2 * (2 * B + 1))))
def runs_hist(g, hist):
    p = np.concatenate(([0], g.astype(np.int8), [0])); dif = np.diff(p)
    st = np.where(dif == 1)[0]; en = np.where(dif == -1)[0]
    np.add.at(hist, np.minimum(en - st, len(hist) - 1), 1)
def fit(hist, pairs, seqs, j0=4, j1=16):
    L = np.arange(len(hist)); js = np.arange(j0, j1 + 1)
    W = [(hist * np.maximum(0, L - j + 1)).sum() / (pairs - (j - 1) * seqs) for j in js]
    return math.exp(np.polyfit(js, np.log(W), 1)[0])
def traj_stats(B, seed, N=6000, edge=5, digit_hist=None):
    rng = np.random.default_rng(seed)
    d = rng.integers(0, B, 40); d[-1] = rng.integers(1, B)
    hist = np.zeros(3000, np.int64); hist_far = np.zeros(3000, np.int64); pairs = seqs = pairs_far = seqs_far = 0; prev = False
    for t in range(N):
        y, cin, cout, s, ov = step(d, B)
        if prev and t > 50:
            m = len(d) // 2
            g = s[edge:m - edge] <= B - 1
            if len(g) > 40:
                runs_hist(g, hist); pairs += len(g); seqs += 1
            if m > 140:
                g2 = s[50:m - 50] <= B - 1; runs_hist(g2, hist_far); pairs_far += len(g2); seqs_far += 1
            if digit_hist is not None and t > N // 2: np.add.at(digit_hist, d, 1)
        d = y; prev = ov
    return hist, pairs, seqs, hist_far, pairs_far, seqs_far
t0 = time.time()
P("E1: base | lambda(3 seeds) mean sd | carry-free model | 2/pi=%.4f" % (2 / math.pi))
E2 = {}
for B in (3, 4, 5, 6, 7, 8, 9, 10, 12, 15, 16, 17, 24, 31, 32, 33, 63, 64, 65):
    vals = []; alt = []
    for sd in (1, 2, 3):
        h, pr, sq, hf, pf, sf = traj_stats(B, 1000 * B + sd)
        vals.append(fit(h, pr, sq))
        if B in (10, 16): alt.append((fit(h, pr, sq, 4, 12), fit(h, pr, sq, 8, 20), fit(hf, pf, sf)))
    if alt: E2[B] = alt
    P(f"{B:3d} | " + " ".join(f"{v:.4f}" for v in vals) + f" | mean {np.mean(vals):.4f} sd {np.std(vals, ddof=1):.4f} | model {model(B):.4f} | {time.time()-t0:.0f}s")
P("E2: robustness (base: j4-12, j8-20, edge50) per seed")
for B, rows in E2.items():
    for r in rows: P(f"  B={B}: " + " ".join(f"{v:.4f}" for v in r))
P("E3: one step from uniform random numbers (no history): base | lambda | model")
rng = np.random.default_rng(99)
for B in (4, 6, 8, 9, 10, 12, 16, 24, 32, 64):
    hist = np.zeros(3000, np.int64); pairs = seqs = 0; tries = 0
    while seqs < 400 and tries < 3000:
        tries += 1
        x = rng.integers(0, B, 3000); x[-1] = rng.integers(1, B)
        y, cin, cout, s, ov = step(x, B)
        if not ov: continue
        y2, _, _, s2, _ = step(y, B)            # y の列和（=次の一歩の列和）
        m = len(y) // 2
        g = s2[5:m - 5] <= B - 1
        runs_hist(g, hist); pairs += len(g); seqs += 1
    P(f"  {B:3d} | {fit(hist, pairs, seqs):.4f} | {model(B):.4f}")
P("E4: digit distribution on trajectories (state-1 numbers, 2nd half): base | max |freq-1/B|*B | most frequent digits")
for B in (8, 10, 12, 16, 32):
    dh = np.zeros(B, np.int64); traj_stats(B, 777, N=6000, digit_hist=dh)
    f = dh / dh.sum(); dev = np.max(np.abs(f - 1 / B)) * B
    top = np.argsort(-f)[:3]
    P(f"  {B:3d} | {dev:.3f} | " + ", ".join(f"{t}:{f[t]*B:.3f}" for t in top) + " (x 1/B)")
P("DONE")
