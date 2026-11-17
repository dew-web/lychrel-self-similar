# 桁あふれパターン 0101（周期4）だけを、基数 $1..$2 で調べる
mkdir -p work/p4scan && cd work/p4scan
export OMP_NUM_THREADS=1 CORE_CAP_LOG=25
one() { B=$1; mkdir -p B$B; cd B$B
  timeout 1500 ../../../bin/outauto2.big -w -cap 25 A $B 0101 > outer.txt 2>&1; echo "rc=$?" >> outer.txt
  for f in outer_B*.txt; do [ -f "$f" ] || continue
    cin=$(head -1 "$f" | awk "{print \$5}")
    timeout 1500 ../../../bin/coreauto2wp10 $B 0101 $cin "$f" > core_$cin.txt 2>&1; echo "rc=$?" >> core_$cin.txt
  done; }
export -f one
seq $1 $2 | xargs -P 16 -L 1 bash -c "one \$0"
