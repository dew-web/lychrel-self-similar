# 緩和の実験（1回ごと）：一様乱数の数から k = 1..K 回の逆順加算をしたあと、
# 桁が増えた直後の項だけで「良い対の連続」を数え、減衰率 λ を推定する。
#   例: python3 carries/relax_perstep.py --bases 8,16,32 --n0 2500 --d0 3000 --k 16 --jobs 3
import argparse, os, time
import numpy as np
from multiprocessing import Pool
from relaxlib import step, lam_from

def run(args):
    B, n0, d0, K, seed = args
    rng = np.random.default_rng(seed + B)
    H = {k: [np.zeros(4 * d0, np.int64), 0, 0] for k in range(1, K + 1)}
    t0 = time.time()
    for r in range(n0):
        d = rng.integers(0, B, d0); d[-1] = rng.integers(1, B)
        for t in range(1, K + 1):
            d, ov = step(d, B)
            if not ov: continue
            m = len(d) // 2; good = ((d + d[::-1])[5:m - 5] <= B - 1).astype(np.int8)
            p = np.concatenate(([0], good, [0])); df = np.diff(p)
            st = np.where(df == 1)[0]; en = np.where(df == -1)[0]
            np.add.at(H[t][0], en - st, 1); H[t][1] += len(good); H[t][2] += 1
    lams = [lam_from(*H[k]) for k in range(1, K + 1)]
    return B, lams, [H[k][2] for k in range(1, K + 1)], time.time() - t0

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--bases", default="16")
    ap.add_argument("--n0", type=int, default=2500, help="乱数の初期値の個数")
    ap.add_argument("--d0", type=int, default=3000, help="初期値の桁数")
    ap.add_argument("--k", type=int, default=16, help="逆順加算の回数")
    ap.add_argument("--seed", type=int, default=5000, help="乱数の種（基数を足して使う）")
    ap.add_argument("--jobs", type=int, default=1, help="並列に動かす基数の数")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "results"))
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    tasks = [(int(b), a.n0, a.d0, a.k, a.seed) for b in a.bases.split(",")]
    with Pool(max(1, a.jobs)) as pool:
        for B, lams, nums, sec in pool.imap_unordered(run, tasks):
            line = f"B={B}: " + " ".join(f"{v:.3f}" for v in lams)
            print(line, flush=True)
            with open(os.path.join(a.out, f"perstep_B{B}.txt"), "w") as f:
                f.write(f"# relax_perstep: n0={a.n0} d0={a.d0} k=1..{a.k} seed={a.seed}+B ({sec:.0f}s)\n")
                f.write(f"# lambda after step k (k=1..{a.k}), estimated from j=4..16\n{line}\n")
                f.write("# state-1 numbers used per step: " + " ".join(map(str, nums)) + "\n")
