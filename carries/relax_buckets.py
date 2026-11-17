# 緩和の実験（区間ごと）：k 回目の逆順加算の直後を、1, 2, 3-4, 5-8, …, 129-256 の区間にまとめて λ を推定する。
#   例: python3 carries/relax_buckets.py --bases 10,15,16,17 --n0 1200 --d0 3000 --jobs 4
import argparse, os, time
import numpy as np
from multiprocessing import Pool
from relaxlib import step, lam_from
BUCKETS = [(1, 1), (2, 2), (3, 4), (5, 8), (9, 16), (17, 32), (33, 64), (65, 128), (129, 256)]

def run(args):
    B, n0, d0, seed = args
    rng = np.random.default_rng(seed + B)
    H = {b: [np.zeros(4 * d0, np.int64), 0, 0] for b in BUCKETS}
    t0 = time.time()
    for r in range(n0):
        d = rng.integers(0, B, d0); d[-1] = rng.integers(1, B)
        for t in range(1, 257):
            d, ov = step(d, B)
            if not ov: continue
            b = next(bb for bb in BUCKETS if bb[0] <= t <= bb[1])
            m = len(d) // 2; good = ((d + d[::-1])[5:m - 5] <= B - 1).astype(np.int8)
            p = np.concatenate(([0], good, [0])); df = np.diff(p)
            st = np.where(df == 1)[0]; en = np.where(df == -1)[0]
            np.add.at(H[b][0], en - st, 1); H[b][1] += len(good); H[b][2] += 1
    out = [f"B={B} N0={n0} D0={d0} seed={seed}+B ({time.time()-t0:.0f}s)"]
    for b in BUCKETS:
        h, pairs, seqs = H[b]
        out.append(f"  steps {b[0]:>3}-{b[1]:<3}: lambda={lam_from(h, pairs, seqs):.4f}  (state-1 numbers {seqs}, pairs {pairs})")
    return B, "\n".join(out)

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--bases", default="10,15,16,17")
    ap.add_argument("--n0", type=int, default=1200)
    ap.add_argument("--d0", type=int, default=3000)
    ap.add_argument("--seed", type=int, default=1000)
    ap.add_argument("--jobs", type=int, default=1)
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "results"))
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    with Pool(max(1, a.jobs)) as pool:
        for B, text in pool.imap_unordered(run, [(int(b), a.n0, a.d0, a.seed) for b in a.bases.split(",")]):
            print(text, flush=True)
            open(os.path.join(a.out, f"buckets_B{B}.txt"), "w").write(text + "\n")
