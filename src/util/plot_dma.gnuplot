set style line 1 linetype 1 linecolor rgb "red" linewidth 3.0
set style line 2 linetype 1 linecolor rgb "blue" linewidth 3.0

set terminal postscript eps enhanced color font 20
set autoscale
set grid

# Plot DMA data
set output 'benchmarkDma.eps'
set xlabel 'Package size [kB]'
set ylabel 'Transfer speed [MB/s]'
set key bottom right
set title 'DMA Transfer Benchmark'

plot 'benchmarkDma_write.out' u ($1*4/1024):4 linestyle 1 w lines title 'DMA WRITE (CPU -> FPGA)', \
'benchmarkDma_read.out' u ($1*4/1024):4 linestyle 2 w lines title 'DMA READ (FPGA->CPU)'

! epstopdf benchmarkDma.eps
! rm benchmarkDma.eps


