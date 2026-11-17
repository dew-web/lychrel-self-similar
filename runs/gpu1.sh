SIF=${SIF:?set SIF to an Apptainer image with PyTorch and CUDA}
nvidia-smi --query-gpu=name,memory.total --format=csv
run() { apptainer exec --nv -B /gs $SIF python3 explore/relax_gpu.py "$@"; }
run 16 --samples 40000 --batch 20000
./bin/relax2 16 3000 40000 80 20 16 | cut -d" " -f1-5
run 16 --samples 2000000
