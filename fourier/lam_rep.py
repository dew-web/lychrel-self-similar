import numpy as np, math, sys, time
import lam as L
out = open("lam_rep.txt", "w")
for B in (9, 10, 12, 16, 20, 30):
    vals = []
    for seed in (101, 202, 303, 404):
        L.rng = np.random.default_rng(seed + B)
        t0 = time.time(); lam, p1, D = L.measure(B, N=6000); vals.append(lam)
    out.write(f"{B} " + " ".join(f"{v:.4f}" for v in vals) + f" mean {np.mean(vals):.4f} sd {np.std(vals, ddof=1):.4f}\n"); out.flush()
out.write("DONE\n"); out.close()
