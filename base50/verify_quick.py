#!/usr/bin/env python3
"""50進法のリクレル数の族を直接確かめる（標準ライブラリだけ）。
N(m) = TOP 49^m CEN 0^m BOT（50進、上位から）。4回の逆順加算で N(m+1) になり、途中に回文が出ないことを調べる。"""
B = 50
TOP = [1,47,5,38,23,4,39,21,8,31,35,28,42,16,18,13,23,3,41,17,16,17,15,19,11,25,47,6,36,25,48,4,40,19,12,24]
CEN = [13,2,20,12,20,2,13]
BOT = [25,11,19,40,3,48,26,36,5,48,26,11,19,15,17,15,17,42,3,23,13,17,15,42,28,36,32,7,21,40,3,23,38,5,47]

def N(m):
    return TOP + [B - 1] * m + CEN + [0] * m + BOT

def step(x):
    D = len(x); out = []; c = 0
    for i in range(D - 1, -1, -1):
        v = x[i] + x[D - 1 - i] + c
        c = 1 if v >= B else 0
        out.append(v - B * c)
    if c: out.append(1)
    return out[::-1]

def period_ok(m):
    x = N(m)
    for _ in range(4):
        if x == x[::-1]: return False
        x = step(x)
    return x == N(m + 1)

good = [period_ok(m) for m in range(0, 80)]
m0 = next(m for m in range(80) if all(good[m:]))
print(f"N(m) -> N(m+1) after 4 steps with no palindrome: holds for every m in {m0}..79 (fails for m < {m0})")
x = N(m0); ok = True
for k in range(1, 251):
    for _ in range(4):
        if x == x[::-1]: ok = False
        x = step(x)
    if x != N(m0 + k): ok = False
print(f"1000 steps from N({m0}): {'OK' if ok else 'FAILED'}")
val = 0
for d in N(m0): val = val * B + d
print(f"smallest member N({m0}): {len(N(m0))} digits in base 50, {len(str(val))} digits in decimal")
