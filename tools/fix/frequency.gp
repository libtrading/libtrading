reset

set terminal unknown

plot file
xmin = GPVAL_DATA_Y_MIN
xmax = GPVAL_DATA_Y_MAX

round(x, width) = width * floor(x / width) + width / 2.0

set terminal postscript eps enhanced
set output "histogram.eps"

xmax = max > 0 ? max : xmax

set xrange [xmin:xmax]
set yrange [0:]

set xtics 0, 50, xmax
set xtics font ",8"
set ytics font ",8"

set xlabel "Round-trip time ({/Symbol m}s)"
set ylabel "Frequency"
set xlabel font ",8"
set ylabel font ",8"

plot file using (round($1, width)):(1.0) smooth freq w boxes lc rgb"blue" notitle
