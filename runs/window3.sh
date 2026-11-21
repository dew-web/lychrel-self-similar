# 窓の大きさ k = 2, 3, 4 で、基数 3〜256 の桁の増え方（規則が何桁目で現れるか）
cd work/window
seq 3 256 | xargs -P 16 -I{} sh -c "../../bin/window {} 2,3,4 20000000 7{} > small_B{}.txt"
