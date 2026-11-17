SIF=${SIF:?set SIF to an Apptainer image with PyTorch and CUDA}
mkdir -p work/relax_gpu
for B in 64 128 256 512 1024 2048 65 129 257 513 1025 63 127 255 511 100 1000; do
  apptainer exec --nv -B /gs $SIF python3 explore/relax_gpu.py $B --samples 3000000 --seed $B > work/relax_gpu/rg_B$B.txt 2>&1
  head -1 work/relax_gpu/rg_B$B.txt
done
