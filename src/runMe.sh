#!/bin/bash

./bin/scanConsole -c configs/default_fei4b.json -s digitalscan -p  -n 30 >& scan.out &
SCAN_PID=$!
echo Scan PID: $SCAN_PID
sleep 2
echo Starting emulator ...
./bin/emulator >& emu.out &
EMU_PID=$!
echo Emulator PID: $EMU_PID
wait $SCAN_PID

echo -n "Killing emulator..."
kill -9 $EMU_PID
echo "Done"

echo "Available plots:"
ls data/0*$(cat ~/.yarr/runCounter)*/*png
