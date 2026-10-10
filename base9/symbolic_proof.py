# symbolic_proof.py — 9進の族 N(m) について「逆順加算10回で N(m+2) になる」ことを
# すべての m ≥ 2 について証明する（計算機援用だが、m を記号のまま扱う厳密な計算）。
#
# 数を「数字の連続（ラン）の列」で表し、各ランの長さを m の1次式 a*m+b で持つ。
# 逆順加算1回を、この表現のまま厳密に計算する:
#   (1) 数とその反転を重ねたときの境界（1次式）を、m ≥ M0 で順序が変わらないことを確かめて並べる
#   (2) 境界の間の各区間では列和 s が一定。区間内の繰り上がりは
#         s ≥ 9 なら常に1、s ≤ 7 なら常に0、s = 8 なら入ってきた値を素通し
#       なので、出力は「先頭1桁 + 残りは同じ数字」になり、長さも1次式で書ける
# これで「位相 t の形 → 位相 t+1 の形」が m ≥ M0 のすべてで成り立つことを示し、
# m < M0 は数値で直接確かめる。
import functools, re

B = 9
P0 = "1012563461128208006630705832188788502108582878207"
C0 = "10581"
S0 = "782087205811204808870238508037588811821254465208"
def N(m): return P0 + "8" * m + C0 + "0" * m + S0

# ---------- 数値計算（小さい m の確認と、各位相の形を見つけるため） ----------
def step_num(d):
    n, o, c = len(d), [], 0
    for i in range(n):
        v = d[i] + d[n - 1 - i] + c; o.append(v % B); c = v // B
    return o + [1] if c else o
dig = lambda s: [int(ch) for ch in reversed(s)]
st = lambda d: "".join(str(v) for v in reversed(d))

def templ(s):
    r = [(mm.group(0)[0], len(mm.group(0))) for mm in re.finditer(r"(.)\1*", s)]
    a, b = sorted(sorted(range(len(r)), key=lambda i: -r[i][1])[:2])
    return ("".join(c * l for c, l in r[:a]), r[a], "".join(c * l for c, l in r[a + 1:b]), r[b],
            "".join(c * l for c, l in r[b + 1:]))

M1 = 40                      # 形を読み取るための代表値（証明そのものは下の記号計算）
d = dig(N(M1)); T = []
for t in range(10):
    A, (ra, la), Cc, (rc, lc), E = templ(st(d))
    assert la == lc
    T.append((A, int(ra), la - M1, Cc, int(rc), E))   # 2本の列の長さ = m + オフセット
    d = step_num(d)
assert T[0] == (P0, 8, 0, C0, 0, S0)

# ---------- m を記号のまま扱う逆順加算 ----------
def add(x, y): return (x[0] + y[0], x[1] + y[1])
def sub(x, y): return (x[0] - y[0], x[1] - y[1])
def cmpf(x, y, M0):           # m ≥ M0 で常に成り立つ大小関係（変わるなら例外）
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
def template_segs(A, ra, off, Cc, rc, E, shift=0):   # 最下位からのラン列
    segs = [(int(ch), (0, 1)) for ch in reversed(E)] + [(rc, (1, off + shift))] + \
           [(int(ch), (0, 1)) for ch in reversed(Cc)] + [(ra, (1, off + shift))] + \
           [(int(ch), (0, 1)) for ch in reversed(A)]
    return normalize(segs)
def F(segs, M0):
    for dg, L in segs:        # m ≥ M0 で、伸びるランは2桁以上・固定ランは1桁以上
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
        s = digit_at(lo) + digit_at(sub(D, hi))       # 区間の列和（鏡像の区間は境界で揃っている）
        first = (s + c) % B; c1 = 1 if s + c >= B else 0
        if L == (0, 1):
            out.append((first, (0, 1))); c = c1; continue
        if L[0] > 0 and L[0] * M0 + L[1] < 2: raise ValueError("short run")
        rest = sub(L, (0, 1))
        if s >= B: rd, c2 = (s + 1) % B, 1            # 区間の2桁目以降: 繰り上がりは常に1
        elif s <= B - 2: rd, c2 = s, 0                # 常に0
        else: rd, c2 = (s + c) % B, c                 # s = 8: 素通し
        out.append((first, (0, 1))); out.append((rd, rest)); c = c2
    if c: out.append((1, (0, 1)))
    return normalize(out)

for M0 in range(3, 60):
    try:
        allok = True
        for t in range(10):
            cur = template_segs(*T[t])
            nxt = template_segs(*T[t + 1]) if t < 9 else template_segs(*T[0], shift=2)   # 位相10 = N(m+2)
            if F(cur, M0) != nxt: allok = False
        break
    except ValueError:
        continue
print(f"symbolic check valid for all m >= M0 = {M0}: every phase t -> t+1 exact, phase 9 -> N(m+2): {allok}")
nonpal = all([x[0] for x in template_segs(*T[t])] != [x[0] for x in template_segs(*T[t])][::-1] for t in range(10))
print(f"no phase can be a palindrome for m >= M0 (run pattern is not symmetric): {nonpal}")
for m in range(2, M0):
    d = dig(N(m))
    for t in range(10):
        d = step_num(d); assert d != d[::-1]
    assert d == dig(N(m + 2))
print(f"direct check for 2 <= m < {M0}: OK")
print("\none period (most significant digit first; run lengths in terms of m):")
for t in range(10):
    A, ra, off, Cc, rc, E = T[t]
    print(f" t={t}: {A} [{ra}^(m{off:+d})] {Cc} [{rc}^(m{off:+d})] {E}")
print(f" t=10: = N(m+2)")
