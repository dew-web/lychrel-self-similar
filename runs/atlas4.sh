# atlas.sh の改良版: 端は outauto2.new、中心は coreauto2wp（基数62まで、キー256ビット）。 usage: atlas3.sh B1 B2 P1 P2 並列数 [出力先]
out=work/${6:-atlas3}; mkdir -p $out && cd $out
export OMP_NUM_THREADS=1 CORE_CAP_LOG=24
one() { B=$1; P=$2; d=B${B}_P$P; mkdir -p $d; cd $d
  timeout 3000 ../../../bin/outauto2.new -w -cap 25 -min 100000 A $B $P > outer.txt 2> outer.err; echo "rc=$?" >> outer.txt
  for f in outer_B*.txt; do [ -f "$f" ] || continue
    ov=$(head -1 "$f" | awk "{print \$4}"); cin=$(head -1 "$f" | awk "{print \$5}")
    timeout 900 ../../../bin/coreauto2wp8 $B $ov $cin "$f" > core_${ov}_${cin}.txt 2>&1; echo "rc=$?" >> core_${ov}_${cin}.txt
  done; }
export -f one
for P in $(seq $3 $4); do for B in $(seq $1 $2); do echo "$B $P"; done; done | xargs -P $5 -L 1 bash -c "one \$0 \$1"
