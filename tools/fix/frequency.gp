reset

set terminal unknown

plot file

set terminal svg size 600, 460 fname 'Verdana' fsize 10
set output "histogram.svg"

set xrange [0:]
set yrange [0:]

unset xtics
set xtics 0, 5
set xtics font ",8"
set ytics font ",8"

set xlabel "RTT (μs)"
set ylabel "Frequency"
set xlabel font ",8"
set ylabel font ",8"

plot file using ($1):(1.0) smooth freq w boxes lc rgb "blue" notitle
