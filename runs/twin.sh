#!/bin/sh
# 同じ計算を2つの資源タイプに投げてあるとき、先に始まった方を残し、もう片方を取り消す
# usage: twin.sh JOB1 JOB2  （ログインノードで nohup して使う。軽い）
while :; do
  s1=$(qstat | awk -v j=$1 "\$1==j {print \$5}"); s2=$(qstat | awk -v j=$2 "\$1==j {print \$5}")
  if [ "$s1" = "r" ]; then qdel $2 >/dev/null 2>&1; echo "$(date +%T) $1 started, cancelled $2"; exit; fi
  if [ "$s2" = "r" ]; then qdel $1 >/dev/null 2>&1; echo "$(date +%T) $2 started, cancelled $1"; exit; fi
  if [ -z "$s1" ] && [ -z "$s2" ]; then echo "$(date +%T) both gone"; exit; fi
  sleep 60
done
