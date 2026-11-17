mkdir -p work/C10 && cd work/C10
for P in 20 22; do for d in 3 6; do ../../bin/outauto2.new -w -cap 25 -min 100000 C 10 $P $d > C10_P${P}_d$d.txt & done; done
for d in 1 2 4 8 7 5 3 6; do ../../bin/outauto2.new -w -cap 25 -min 100000 C 10 24 $d > C10_P24_d$d.txt & done; wait
grep -H summary C10_P2[024]*.txt; grep -H -A2 "terminals=[1-9]\|CAPACITY" C10_P2[024]*.txt | head -40
