set terminal wxt
set datafile missing '0x'
plot 'test_input_hex.txt' using 0:2 with lines
