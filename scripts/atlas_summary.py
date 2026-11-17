#!/usr/bin/env python3
"""work/atlas/B*_P*/ の結果を1行ずつにまとめる。
usage: atlas_summary.py work/atlas [--all]
各 (基数, 周期): パターン数、端の解があるパターン数、上限超過、中心の判定（VERIFIED / 中心なし / 未決）"""
import glob, os, re, sys
root = sys.argv[1]; show_all = '--all' in sys.argv
rows = []
for d in glob.glob(os.path.join(root, 'B*_P*')):
    m = re.match(r'B(\d+)_P(\d+)$', os.path.basename(d))
    if not m: continue
    B, P = int(m.group(1)), int(m.group(2))
    txt = open(os.path.join(d, 'outer.txt')).read() if os.path.exists(os.path.join(d, 'outer.txt')) else ''
    sm = re.search(r'patterns=(\d+) with_outer_solution=(\d+) inconclusive=(\d+) maxreach=(\d+)', txt)
    rc = re.search(r'rc=(\d+)', txt)
    status = 'ok' if sm else ('TIMEOUT' if rc and rc.group(1) == '124' else 'running/failed')
    ver, none, und, sigs = 0, 0, 0, []
    for c in sorted(glob.glob(os.path.join(d, 'core_*.txt'))):
        t = open(c).read(); name = os.path.basename(c)[5:-4]
        if 'SELF-SIMILAR, NO PALINDROME (VERIFIED)' in t: ver += 1; sigs.append(name)
        elif 'CAPACITY' in t or 'rc=124' in t or t.count('no periodic core of ANY length') < 2: und += 1
        else: none += 1
    rows.append((B, P, status, sm.groups() if sm else None, ver, none, und, sigs))
rows.sort()
print("base period status patterns with_outer inconclusive maxreach | core: verified none undecided")
for B, P, status, sm, ver, none, und, sigs in rows:
    interesting = status != 'ok' or (sm and (int(sm[1]) or int(sm[2])))
    if not (show_all or interesting): continue
    s = ' '.join(sm) if sm else '-'
    print(f"B={B} P={P} {status} {s} | {ver} {none} {und} {' '.join(sigs)}")
