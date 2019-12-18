set terminal postscript landscape
set nolabel
set xlabel "Percent Dropped"
set xrange [0:10]
set ylabel "usec"
set yrange [0:8000000]
set output "udpa.ps"
plot "1gbpsa-1.dat" title "1gbpsa-1 slinding window" with linespoints, 7193277 title "1gbpsa-1 stopNwait" with line