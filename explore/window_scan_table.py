#!/usr/bin/env python3
"""results/2026-11/window/scan_B3-256.txt（窓のモデル、k=8 と 12 の平均）を、基数の 2 進の性質ごとにまとめる。
usage: window_scan_table.py scan_B3-256.txt"""
import re, sys
rate = {}
for line in open(sys.argv[1]):
    m = re.match(r'B=(\d+) k=\s*(\d+)\s+overflow rate[^=]*= ([0-9.]+)', line)
    if m: rate.setdefault(int(m.group(1)), []).append(float(m.group(3)))
rate = {B: sum(v) / len(v) for B, v in rate.items()}
def v2(n):
    c = 0
    while n > 0 and n % 2 == 0: n //= 2; c += 1
    return c
def group(title, key, cond):
    print(title)
    g = {}
    for B, r in rate.items():
        if cond(B): g.setdefault(key(B), []).append(r)
    for k in sorted(g):
        xs = g[k]; print(f"  {k}: n={len(xs):3d}  mean={sum(xs)/len(xs):.4f}  min={min(xs):.4f}  max={max(xs):.4f}")
print("overflow rate (digits per step) of the window model, bases 3..256; groups use bases >= 17")
group("odd B, by how many times 2 divides B-1:", lambda B: v2(B - 1), lambda B: B >= 17 and B % 2 == 1)
group("even B, by how many times 2 divides B-2:", lambda B: v2(B - 2), lambda B: B >= 18 and B % 2 == 0)
group("B mod 8:", lambda B: B % 8, lambda B: B >= 17)
print("fastest 15:", ", ".join(f"{B}:{r:.4f}" for B, r in sorted(rate.items(), key=lambda t: -t[1])[:15]))
print("slowest 15:", ", ".join(f"{B}:{r:.4f}" for B, r in sorted(rate.items(), key=lambda t: t[1])[:15]))
print("1-digit window formula (B+1)/(3B-2) for comparison: B=10 %.5f, B=100 %.5f, limit 1/3" % (11/28, 101/298))
