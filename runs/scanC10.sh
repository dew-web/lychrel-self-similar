# 10進・型C: 中央列の数字の周期（3,6 は周期2、1,2,4,8,7,5 は周期6）ごとに全パターン
mkdir -p work/C10 && cd work/C10
for P in 2 4 6 8 10 12 14 16 18 20; do for d in 3 6; do ../../bin/outauto2 -w -cap 25 -min 1000 C 10 $P $d > C10_P${P}_d$d.txt & done; done; wait
for P in 6 12 18; do for d in 1 2 4 8 7 5; do ../../bin/outauto2 -w -cap 25 -min 1000 C 10 $P $d > C10_P${P}_d$d.txt & done; wait; done
grep -H summary C10_*.txt
grep -H -A2 "terminals=[1-9]" C10_*.txt | head -40
# 旧 outauto（修正版）とも突き合わせる
for pd in "6 1" "6 8" "8 3" "12 5" "10 6"; do set -- $pd; ../../bin/outauto Ca 10 $1 $2 > oldC_P$1_d$2.txt; ../../bin/outauto2 C 10 $1 $2 > newC_P$1_d$2.txt; python3 ../../scripts/compare_outauto.py oldC_P$1_d$2.txt newC_P$1_d$2.txt; done
