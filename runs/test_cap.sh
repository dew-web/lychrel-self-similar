#!/bin/sh
#$ -cwd
#$ -l cpu_4=1
#$ -l h_rt=0:30:00
#$ -N lyc_test
cd "${LYCHREL_DIR:?set LYCHREL_DIR to the repository directory}"
for L in 23 26; do
  ( /usr/bin/time -v ./bin/outauto_$L Aa 10 16 0 p0101010101010101 > runs/test_P16_L$L.out 2> runs/test_P16_L$L.time ) &
  ( /usr/bin/time -v ./bin/outauto_$L Aa 10 18 0 p010101001010101001 > runs/test_P18_L$L.out 2> runs/test_P18_L$L.time ) &
done
wait
