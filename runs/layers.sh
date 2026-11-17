#!/bin/sh
#$ -cwd
#$ -l cpu_4=1
#$ -l h_rt=0:30:00
#$ -N lyc_layers
cd "${LYCHREL_DIR:?set LYCHREL_DIR to the repository directory}"
export OA_LAYERS=1
./bin/outauto_26 Aa 10 16 0 p0101010101010101 > runs/layers_P16_L26.out 2> runs/layers_P16_L26.err &
./bin/outauto_23 Aa 10 18 0 p010101001010101001 > runs/layers_P18_L23.out 2> runs/layers_P18_L23.err &
wait
