mkdir -p work/val1 && cd work/val1
for P in 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
  ../../bin/outauto Aa 10 $P 0 > old_P$P.txt
  ../../bin/outauto2 A 10 $P > new_P$P.txt
  ../../bin/outauto2 -nosym A 10 $P > nosym_P$P.txt
  python3 ../../scripts/compare_outauto.py old_P$P.txt new_P$P.txt
  python3 ../../scripts/compare_outauto.py old_P$P.txt nosym_P$P.txt
done
for bp in "9 10" "11 6" "26 4" "2 4" "4 6" "3 10" "6 8" "7 8"; do set -- $bp
  ../../bin/outauto Aa $1 $2 0 > old_B$1_P$2.txt; ../../bin/outauto2 -w A $1 $2 > new_B$1_P$2.txt
  python3 ../../scripts/compare_outauto.py old_B$1_P$2.txt new_B$1_P$2.txt; grep -c "direct window simulation: OK" new_B$1_P$2.txt; grep -c FAILED new_B$1_P$2.txt
done
