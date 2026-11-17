#!/usr/bin/env python3
"""coreauto2 系の出力（FULL NUMBER の行）から族を読み取り、探索プログラムとは無関係な直接計算で確かめる。
usage: verify_family.py core出力ファイル [...]
族は N(m) = TOP (B-1)^m CEN 0^m BOT（または 0 と B-1 が逆）。P 回の逆順加算で N(m+δ) になり、途中に回文がないことを、
小さい m から順に調べる。標準ライブラリだけを使う。"""
import re, sys

DIG = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

def step(x, B):
    D = len(x); out = []; c = 0
    for i in range(D - 1, -1, -1):
        v = x[i] + x[D - 1 - i] + c
        c = 1 if v >= B else 0
        out.append(v - B * c)
    if c: out.append(1)
    return out[::-1]

def longest_run(x, d):
    best = (0, 0); i = 0
    while i < len(x):
        if x[i] == d:
            j = i
            while j < len(x) and x[j] == d: j += 1
            if j - i > best[1]: best = (i, j - i)
            i = j
        else: i += 1
    return best

def check(fn):
    txt = open(fn).read()
    for blk in re.split(r'(?=B=\d+ P=\d+ par0=)', txt):
        h = re.match(r'B=(\d+) P=(\d+) par0=(\d)', blk)
        m = re.search(r'FULL NUMBER x0 \((\d+) digits\): (.*)', blk)
        if not h or not m: continue
        B, P, par = int(h.group(1)), int(h.group(2)), int(h.group(3))
        s = m.group(2).strip()
        x = [int(t) for t in s.split()] if B > 36 else [DIG.index(ch) for ch in s]
        assert len(x) == int(m.group(1))
        (ih, lh), (iz, lz) = longest_run(x, B - 1), longest_run(x, 0)
        first, second = ((ih, lh, B - 1), (iz, lz, 0)) if ih < iz else ((iz, lz, 0), (ih, lh, B - 1))
        top, cen, bot = x[:first[0]], x[first[0] + first[1]:second[0]], x[second[0] + second[1]:]
        d1, d2 = first[2], second[2]
        def N(a, b): return top + [d1] * a + cen + [d2] * b + bot
        # 1周期での伸びを、十分長い列で実測する
        a0, b0 = first[1], second[1]
        y = N(a0, b0)
        for _ in range(P): y = step(y, B)
        grow = None
        for da in range(0, 2 * P + 1):
            for db in range(0, 2 * P + 1):
                if y == N(a0 + da, b0 + db): grow = (da, db)
        if grow is None: print(f"{fn}: B={B} P={P} par0={par}: could not match after one period: FAILED"); continue
        da, db = grow
        # 最小の列の長さを探し、そこから長く確かめる
        def ok(a, b):
            y = N(a, b)
            for _ in range(P):
                if y == y[::-1]: return False
                y = step(y, B)
            return y == N(a + da, b + db)
        off = b0 - a0
        good = [ok(a, a + off) if a + off >= 0 else False for a in range(0, 60)]
        amin = next((a for a in range(60) if all(good[a:])), None)
        if amin is None: print(f"{fn}: B={B} P={P} par0={par}: FAILED"); continue
        y = N(amin, amin + off); allok = True
        for k in range(1, 201):
            for _ in range(P):
                if y == y[::-1]: allok = False
                y = step(y, B)
            if y != N(amin + k * da, amin + off + k * db): allok = False
        tot = len(N(amin, amin + off))
        print(f"B={B} P={P} par0={par}: top {len(top)} + run {d1}^m + center {len(cen)} + run {d2}^(m{off:+d}) + bottom {len(bot)}; "
              f"+{da} per period; valid for all tested m >= {amin}; smallest member {tot} digits; {200 * P} steps: {'OK' if allok else 'FAILED'}")
        if '-v' in sys.argv:
            print("   TOP:", top); print("   CEN:", cen); print("   BOT:", bot)

for fn in sys.argv[1:]:
    if fn != '-v': check(fn)
