cd work/b10p24
o=000100010001; c=010100101011
echo "== base 10, outer period 12, center period 48"
CORE_LAYERS=1 CORE_CAP_LOG=29 ../../bin/coreauto2wp 10 $o$o$o$o $c$c$c$c outer_B10_P12_ov000100010001_cin010100101011.txt 2> p48.err | cut -c1-400; tail -1 p48.err
cd ../A10
o=11000100001; c=01001100111
echo "== base 10, outer period 11, center period 33"
CORE_LAYERS=1 CORE_CAP_LOG=29 ../../bin/coreauto2wp 10 $o$o$o $c$c$c outer_B10_P22_ov1100010000111000100001_cin0100110011101001100111.txt 2> p33.err | cut -c1-400; tail -1 p33.err
