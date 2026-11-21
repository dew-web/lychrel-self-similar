# 両端と真ん中で「良い対の連なり」の出やすさを比べる（本物の軌道）と、窓のモデルでの同じ量
mkdir -p work/ends && cd work/ends
{ echo "../../bin/ends_palin 10 196 1000000 3000 > ends_196.txt"
  for s in 1 2; do echo "../../bin/ends_palin 10 rand:500:$s 300000 3000 > ends_B10_rand$s.txt"; done
  for B in 9 16 17 33 65 66 18 12 100; do echo "../../bin/ends_palin $B rand:500:5 300000 3000 > ends_B$B.txt"; done
  echo "../../bin/window2 10 32 200000000 11 > win_B10.txt"
  for B in 9 16 17 33 65 66 18 12 100; do echo "../../bin/window2 $B 32 50000000 12 > win_B$B.txt"; done
} | xargs -P 16 -I{} sh -c "{}"
