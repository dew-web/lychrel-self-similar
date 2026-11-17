# 10進・型A・周期 $1..$2 の全パターン（8並列）
mkdir -p work/A10 && cd work/A10
for P in $(seq $1 $2); do
  for i in 0 1 2 3 4 5 6 7; do ../../bin/outauto2.new -w -cap 25 -min 1000 -part $i/8 A 10 $P > A10_P${P}_part$i.txt & done; wait
  cat A10_P${P}_part*.txt | grep summary | awk -v P=$P "{for(i=1;i<=NF;i++){split(\$i,a,\"=\"); if(a[1]==\"patterns\")p+=a[2]; if(a[1]==\"with_outer_solution\")s+=a[2]; if(a[1]==\"inconclusive\")c+=a[2]; if(a[1]==\"maxreach\"&&a[2]+0>m)m=a[2]+0}} END{print \"P=\"P\" patterns=\"p\" with_outer=\"s\" inconclusive=\"c\" maxreach=\"m}"
  date +%T
done
