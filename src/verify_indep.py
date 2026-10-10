# verify_indep.py — Cのコードとは独立に、ログに出た数（文字列）をそのまま使って逆順加算を直接回し、
# (1) 回文が一度も出ない (2) P ステップごとに「同じ形で長い2本の列が δ ずつ伸びた数」に戻る
# を多数の周期・複数の初期列長で確かめる。
import re, sys
def step(d, B):                      # d: 下位桁から
    n = len(d); out = []; c = 0
    for i in range(n):
        v = d[i] + d[n-1-i] + c; c, v = divmod(v, B); out.append(v)
    if c: out.append(1)
    return out
def runs(s):
    return [(m.group(0)[0], len(m.group(0))) for m in re.finditer(r"(.)\1*", s)]
def shape(s):                         # 長さ10以上の列を * に置き換えた形と、その列の長さ
    r = runs(s); return tuple((c, l if l < 10 else "*") for c, l in r), [l for c, l in r if l >= 10]
def check(x0str, B, P, periods, adjust):
    s = x0str
    # 2本の長い列の長さを adjust だけ変えた変種も作る（列の長さによらないことの確認）
    r = runs(s); out = ""
    for c, l in r: out += c * (l + adjust if l >= 10 else l)
    d = [int(ch, 36) for ch in reversed(out)]
    sh0, L0 = shape(out)
    for k in range(1, periods + 1):
        for t in range(P):
            d = step(d, B)
            if d == d[::-1]: return f"PALINDROME at period {k}"
        st = "".join("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[v] for v in reversed(d))
        sh, L = shape(st)
        if sh != sh0 or len(L) != len(L0) or any(L[i] - L0[i] != 2 * k for i in range(len(L))):
            return f"shape changed at period {k}"
    return f"OK: {periods} periods ({periods*P} steps), no palindrome, both long runs grew by exactly 2 per period, final length {len(d)}"
if __name__ == "__main__":
    nums = re.findall(r"FULL NUMBER x0 \((\d+) digits\): (\S+)", open(sys.argv[1] if len(sys.argv) > 1 else "results/logs/scan3_core.log").read())
    for D, x in nums:
        if not x.startswith("1012563461"): continue        # 9進の4つ
        for adj in (-8, 0, 25, 100):
            print(f"base 9, x0 ({int(D)+2*adj} digits, run length shifted by {adj:+d}): {check(x, 9, 10, 150, adj)}")
