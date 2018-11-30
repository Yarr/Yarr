#!/bin/bash

echo "Measuring currnent for a specific loaded configuration"
echo "Need to have labRemote, change the path to the repository"

cd ~/labRemote/build

#Analog
AnalogI=`./bin/agilent_measure -c 1 -p /dev/ttyUSB0 -g 3 get-current`
#AnalogI=`./bin/rigol_measure -c 1 -p /dev/usbtmc0 get-current`
#Digital
DigitalI=`./bin/agilent_measure -c 2 -p /dev/ttyUSB0 -g 3 get-current`
#DigitalI=`./bin/rigol_measure -c 2 -p /dev/usbtmc0 get-current`

echo "Analog current is ${AnalogI}"
echo "Digital current is ${DigitalI}"

cd -

exit 0

