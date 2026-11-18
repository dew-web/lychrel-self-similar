# 窓のモデルで、基数 3〜300 の桁の増え方を測る（k = 8 と 12、各 2000万ステップ）。2^b+1 型など一部は本物の軌道とも比べる
cd work/window
{ for B in $(seq 3 300); do echo "../../bin/window $B 8,12 20000000 $B > scan_B$B.txt"; done
  for B in 18 33 34 65 66 96 120; do echo "../../bin/orbit_record $B rand:500:9 300000 3000 50000 300 1 > real_B$B.txt"; done
} | xargs -P 16 -I{} sh -c "{}"
