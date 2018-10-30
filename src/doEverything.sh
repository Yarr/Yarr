#!/bin/sh

source ../setup.sh

cd configs

./create_fei4b_4chip_config.sh $1

cd ../

pwd

./doScan.sh $1 thresholdscan

./doScan.sh $1 tune_globalthreshold

./doScan.sh $1 thresholdscan

./doScan.sh $1 tune_pixelthreshold

./doScan.sh $1 thresholdscan

./doScan.sh $1 tune_globalpreamp

./doScan.sh $1 totscan

./doScan.sh $1 tune_pixelpreamp

./doScan.sh $1 totscan

./doScan.sh $1 digitalscan

./doScan.sh $1 analogscan

./doScan.sh $1 thresholdscan

./doScan.sh $1 noisescan

#./doScan.sh $1 selftrigger
