mkdir -p work/relax && cd work/relax
for B in $(seq 2 40) 48 56 63 64 65 72 80 96 100 127 128 129; do echo $B; done | xargs -P 16 -I{} sh -c "../../bin/relax {} 3000 100000 32 {} > relax_B{}.txt"
