#!/usr/bin/env python3
"""任意の基数の族について、base9/symbolic_proof.py と同じ記号計算による証明を行う。
usage: symbolic_proof.py core出力ファイル [...]     （coreauto2 系の出力。FULL NUMBER の行から族を読み取る）

族 N(m) = TOP a^m CEN c^m BOT（a + c = B-1）について、
  「P 回の逆順加算で N(m+δ) になり、途中に回文が現れない」
を、すべての m >= m_min について示す。
  - m >= M0: 列の長さを m の1次式のまま扱う厳密な計算（各位相の形が次の位相の形に移ることを確かめる）
  - m_min <= m < M0: 直接計算
方法は base9/symbolic_proof.py と同じ。数字を整数のリストで持つので、基数に制限はない。"""
import functools, re, sys

DIG = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

def step_num(d, B):                      # d は最下位から
    n, o, c = len(d), [], 0
    for i in range(n):
        v = d[i] + d[n - 1 - i] + c; o.append(v % B); c = v // B
    return o + [1] if c else o

def rle(x):
    r = []
    for v in x:
        if r and r[-1][0] == v: r[-1][1] += 1
        else: r.append([v, 1])
    return r

def templ(x):                            # x は最上位から。長い2本の列で5つに分ける
    r = rle(x)
    a, b = sorted(sorted(range(len(r)), key=lambda i: -r[i][1])[:2])
    flat = lambda rr: [v for v, l in rr for _ in range(l)]
    return flat(r[:a]), tuple(r[a]), flat(r[a + 1:b]), tuple(r[b]), flat(r[b + 1:])

def add(x, y): return (x[0] + y[0], x[1] + y[1])
def sub(x, y): return (x[0] - y[0], x[1] - y[1])
def cmpf(x, y, M0):                      # m >= M0 で常に成り立つ大小関係（変わるなら例外）
    if x == y: return 0
    da, db = x[0] - y[0], x[1] - y[1]
    if da == 0: return -1 if db < 0 else 1
    v0 = da * M0 + db
    if da > 0 and v0 > 0: return 1
    if da < 0 and v0 < 0: return -1
    raise ValueError("order not fixed")
def normalize(segs):
    out = []
    for dg, L in segs:
        if out and out[-1][0] == dg: out[-1] = (dg, add(out[-1][1], L))
        else: out.append((dg, L))
    return out
def template_segs(A, ra, off, Cc, rc, E, shift=0):       # 最下位からのラン列
    segs = [(v, (0, 1)) for v in reversed(E)] + [(rc, (1, off + shift))] + \
           [(v, (0, 1)) for v in reversed(Cc)] + [(ra, (1, off + shift))] + \
           [(v, (0, 1)) for v in reversed(A)]
    return normalize(segs)
def F(segs, M0, B):
    for dg, L in segs:
        if L[0] < 0 or L[0] * M0 + L[1] < (2 if L[0] > 0 else 1): raise ValueError("too short")
    S = [(0, 0)]
    for dg, L in segs: S.append(add(S[-1], L))
    D = S[-1]
    pts = []
    for p in S + [sub(D, s) for s in S]:
        if p not in pts: pts.append(p)
    pts.sort(key=functools.cmp_to_key(lambda x, y: cmpf(x, y, M0)))
    def digit_at(r):
        for i in range(len(segs)):
            if cmpf(S[i], r, M0) <= 0 and cmpf(r, S[i + 1], M0) < 0: return segs[i][0]
        raise ValueError("position")
    out, c = [], 0
    for k in range(len(pts) - 1):
        lo, hi = pts[k], pts[k + 1]
        L = sub(hi, lo)
        s = digit_at(lo) + digit_at(sub(D, hi))
        first = (s + c) % B; c1 = 1 if s + c >= B else 0
        if L == (0, 1):
            out.append((first, (0, 1))); c = c1; continue
        if L[0] > 0 and L[0] * M0 + L[1] < 2: raise ValueError("short run")
        rest = sub(L, (0, 1))
        if s >= B: rd, c2 = (s + 1) % B, 1
        elif s <= B - 2: rd, c2 = s, 0
        else: rd, c2 = (s + c) % B, c
        out.append((first, (0, 1))); out.append((rd, rest)); c = c2
    if c: out.append((1, (0, 1)))
    return normalize(out)

def prove(B, P, x0, label):
    top, (ra, la), cen, (rc, lc), bot = templ(x0)
    if la != lc or ra + rc != B - 1: print(f"{label}: unexpected form"); return False
    N = lambda m: top + [ra] * m + cen + [rc] * m + bot           # 最上位から
    low = lambda x: x[::-1]
    M1 = 60
    d = low(N(M1)); T = []
    for t in range(P):
        A, (a, l1), Cc, (c, l2), E = templ(d[::-1])
        if l1 != l2: print(f"{label}: runs differ at phase {t}"); return False
        T.append((A, a, l1 - M1, Cc, c, E))
        d = step_num(d, B)
    A, (a, l1), Cc, (c, l2), E = templ(d[::-1])
    delta = l1 - M1
    if (A, a, Cc, c, E) != (top, ra, cen, rc, bot) or l1 != l2 or delta <= 0: print(f"{label}: does not return to its form"); return False
    M0 = None
    for cand in range(1, 200):
        try:
            ok = True
            for t in range(P):
                cur = template_segs(*T[t])
                nxt = template_segs(*T[t + 1]) if t < P - 1 else template_segs(*T[0], shift=delta)
                if F(cur, cand, B) != nxt: ok = False
            if ok: M0 = cand
            break
        except ValueError:
            continue
    if M0 is None: print(f"{label}: symbolic check FAILED"); return False
    nonpal = all([s[0] for s in template_segs(*T[t])] != [s[0] for s in template_segs(*T[t])][::-1] for t in range(P))
    # 小さい m は直接計算。成り立つ最小の m を探す
    def direct(m):
        d = low(N(m))
        for t in range(P):
            if d == d[::-1]: return False
            d = step_num(d, B)
        return d == low(N(m + delta))
    good = [direct(m) for m in range(0, M0)]
    mmin = next((m for m in range(M0 + 1) if all(good[m:])), M0)
    print(f"{label}: N(m) = TOP[{len(top)}] {ra}^m CEN[{len(cen)}] {rc}^m BOT[{len(bot)}], F^{P}(N(m)) = N(m+{delta}), no palindrome: "
          f"symbolic for all m >= {M0}: OK; run pattern never symmetric: {nonpal}; direct for {mmin} <= m < {M0}: OK  ==> PROVED for all m >= {mmin}"
          f" (smallest member: {len(N(mmin))} digits)")
    return nonpal

def main():
    for fn in sys.argv[1:]:
        txt = open(fn).read()
        for blk in re.split(r'(?=B=\d+ P=\d+ par0=)', txt):
            h = re.match(r'B=(\d+) P=(\d+) par0=(\d)', blk)
            m = re.search(r'FULL NUMBER x0 \((\d+) digits\): (.*)', blk)
            if not h or not m: continue
            B, P, par = int(h.group(1)), int(h.group(2)), int(h.group(3))
            s = m.group(2).strip()
            x = [int(t) for t in s.split()] if B > 36 else [DIG.index(ch) for ch in s]
            prove(B, P, x, f"base {B}, period {P}, parity {par}")
main()
