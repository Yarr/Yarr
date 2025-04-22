set style line 1 linetype 1 linecolor rgb "red" linewidth 3.0
set style line 2 linetype 1 linecolor rgb "blue" linewidth 3.0

set terminal postscript eps enhanced color font 20
set autoscale
set grid

# Plot DMA data
set output 'benchmarkSingle.eps'
set xlabel 'Package size [B]'
set ylabel 'Transfer speed [MB/s]'
set key right center
set title 'CSR Transfer Benchmark'
set log y

plot 'benchmarkSingle_write.out' u ($1*4):4 lt 2 lc rgb "red" lw 3 pt 7 ps 1  w linespoints title 'CSR WRITE (CPU -> FPGA)', \
     'benchmarkSingle_read.out' u ($1*4):4 lt 2 lc rgb "blue" lw 3 pt 7 ps 1 w linespoints title 'CSR READ (FPGA->CPU)'

! epstopdf benchmarkSingle.eps
! rm benchmarkSingle.eps


