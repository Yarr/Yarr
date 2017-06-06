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
LZ49CSV="${DATAFOLDER}lz49.csv" # path to the strong LZ4 data file
GZIPCSV="${DATAFOLDER}gzip.csv" # path to the standard GZIP data file


# Algorithms paths
ALGOFOLDER="$HOME/GIT/Bachelor_Project/Yarr/compress/algorithms/"
LZ4="${ALGOFOLDER}lz4/lz4" # base
LZ4d=" -d" # decompression
LZ49=" -9" # strong and slow compression

GZIP="${ALGOFOLDER}gzip-1.2.4/gzip"

# scan launch script
SCAN="basicTotScan.sh"

# 1st arg. is compression algo, 2nd is iteration value
# 3rd is decompression param, 4th is the output file
# 5th for the strong comp param

function compression (){

    printf "compression\n"
    for((i=1;i<=$2;i++));do # loop with the past argument
	printf "loop\n"
	# generate one raw data
	cd ~/GIT/Bachelor_Project/Yarr/src
	bash $SCAN

	cd $YARRRAWDATA # go to the raw data folder

	# save raw size
	printf "$(stat --printf="%s" rawData.dat)," >> $4

	#compression
	STARTTIME=$(date +%s%N) # starts measure time [nanoseconds]
	$1 $5 rawData.dat # compress
	STOPTIME=$(date +%s%N) # stop measure time [nanoseconds]

	# save compressed size
	if [ $1 == "-gzip" ]
	then
	    printf "$(stat --printf="%s" rawData.dat.gz)," >> $4
	elif [ $1 == "-lz4" ]||[ $1 == "-lz49" ]
	then
	    printf "$(stat --printf="%s" rawData.dat.lz4)," >> $4
	fi
	

	# save the compression time in nanoseconds
	# use bc calculator to know the diff
	printf "$(echo "$STOPTIME - $STARTTIME" | bc)," >> $4
	
	if [ $1 != "-gzip" ] # As gzip does not keep the original file
	then
	    rm rawData.dat # removed to avoid trouble when uncompressing
	fi
	
       
	# decompression
	if [ $1 == "-gzip" ]
	then
	    STARTTIME=$(date +%s%N) # starts measure time [nanoseconds]
	    $1 $3 rawData.dat.gz # decompress
	    STOPTIME=$(date +%s%N) # stop measure time [nanoseconds]
	elif [ $1 == "-lz4" ]||[ $1 == "-lz49" ]
	then
	    STARTTIME=$(date +%s%N) # starts measure time [nanoseconds]
	    $1 $3 rawData.dat.lz4 # decompress
	    STOPTIME=$(date +%s%N) # stop measure time [nanoseconds]
	fi
	
	    
	# save the decompression time in nanoseconds
	# use bc calculator to know the diff
	printf "$(echo "$STOPTIME - $STARTTIME" | bc)\n" >> $4
	
	rm rawData.dat*
       
    done

}

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
    echo With no OPTION, but an ITERATIONS value,  no compression is applied and raw data size is stored.
    echo
    echo -e "  -lz4                      standard LZ4 compression"
    echo -e "  -lz49                     strong but slow LZ4 compression"
    echo -e "  -gzip                     standard GZIP compression"
    echo -e "  -h,  --help, <nothing>    display this help and exit"
    echo -e "  -p,  --path               display the path to the useful files"
    echo
    echo When a COMPRESSION MODE is given, the raw size, compressed size, compression time and decompression time are saved.
    echo
    exit 1
fi
    
if [ "$1" == "-p" ]||[ "$1" == "--path" ]
then
    printf "Data are in: $DATAFOLDER \n"
    printf "Yarr source code is in: $YARRSRC \n"
    printf "The raw data are in: $YARRRAWDATA \n"
	

# ------------------------
# Raw data size measurment
# ------------------------
elif [ $# -eq 1 ]
then
   for((i=1;i<=$1;i++));do # loop with the past argument
       # generate one raw data
       cd $YARRSRC
       bash $SCAN

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
   compression $LZ4 $2 $LZ4d $LZ4CSV


# ------------------------
# Strong and slow LZ4 compression
# ------------------------
elif [ "$1" == "-lz49" ]
then
   compression $LZ4 $2 $LZ4d $LZ49CSV $LZ49


# ------------------------
# Stanrard GZIP compression
# ------------------------
elif [ "$1" == "-gzip" ]
then
   compression $GZIP $2 $LZ4d $GZIPCSV


fi

#EOF
