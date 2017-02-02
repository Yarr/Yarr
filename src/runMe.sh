#!/bin/bash

scanType=${1}
[ -z ${scanType} ] && echo -e "Usage: ${0} scanType" && exit

echo Starting scanConsole ...
./bin/scanConsole -c configs/default_fei4b.json -s ${scanType} -p  -n 30 >& scan.out &
SCAN_PID=$!
echo Scan PID: $SCAN_PID
sleep 2
echo Starting emulator ...
./bin/emulator >& emu.out &
EMU_PID=$!
echo Emulator PID: $EMU_PID
wait $SCAN_PID
echo "Scan completed."

echo -n "Killing emulator..."
kill -9 $EMU_PID
echo "Done"

scanNo=$(cat ~/.yarr/runCounter)
scanDataFolder=data/$(printf "%06d" ${scanNo})_${scanType}

echo "Available plots:"
ls ${scanDataFolder}/*png

mv scan.out emu.out ${scanDataFolder}

echo "Available logs:"
ls ${scanDataFolder}/*out

echo "Note: the killing of PID ${EMU_PID} (bin/emulator) is expected"
