#!/usr/bin/env python3
"""work/window/ の結果を表にする: 基数ごとに、本物の軌道の桁の増え方と、窓モデル（厳密解 k=1..4、モンテカルロ k=4..32）を並べる。
usage: window_summary.py work/window"""
import glob, math, os, re, sys

d = sys.argv[1]
def rate_of(fn, k=None):
    if not os.path.exists(fn): return None
    t = open(fn).read()
    if k is None:
        m = re.search(r'overflow rate[^=]*= ([0-9.]+)', t)
        return float(m.group(1)) if m else None
    m = re.search(r'k=\s*%d\s+overflow rate[^=]*= ([0-9.]+)' % k, t)
    return float(m.group(1)) if m else None

bases = sorted({int(re.search(r'_B(\d+)', f).group(1)) for f in glob.glob(os.path.join(d, '*_B*.txt'))})
print("base  log_B2   real(orbit)  exact k=1   k=2       k=3       k=4     | MC k=4    k=8      k=16     k=32")
for B in bases:
    real = rate_of(os.path.join(d, f'real_B{B}.txt')) if B != 10 else rate_of(os.path.join(d, 'real_196.txt'))
    ex = [rate_of(os.path.join(d, f'exact_B{B}_k{k}.txt')) for k in (1, 2, 3, 4)]
    mcf = os.path.join(d, f'mc_B{B}.txt') if B != 10 else os.path.join(d, 'mc_B10_p50.txt')
    mc = [rate_of(mcf, k) for k in (4, 8, 16, 32)]
    f = lambda v: f"{v:.5f}" if v is not None else "   -   "
    print(f"{B:<5} {math.log(2)/math.log(B):.4f}  {f(real)}     " + "  ".join(f(v) for v in ex) + "  | " + "  ".join(f(v) for v in mc))
print()
for fn in sorted(glob.glob(os.path.join(d, 'real_B10_rand*.txt'))):
    print(os.path.basename(fn), rate_of(fn))
for fn in sorted(glob.glob(os.path.join(d, 'mc_B10_p*.txt'))) + [os.path.join(d, 'mc_B10_rho.txt')]:
    if os.path.exists(fn):
        print(os.path.basename(fn), re.findall(r'k=\s*(\d+)\s+overflow rate[^=]*= ([0-9.]+)', open(fn).read()))
