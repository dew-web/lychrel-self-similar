mkdir -p work/relax2b && cd work/relax2b
for B in 3 4 5 6 7 8 9 10 11 12 13 15 16 17 18 20 24 25 26 27 31 32 33 34 48 63 64 65 96 127 128 129 255 256 257; do echo $B; done | xargs -P 16 -I{} sh -c "../../bin/relax2 {} 3000 40000 80 20 {} > r2_B{}.txt"
