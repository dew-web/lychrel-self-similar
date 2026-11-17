# w を REP 回くり返した桁あふれパターンだけを調べる版。 usage: atlasw.sh B1 B2 P1 P2 並列数 REP [周期の刻み]
mkdir -p work/atlas_rep$6 && cd work/atlas_rep$6
export REP=$6
one() { B=$1; P=$2; d=B${B}_P$P; mkdir -p $d; cd $d
  timeout 3000 ../../../bin/outauto2.new -w -cap 25 -min 100000 -rep $REP A $B $P > outer.txt 2> outer.err; echo "rc=$?" >> outer.txt
  for f in outer_B*.txt; do [ -f "$f" ] || continue
    ov=$(head -1 "$f" | awk "{print \$4}"); cin=$(head -1 "$f" | awk "{print \$5}")
    timeout 900 ../../../bin/coreauto2 $B $ov $cin "$f" > core_${ov}_${cin}.txt 2>&1; echo "rc=$?" >> core_${ov}_${cin}.txt
  done; }
export -f one
for P in $(seq $3 ${7:-1} $4); do for B in $(seq $1 $2); do echo "$B $P"; done; done | xargs -P $5 -L 1 bash -c "one \$0 \$1"
