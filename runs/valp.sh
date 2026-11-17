for a in "A 10 01010101" "A 9 1000110001" "A 10 010101001010101001" "A 10 1100010000111000100001" "A 10 0010111000100101110001" "C 10 000100010001 5" "A 14 1000110001"; do
  ./bin/outauto2p -cap 26 $a 2>/dev/null | grep -v "^single"; ./bin/outauto2p -cap 26 $a 2>/dev/null | tail -1
  ./bin/outauto2.new -cap 26 $a | grep -E "ov=|^single"
done
