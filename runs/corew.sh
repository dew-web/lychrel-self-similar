# coreauto2 と coreauto2w が同じ答えを出すことを、既知の例で確かめる
cd work/atlas; n=0; bad=0
for f in B{2,4,6,7,8,9,11,14,16,17,20,24,26}_P{4,5,6,8,10}/outer_B*.txt; do [ -f "$f" ] || continue
  B=$(head -1 $f | awk "{print \$1}"); ov=$(head -1 $f | awk "{print \$4}"); cin=$(head -1 $f | awk "{print \$5}")
  a=$(timeout 300 ../../bin/coreauto2 $B $ov $cin $f | grep -E "par0|VERIFIED|FAILED" | sed "s/  */ /g"); b=$(timeout 300 ../../bin/coreauto2w $B $ov $cin $f | grep -E "par0|VERIFIED|FAILED" | sed "s/  */ /g")
  n=$((n+1)); if [ "$a" != "$b" ]; then bad=$((bad+1)); echo "DIFF $f"; echo "$a"; echo "$b"; fi
done; echo "compared=$n different=$bad"
cd ../A10
f=outer_B10_P22_ov1100010000111000100001_cin0100110011101001100111.txt
w=11000100001; c=01001100111
for k in 1 2 3 4; do ov=""; cin=""; for i in $(seq $k); do ov=$ov$w; cin=$cin$c; done; echo "== base 10, center period $((11*k))"; CORE_CAP_LOG=27 ../../bin/coreauto2w 10 $ov $cin $f | cut -c1-200; done
