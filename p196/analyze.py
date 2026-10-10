# analyze.py — 196の軌道の窓統計 W(j) と、ランダム初項の実測停止確率 q(m) から
#               「1対あたりの減衰率 λ」を推定して突き合わせる
import csv, glob, math, sys
import numpy as np

run_dir = sys.argv[1] if len(sys.argv) > 1 else "run1e6"

# ---------- (1) 196 の軌道: j 個連続して「良い対（列和<=9）」が並ぶ確率 W(j) ----------
rows = list(csv.DictReader(open(f"{run_dir}/runlen_hist.csv")))
W = {}
for st in ("0", "1"):
    R = [r for r in rows if r["state"] == st]
    it, pairs = float(R[0]["iters"]), float(R[0]["pairs"])
    h = {int(r["L"]): int(r["count"]) for r in R}
    Ws = {}
    for j in range(1, 61):
        num = sum(c * max(0, L - j + 1) for L, c in h.items())
        nruns = sum(c for L, c in h.items() if L >= j)        # 独立な事象数の目安
        den = pairs - (j - 1) * it
        if num > 0:
            Ws[j] = (num / den, nruns)
    W[int(st)] = Ws

def fit_rate(xs, ys, ws):
    xs, ys, ws = map(np.asarray, (xs, ys, ws))
    A = np.vstack([np.ones_like(xs), xs]).T * np.sqrt(ws)[:, None]
    b = ys * np.sqrt(ws)
    coef, *_ = np.linalg.lstsq(A, b, rcond=None)
    # 標準誤差（重み付き最小二乗）
    res = b - A @ coef
    dof = max(1, len(xs) - 2)
    cov = np.linalg.inv(A.T @ A) * (res @ res) / dof
    return coef, np.sqrt(np.diag(cov))

lam196 = {}
print("== 196 trajectory (interior windows) ==")
for st in (0, 1):
    js = [j for j, (w, nr) in W[st].items() if j >= 8 and nr >= 30]
    coef, se = fit_rate(js, [math.log(W[st][j][0]) for j in js], [W[st][j][1] for j in js])
    lam, lam_se = math.exp(coef[1]), math.exp(coef[1]) * se[1]
    lam196[st] = (lam, lam_se)
    print(f" state {st}: fit j={min(js)}..{max(js)}  lambda = {lam:.4f} ± {lam_se:.4f}  "
          f"(marginal p = {W[st][1][0]:.4f})")
    for j in (1, 5, 10, 15, 20, 25, 30, 35, 40):
        if j in W[st]:
            print(f"    W({j:2d}) = {W[st][j][0]:.3e}   [runs>=j: {W[st][j][1]}]")

# ---------- (2) ランダム初項: 桁数 d の項から次で回文（型A）になる確率 q ----------
agg = {}
for fn in glob.glob("hazard_*.csv"):
    for r in csv.DictReader(open(fn)):
        k = (int(r["d"]), int(r["state"]))
        s = agg.setdefault(k, [0.0, 0.0, 0.0])
        s[0] += float(r["steps"]); s[1] += float(r["eventsA"]); s[2] += float(r["eventsB"])
d0 = int(sys.argv[2]) if len(sys.argv) > 2 else 15
print("\n== random seeds: per-step palindrome probability ==")
lamseed = {}
for st in (0, 1):
    for par in (0, 1):
        ds = [d for (d, s) in agg if s == st and d % 2 == par and d >= d0 + 5 and agg[(d, s)][1] >= 10]
        if len(ds) < 3:
            continue
        ms = [d // 2 for d in ds]
        coef, se = fit_rate(ms, [math.log(agg[(d, st)][1] / agg[(d, st)][0]) for d in ds],
                            [agg[(d, st)][1] for d in ds])
        lam, lam_se = math.exp(coef[1]), math.exp(coef[1]) * se[1]
        lamseed[(st, par)] = (lam, lam_se, ds)
        print(f" state {st}, d {'even' if par == 0 else 'odd '}: d={min(ds)}..{max(ds)}  lambda = {lam:.4f} ± {lam_se:.4f}")
print("\n d  | state1: steps   eventsA  q          | state0: steps   eventsA  q         | typeB events")
for d in range(d0 + 1, 70):
    a1 = agg.get((d, 1)); a0 = agg.get((d, 0))
    if not a1 or not a0:
        continue
    print(f" {d:2d} | {a1[0]:12.0f} {a1[1]:8.0f} {a1[1]/a1[0]:.3e} | {a0[0]:12.0f} {a0[1]:8.0f} {a0[1]/a0[0]:.3e} | {a1[2]+a0[2]:.0f}")

# ---------- (3) 外挿 ----------
lam1 = lam196[1][0]
print("\n== heuristic extrapolation for 196 ==")
for D in (10**6, 10**9):
    log10P = (D / 2) * math.log10(lam1) + math.log10(1 / (1 - math.sqrt(lam1)))
    print(f" beyond {D:>13,d} digits: P(ever palindrome) ~ 10^({log10P:,.0f})")
