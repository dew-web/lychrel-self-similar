# base2_vs_base10.py
# (1) 2進法の 10110(=22) の軌道が周期4の「型」に入り、回文にならないことを確認
# (2) 2進の22の軌道と10進の196の軌道の「圧縮しやすさ」（=構造の有無）を比べる
import lzma, random, re, sys

def rev_add(x, base):
    if base == 2:
        return x + int(bin(x)[:1:-1], 2)
    s = str(x)
    return x + int(s[::-1])

def digits(x, base):
    return bin(x)[2:] if base == 2 else str(x)

# ---- (1) 2進の 22 ----
x = 22
pattern_ok = True
pal_found = False
forms = {}
N2 = 4000
for n in range(N2 + 1):
    s = digits(x, 2)
    if s == s[::-1]:
        pal_found = True
    if n % 4 == 0 and n >= 4:
        k = n // 4
        expect = "10" + "1" * (k + 1) + "01" + "0" * (k + 1)
        if s != expect:
            pattern_ok = False
    if 40 <= n < 44:
        forms[n % 4] = s
    x = rev_add(x, 2)
print("base2: 22 -> 4n steps pattern '10 1^(n+1) 01 0^(n+1)' holds for all n<=%d: %s" % (N2 // 4, pattern_ok))
print("base2: any palindrome among first %d terms: %s" % (N2, pal_found))
# 4つの位相の形を run-length で表示（n=40..43）
def rle(s):
    return " ".join("%s^%d" % (m.group(0)[0], len(m.group(0))) for m in re.finditer(r"0+|1+", s))
for r in range(4):
    print("  phase %d (n=%d): %s" % (r, 40 + r, rle(forms[r])))
# 圧縮比較用: 上で確認した型（4kステップ後 = 10・1^(k+1)・01・0^(k+1)）から約10^6ビットの項を作る
K = 499_997
b2 = "10" + "1" * (K + 1) + "01" + "0" * (K + 1)

# ---- (2) 圧縮率の比較 ----
def bits_per_symbol(s):
    return 8 * len(lzma.compress(s.encode(), preset=9 | lzma.PRESET_EXTREME)) / len(s)

random.seed(0)
rand2 = "".join(random.choice("01") for _ in range(len(b2)))
print("\nbase2 (22, %d bits):   %.4f bits/symbol  (random bits: %.4f, ideal 1.0)" %
      (len(b2), bits_per_symbol(b2), bits_per_symbol(rand2)))

final = open(sys.argv[1]).read().strip() if len(sys.argv) > 1 else None
if final:
    L = len(final)
    rand10 = "".join(random.choice("0123456789") for _ in range(L))
    print("base10 (196, %d digits): %.4f bits/symbol  (random digits: %.4f, ideal log2(10)=3.3219)" %
          (L, bits_per_symbol(final), bits_per_symbol(rand10)))
