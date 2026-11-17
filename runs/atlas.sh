# 基数 $1..$2、周期 $3..$4 の型A全パターンを調べ、端の解が出たら中心（coreauto2）も判定する。並列数 $5
mkdir -p work/atlas && cd work/atlas
one() { B=$1; P=$2; d=B${B}_P$P; mkdir -p $d; cd $d
  timeout 3000 ../../../bin/outauto2 -w -cap 25 -min 100000 A $B $P > outer.txt 2> outer.err; echo "rc=$?" >> outer.txt
  for f in outer_B*.txt; do [ -f "$f" ] || continue
    ov=$(head -1 "$f" | awk "{print \$4}"); cin=$(head -1 "$f" | awk "{print \$5}")
    timeout 900 ../../../bin/coreauto2 $B $ov $cin "$f" > core_${ov}_${cin}.txt 2>&1; echo "rc=$?" >> core_${ov}_${cin}.txt
  done; }
export -f one
for P in $(seq $3 $4); do for B in $(seq $1 $2); do echo "$B $P"; done; done | xargs -P $5 -L 1 bash -c "one \$0 \$1"
