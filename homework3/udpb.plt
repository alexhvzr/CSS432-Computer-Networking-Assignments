set terminal postscript landscape
set nolabel
set xlabel "Percent Dropped"
set xrange [0:10]
set ylabel "usec"
set yrange [0:1680000]
set output "udpb.ps"
plot "1gbpsa-30.dat" title "1gbpsa-30 sliding window" with linespoints, 1610155 title "1gbpsa-30 stopNwait" with line