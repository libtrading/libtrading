reset

set terminal unknown

plot file
xmin = GPVAL_DATA_Y_MIN
xmax = GPVAL_DATA_Y_MAX

round(x, width) = width * floor(x / width) + width / 2.0

set terminal svg size 600, 460 fname 'Verdana' fsize 10
set output "histogram.svg"

xmax = max > 0 ? max : xmax

set xrange [xmin:xmax]
set yrange [0:]

unset xtics
set xtics font ",8"
set ytics font ",8"

set xlabel "RTT (μs)"
set ylabel "Frequency"
set xlabel font ",8"
set ylabel font ",8"

plot file using (round($1, width)):(1.0) smooth freq w boxes lc rgb "blue" notitle
