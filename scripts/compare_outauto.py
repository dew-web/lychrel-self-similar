#!/usr/bin/env python3
"""outauto（旧）と outauto2（新）の結果を、パターンごとに突き合わせる。
usage: compare_outauto.py old.txt new.txt
旧: '  ov=... reachable=N terminals=T'   新: '  ov=... reachable=N canon=M sym=k depth=d terminals=T'"""
import re, sys
def load(fn):
    d = {}
    for line in open(fn):
        m = re.search(r'ov=([01]+)\s+reachable=(\d+).*?terminals=(\d+)(.*)', line)
        if m: d[m.group(1)] = (int(m.group(2)), int(m.group(3)), 'CAPACITY' in m.group(4))
    return d
a, b = load(sys.argv[1]), load(sys.argv[2])
bad = 0
for ov in sorted(set(a) | set(b)):
    if ov not in a or ov not in b: print("only in one:", ov); bad += 1; continue
    if a[ov][2] or b[ov][2]: continue          # 上限に達したものは比べない
    if a[ov] != b[ov]: print("MISMATCH", ov, a[ov], b[ov]); bad += 1
print(f"{sys.argv[1]} vs {sys.argv[2]}: patterns old={len(a)} new={len(b)} mismatches={bad}")
sys.exit(1 if bad else 0)
