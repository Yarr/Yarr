# =======================================
#
# Script to tests raw data
# and various compressions algorthms
#
# Conus Vincent
# 31.06.2017
#
# =======================================

#!/bin/bash

# -----------------
# variable for path
# -----------------
YARRSRC="$HOME/GIT/Bachelor_Project/Yarr/src" # path to the Yarr source
YARRRAWDATA="${YARRSRC}/data/rawData" # path to the Yarr raw data folder


# Data files paths
DATAFOLDER="$HOME/GIT/Bachelor_Project/Yarr/compress/data/"
UNCOMSIZECSV="${DATAFOLDER}uncompSize.csv" # path to the raw size data file
LZ4CSV="${DATAFOLDER}lz4.csv" # path to the standard LZ4 data file

# Algorithms paths
ALGOFOLDER="$HOME/GIT/Bachelor_Project/Yarr/compress/algorithms/"
LZ4="${ALGOFOLDER}lz4/lz4"


# ---------
# HELP text
# ---------
if [ -z "$1" ]||[ "$1" == "--help" ]||[ "$1" == "-h" ] 
then
    echo
    echo Usage: addData.sh [OPTION]...[ITERATIONS]...
    echo Add size measures from Yarr emulator to a CSV file.
    echo
    echo The ITERATIONS is the number of wanted data. 
    echo
    echo With no OPTION,  no compression is applied and raw data size is stored.
    echo
    echo -e "  -lz4                      standard LZ4 compression"
    echo -e "  -h,  --help, <nothing>    display this help and exit"
    echo -e "  -p,  --path               display the path to the useful files"
    echo
    echo When a COMPRESSION MODE is given, the raw size, compressed size, compression time and decompression time are saved.
    echo
    echo Open the source code to have details about the paths and relations.
    echo
    exit 1
fi
    
if [ "$1" == "-p" ]||[ "$1" == "--path" ]
then
    printf "Data are in: $DATAFOLDER \n"
    printf "Yarr source code is in: $YARRSRC \n"
    printf "The raw data are in: YARRRAWDATA \n"
	

# ------------------------
# Raw data size measurment
# ------------------------
elif [ $# -eq 1 ]
then
   for((i=1;i<=$1;i++));do # loop with the past argument
       # generate one raw data
       cd $YARRSRC
       bash basicTotScan.sh

       # print size of the data and add the value to the data file
       cd $YARRRAWDATA

       stat --printf="%s" rawData.dat >> $UNCOMSIZECSV
       echo -e  '\r' >> $UNCOMSIZECSV
    
       rm rawData.dat
   done
# ------------------------
# Standard LZ4 compression
# ------------------------
elif [ "$1" == "-lz4" ]
then
    for((i=1;i<=$2;i++));do # loop with the past argument
	# generate one raw data
       cd ~/GIT/Bachelor_Project/Yarr/src
       bash basicTotScan.sh

       cd $YARRRAWDATA # go to the raw data folder

       # save raw size
       printf "$(stat --printf="%s" rawData.dat)," >> $LZ4CSV

       #compression
       STARTTIME=$(date +%s%N) # starts measure time [nanoseconds]
       $LZ4 rawData.dat # compress
       STOPTIME=$(date +%s%N) # stop measure time [nanoseconds]

       # save compressed size
       printf "$(stat --printf="%s" rawData.dat.lz4)," >> $LZ4CSV

       # save the compression time in nanoseconds
       # use bc calculator to know the diff
       printf "$(echo "$STOPTIME - $STARTTIME" | bc)," >> $LZ4CSV

       rm rawData.dat
       
       # decompression
       STARTTIME=$(date +%s%N) # starts measure time [nanoseconds]
       $LZ4 -d rawData.dat.lz4 # decompress
       STOPTIME=$(date +%s%N) # stop measure time [nanoseconds]

       # save the decompression time in nanoseconds
       # use bc calculator to know the diff
       printf "$(echo "$STOPTIME - $STARTTIME" | bc)\n" >> $LZ4CSV
       
       rm rawData.dat*
       
    done
fi

#EOF
