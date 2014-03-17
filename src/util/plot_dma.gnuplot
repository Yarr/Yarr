set terminal postscript eps enhanced color font 20
set autoscale
set grid

# Plot DMA data
set output 'benchmarkDma.eps'
set xlabel 'Package size [kB]'
set ylabel 'Transfer speed [MB/s]'
set key bottom right
set title 'DMA Transfer Benchmark'
#set logscale x

plot 'benchmarkDma_write.out' u ($1*4/1024):4 lt 2 lc rgb "red" lw 3 pt 7 ps 1  w linespoints title 'DMA WRITE (CPU -> FPGA)', \
     'benchmarkDma_read.out' u ($1*4/1024):4 lt 2 lc rgb "blue" lw 3 pt 7 ps 1 w linespoints title 'DMA READ (FPGA->CPU)'

! epstopdf benchmarkDma.eps
! rm benchmarkDma.eps


