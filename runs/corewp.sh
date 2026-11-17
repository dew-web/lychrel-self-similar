# 並列版 coreauto2wp を既知の例で coreauto2 と突き合わせ、そのあと10進の周期22の中心を調べる
cd work/atlas; n=0; bad=0
for f in B{2,4,6,7,8,9,11,14,16,17,20,24,26}_P{4,5,6,8,10}/outer_B*.txt; do [ -f "$f" ] || continue
  B=$(head -1 $f | awk "{print \$1}"); ov=$(head -1 $f | awk "{print \$4}"); cin=$(head -1 $f | awk "{print \$5}")
  a=$(timeout 300 ../../bin/coreauto2 $B $ov $cin $f | grep -E "par0|VERIFIED|FAILED" | sed "s/reachable states=[0-9]*//; s/([0-9]* layers.*//; s/  */ /g")
  b=$(timeout 300 ../../bin/coreauto2wp $B $ov $cin $f | grep -E "par0|VERIFIED|FAILED" | sed "s/reachable states=[0-9]*//; s/([0-9]* layers.*//; s/  */ /g")
  n=$((n+1)); if [ "$a" != "$b" ]; then bad=$((bad+1)); echo "DIFF $f"; echo "$a"; echo "$b"; fi
done; echo "compared=$n different=$bad"
# 中心がない場合の状態数は、並列でも完全に一致するはず
../../bin/coreauto2 10 11000100001 01001100111 | cut -c1-120; ../../bin/coreauto2wp 10 11000100001 01001100111 | cut -c1-120
cd ../A10
f=outer_B10_P22_ov1100010000111000100001_cin0100110011101001100111.txt
echo "== base 10, center period 22, cap 2^29"
CORE_LAYERS=1 CORE_CAP_LOG=29 ../../bin/coreauto2wp 10 1100010000111000100001 0100110011101001100111 $f | cut -c1-300
