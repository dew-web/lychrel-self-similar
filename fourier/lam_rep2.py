import numpy as np, math
import lam as L
out = open("lam_rep2.txt", "w")
for B in (8, 15, 17, 24, 32, 6):
    vals = []
    for seed in (11, 22, 33):
        L.rng = np.random.default_rng(seed * 100 + B)
        vals.append(L.measure(B, N=6000)[0])
    model = 1 / (2 * B * math.sin(math.pi / (2 * (2 * B + 1))))
    out.write(f"{B} " + " ".join(f"{v:.4f}" for v in vals) + f" mean {np.mean(vals):.4f} sd {np.std(vals, ddof=1):.4f} model {model:.4f}\n"); out.flush()
out.write("DONE\n"); out.close()
