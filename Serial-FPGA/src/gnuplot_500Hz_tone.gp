set terminal wxt

set title "Scaled Data Plot"

set xlabel "Index"
set ylabel "Value (scaled by 1/1000)"

set grid

plot 'tone500Hz.txt' using 1:2 with lines
