# 端の解はあるが同じ周期の中心がなかったものについて、中心の周期を k 倍（k=2,3）にして調べる
mkdir -p work/coremult; out=work/coremult
for f in work/atlas/B*_P*/outer_B*.txt work/atlas_rep2/B*_P*/outer_B*.txt work/A10/outer_B10_P22*.txt; do [ -f "$f" ] || continue
  d=$(dirname $f); B=$(head -1 $f | awk "{print \$1}"); P=$(head -1 $f | awk "{print \$2}"); ov=$(head -1 $f | awk "{print \$4}"); cin=$(head -1 $f | awk "{print \$5}")
  if grep -q "VERIFIED" $d/core_${ov}_${cin}.txt 2>/dev/null; then continue; fi
  for k in 2 3; do
    [ $((P*k)) -le 48 ] || continue
    o=""; c=""; for i in $(seq $k); do o=$o$ov; c=$c$cin; done
    r=$out/B${B}_P${P}x${k}_${ov}_${cin}.txt
    [ -s $r ] && continue
    CORE_CAP_LOG=27 timeout 900 ./bin/coreauto2wp $B $o $c $f > $r 2>&1; echo "rc=$?" >> $r
    echo "B=$B P=$P x$k ov=$ov cin=$cin : $(grep -c "no periodic core" $r) none, $(grep -c CAPACITY $r) cap, $(grep -c VERIFIED $r) VERIFIED, $(grep -c "key too long" $r) toolong, $(tail -1 $r)"
  done
done
