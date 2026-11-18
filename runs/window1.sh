# 両端の窓モデル: 厳密解（小さい k）、モンテカルロ（大きい k）、本物の軌道との比較
cd work/window
BASES="2 3 4 5 6 7 8 9 10 11 12 13 16 17 20 24 26 32 50 64 100 127"
# 1) 本物の軌道（時間がかかるものから先に）
{
  echo "../../bin/orbit_record 10 196 1000000 3000 5000 3000 8 bits_196.bin > real_196.txt"
  for s in 1 2 3; do echo "../../bin/orbit_record 10 rand:500:$s 300000 3000 5000 3000 8 > real_B10_rand$s.txt"; done
  for B in $BASES; do [ $B = 10 ] || echo "../../bin/orbit_record $B rand:500:7 300000 3000 5000 3000 8 > real_B$B.txt"; done
  # 2) 厳密解（k = 1, 2, 3）
  for B in $BASES; do echo "../../bin/window_exact $B 1 > exact_B${B}_k1.txt; ../../bin/window_exact $B 2 > exact_B${B}_k2.txt"; done
  for B in 2 3 4 5 6 7 8 9 10 11 12 13 16 17 20 24 26; do echo "../../bin/window_exact $B 3 0.5 1e-12 20000 > exact_B${B}_k3.txt"; done
  echo "../../bin/window_exact 10 4 0.5 1e-11 4000 > exact_B10_k4.txt"
  # 3) モンテカルロ
  echo "../../bin/window 10 1,2,3,4,6,8,12,16,24,32,48,64 400000000 1 > mc_B10_p50.txt"
  for p in 0.40 0.45 0.55 0.60; do echo "../../bin/window 10 16 200000000 2 $p > mc_B10_p$p.txt"; done
  echo "../../bin/window 10 16 200000000 3 0.5 100000 -0.17 > mc_B10_rho.txt"
  for B in $BASES; do echo "../../bin/window $B 4,8,16,32 100000000 4 > mc_B$B.txt"; done
} | xargs -P 8 -I{} sh -c "{}"
