#!/bin/sh
cd /home/claude/p196
for s in 11 12 13 14; do ./hazard 5000000 15 64 $s >> hazard_out.txt 2>&1; done
echo DONE >> hazard_out.txt
