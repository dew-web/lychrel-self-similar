#!/bin/sh
# 計算用の PC の環境を表示する（何も変更しない）
echo "OS:      $(uname -s) $(uname -r)   CPU arch: $(uname -m)"
if [ "$(uname -s)" = "Darwin" ]; then
  echo "CPU:     $(sysctl -n machdep.cpu.brand_string 2>/dev/null)   cores: $(sysctl -n hw.ncpu)   memory: $(( $(sysctl -n hw.memsize) / 1073741824 )) GB"
  if sysctl -n hw.optional.avx2_0 2>/dev/null | grep -q 1; then echo "AVX2:    あり（p196 を作れる）"; else echo "AVX2:    なし（p196 は作れない。他はすべて動く）"; fi
else
  echo "CPU:     $(grep -m1 'model name' /proc/cpuinfo 2>/dev/null | cut -d: -f2)   cores: $(nproc 2>/dev/null)   memory: $(awk '/MemTotal/{printf "%d", $2/1048576}' /proc/meminfo 2>/dev/null) GB"
  if grep -q avx2 /proc/cpuinfo 2>/dev/null; then echo "AVX2:    あり（p196 を作れる）"; else echo "AVX2:    なし（p196 は作れない。他はすべて動く）"; fi
fi
echo "C:       $( (cc --version 2>/dev/null || gcc --version 2>/dev/null) | head -1)"
echo "make:    $(make --version 2>/dev/null | head -1)"
echo "Python:  $(python3 --version 2>&1)   numpy: $(python3 -c 'import numpy; print(numpy.__version__)' 2>/dev/null || echo 'なし → python3 -m pip install numpy')"
echo "LaTeX:   $(command -v lualatex >/dev/null 2>&1 && echo 'lualatex あり' || echo 'なし（論文のPDFを作るときだけ必要）')"
