# verify_quick.py — 9進の族 N(m) が「逆順加算10回で N(m+2)、途中に回文なし」を m=2..60 で確認する短いチェック
# （すべての m についての証明は symbolic_proof.py）
P = "1012563461128208006630705832188788502108582878207"; C = "10581"
S = "782087205811204808870238508037588811821254465208"
N = lambda m: P + "8" * m + C + "0" * m + S
dig = lambda s: [int(c) for c in reversed(s)]
def step(d):
    n, o, c = len(d), [], 0
    for i in range(n):
        v = d[i] + d[n - 1 - i] + c; o.append(v % 9); c = v // 9
    return o + [1] if c else o
for m in range(2, 61):
    d = dig(N(m))
    for _ in range(10):
        d = step(d); assert d != d[::-1]
    assert d == dig(N(m + 2))
print("OK: N(m) -> N(m+2) after 10 steps with no palindrome, for m = 2..60")
