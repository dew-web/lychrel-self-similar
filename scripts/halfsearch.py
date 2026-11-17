# halfsearch.py — 桁あふれのパターンを「同じ語の繰り返し ww」に絞った、自己相似サイクルの探索
#  ・各基数で、周期 P = 2h（h = 2..hmax）の ww 型パターンだけ両端オートマトン（outauto）を回す
#  ・端の解 (ov, cin) が出たら、両方を同時に繰り返す最小の周期 p0（桁あふれの数が偶数）に縮めて中心（coreauto2）を判定
#  ・中心が見つかったら、元の周期の両端の窓と組み合わせて、実際の数で直接シミュレーションして確認する
#   例: python3 scripts/halfsearch.py 9 10                 （9進で再発見できるかの確認、数分）
#       python3 scripts/halfsearch.py 3,5,6,7,14,18 10      （端の解が出る基数）
#       python3 scripts/halfsearch.py 12 12 --outauto bin/outauto --outer-timeout 600   （大きいPC向け）
import subprocess, itertools, os, sys, time, glob, re, argparse
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ap = argparse.ArgumentParser()
ap.add_argument("bases"); ap.add_argument("hmax", type=int, help="半周期の最大値（周期は 2*hmax まで）")
ap.add_argument("--outauto", default=os.path.join(ROOT, "bin", "outauto_s"))
ap.add_argument("--coreauto", default=os.path.join(ROOT, "bin", "coreauto2"))
ap.add_argument("--outer-timeout", type=float, default=20); ap.add_argument("--core-timeout", type=float, default=60)
ap.add_argument("--work", default=os.path.join(ROOT, "work", "half"))
a = ap.parse_args()
OUTAUTO, COREAUTO = os.path.abspath(a.outauto), os.path.abspath(a.coreauto)
os.makedirs(a.work, exist_ok=True); os.chdir(a.work)
out = open("half_progress.txt", "a")
def P(*x):
    line = " ".join(map(str, x)); print(line, flush=True); out.write(line + "\n"); out.flush()
def necklaces(h):
    seen = set(); res = []
    for bits in itertools.product("01", repeat=h):
        w = "".join(bits)
        if "1" not in w: continue
        c = min(w[k:] + w[:k] for k in range(h))
        if c not in seen: seen.add(c); res.append(c)
    return res
def run(cmd, tl):
    try: return subprocess.run(cmd, capture_output=True, text=True, timeout=tl).stdout
    except subprocess.TimeoutExpired: return "TIMEOUT"
def minper(ov, cin):
    n = len(ov)
    for p in range(1, n + 1):
        if n % p == 0 and ov == ov[:p] * (n // p) and cin == cin[:p] * (n // p) and ov[:p].count("1") % 2 == 0: return p
    return n
def step(d, B):
    n = len(d); s = [d[i] + d[n - 1 - i] for i in range(n)]; o = []; c = 0
    for v in s: v += c; c = v // B; o.append(v % B)
    if c: o.append(1)
    return o
def verify(B, f, core, cin_full):
    L = open(f).read().split("\n"); P_ = int(L[0].split()[1])
    a0 = [int(t) for t in L[1].split("|")[0].split()]; b0 = [int(t) for t in L[1].split("|")[1].split()]
    Lr, Hr = (B - 1) * (1 - int(cin_full[-1])), (B - 1) * int(cin_full[-1])
    nov = L[0].split()[3].count("1")
    build = lambda n: b0 + [Lr] * n + core + [Hr] * n + a0[::-1]
    x = build(20)
    for rep in range(1, 6):
        for t in range(P_):
            x = step(x, B)
            if x == x[::-1]: return "PALINDROME"
        if x != build(20 + rep * nov // 2): return "FAILED"
    return "VERIFIED"
cache = {}
P(f"# start {time.strftime('%Y-%m-%d %H:%M:%S')} bases={a.bases} hmax={a.hmax} outauto={os.path.basename(OUTAUTO)} timeouts={a.outer_timeout}/{a.core_timeout}s")
for B in [int(b) for b in a.bases.split(",")]:
    t0 = time.time(); n = found = tout = 0
    for h in range(2, a.hmax + 1):
        for w in necklaces(h):
            pat = w + w; n += 1
            r = run([OUTAUTO, "Aa", str(B), str(2 * h), "0", "p" + pat], a.outer_timeout)
            if r == "TIMEOUT": tout += 1; P(f"  B={B} ov={pat} outer TIMEOUT"); continue
            if "terminals=" not in r or "terminals=0" in r: continue
            for f in sorted(glob.glob(f"outer_B{B}_P{2*h}_ov{pat}_cin*.txt")):
                cin = open(f).readline().split()[4]; found += 1
                p0 = minper(pat, cin); key = (B, pat[:p0], cin[:p0])
                if key not in cache:
                    c = run([COREAUTO, str(B), pat[:p0], cin[:p0]], a.core_timeout); cache[key] = c
                    res = "TIMEOUT" if c == "TIMEOUT" else ("FOUND" if "CORE FOUND" in c else ("CAP" if "CAPACITY" in c else "NONE"))
                    P(f"  B={B} P={2*h} ov={pat} cin={cin} -> core check at period {p0}: {res}")
                    if res == "FOUND":
                        for m in re.finditer(r"core-list:(\S+)", c):
                            core = [int(t) for t in m.group(1).split(",")]
                            P(f"    core {core} -> full-number check: {verify(B, f, core, cin)}")
    P(f"B={B}: patterns={n} outer_solutions={found} outer_timeouts={tout} ({time.time()-t0:.0f}s)")
P("DONE")
