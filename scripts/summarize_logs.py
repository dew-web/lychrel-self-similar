# summarize_logs.py — results/logs から表を作って results/SUMMARY_tables.md に書く
import re, os, glob
os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
L = "results/logs"; out = []
out.append("## Base 10, two-run (type A): outer strings of any length, all overflow patterns\n")
out.append("| period | patterns | with outer solution | over state limit |\n|---|---|---|---|")
rows = {}
for fn, pat in [(f"{L}/fix_progress.log", r"P=(\d+): patterns=(\d+) with_terminals=(\d+) capacity=(\d+)"),
                (f"{L}/scan3_progress.log", r"A B=10 P=(\d+) patterns=(\d+) with_outer=(\d+) capacity=(\d+)")]:
    for line in open(fn):
        m = re.match(pat, line.strip())
        if m: rows[int(m.group(1))] = m.groups()[1:]
for P in sorted(rows): out.append(f"| {P} | {rows[P][0]} | {rows[P][1]} | {rows[P][2]} |")
out.append("\nState-limit cases: P=16 pattern 0101010101010101 (over 8,388,608 states, see p16_single.txt); P=18 pattern 010101001010101001 (over 2,097,152).\n")
out.append("## Other bases (two-run type)\n")
out.append("| base | period | patterns | with outer solution | over state limit |\n|---|---|---|---|---|")
for fn in (f"{L}/scan3_progress.log", f"{L}/scan4_progress.log"):
    for line in open(fn):
        m = re.match(r"A B=(\d+) P=(\d+) patterns=(\d+) with_outer=(\d+) capacity=(\d+)", line.strip())
        if m and m.group(1) != "10": out.append("| " + " | ".join(m.groups()) + " |")
out.append("\n## Center (core) decisions for every outer solution found\n")
out.append("| outer solution | center parity | reachable states | result | full number check |\n|---|---|---|---|---|")
for fn in (f"{L}/fixcore.log", f"{L}/scan3_core.log", f"{L}/scan4_core.log"):
    cur = None; last = None
    for line in open(fn):
        if line.startswith("== "): cur = os.path.basename(line[3:].strip()).replace(".txt", ""); continue
        m = re.match(r"B=(\d+) P=(\d+) par0=(\d) .*reachable states=(\d+)\s+(.*)", line.strip())
        if m and cur:
            res = m.group(5)
            res = "core found" if "CORE FOUND" in res else ("no core of any length" if "ANY length" in res else "state limit")
            last = [cur, m.group(3), m.group(4), res, ""]; out.append(last)
        if "direct reverse-and-add simulation" in line and last:
            last[4] = "VERIFIED" if "VERIFIED" in line else "FAILED"
out = [("| " + " | ".join(x) + " |") if isinstance(x, list) else x for x in out]
open("results/SUMMARY_tables.md", "w").write("\n".join(out) + "\n")
print("wrote results/SUMMARY_tables.md")
