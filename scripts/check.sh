#!/bin/sh
# 動作確認（数分）。すべて PASS なら、この PC で計算を再開できる。
cd "$(dirname "$0")/.."
mkdir -p work/check
pass=0; fail=0; T0=$(date +%s)
ok() { echo "PASS  $1"; pass=$((pass+1)); }
ng() { echo "FAIL  $1"; fail=$((fail+1)); }
python3 base9/verify_quick.py > work/check/verify_quick.txt 2>&1 && ok "9進の族を直接計算で確認（verify_quick.py）" || ng "9進の族の直接計算 → work/check/verify_quick.txt"
python3 base9/symbolic_proof.py > work/check/symbolic.txt 2>&1 && ok "9進の族の記号計算による証明（symbolic_proof.py）" || ng "記号計算 → work/check/symbolic.txt"
./bin/coreauto2 9 10001 10000 > work/check/core9.txt 2>&1
grep -q "CORE FOUND" work/check/core9.txt && ok "中心オートマトンが9進の中心を見つける（coreauto2）" || ng "中心オートマトン → work/check/core9.txt"
(cd work/check && ../../bin/outauto_s Aa 9 10 0 p1000110001 > outer9.txt 2>&1)
ls work/check/outer_B9_P10_ov1000110001_cin*.txt >/dev/null 2>&1 && ok "両端オートマトンが9進の両端を見つける（outauto_s）" || ng "両端オートマトン → work/check/outer9.txt"
python3 carries/op.py --quick > work/check/op.txt 2>&1 && ok "繰り上がり込みの作用素（op.py）" || ng "作用素 → work/check/op.txt"
python3 carries/relax_perstep.py --bases 16 --n0 40 --d0 600 --k 6 --out work/check > work/check/relax.txt 2>&1 && ok "緩和の実験が動く（relax_perstep.py、少数）" || ng "緩和の実験 → work/check/relax.txt"
(cd fourier && python3 wave.py > ../work/check/wave.txt 2>&1) && ok "波の検証1〜3（fourier/wave.py）" || ng "波の検証 → work/check/wave.txt"
if [ -x bin/p196 ]; then ./bin/p196 --selftest > work/check/p196.txt 2>&1 && ok "p196（AVX2）の自己テスト" || ng "p196 → work/check/p196.txt"; else echo "SKIP  p196（make p196 で作成。AVX2 がある x86-64 の PC だけ）"; fi
echo "---- PASS $pass / FAIL $fail （$(( $(date +%s) - T0 )) 秒）"
[ $fail -eq 0 ]
