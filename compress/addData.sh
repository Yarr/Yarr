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
YARRSRC="$HOME/git/Bachelor/Yarr/src" # path to the Yarr source
YARRRAWDATA="${YARRSRC}/data/rawData" # path to the Yarr raw data folder


# Data files paths
DATAFOLDER="$HOME/git/Bachelor/Yarr/compress/data/"
UNCOMSIZECSV="${DATAFOLDER}uncompSize.csv" # path to the raw size data file
LZ4CSV="${DATAFOLDER}lz4.csv" # path to the standard LZ4 data file
LZ49CSV="${DATAFOLDER}lz49.csv" # path to the strong LZ4 data file
GZIPCSV="${DATAFOLDER}gzip.csv" # path to the standard GZIP data file


# Algorithms paths
ALGOFOLDER="$HOME/git/Bachelor/Yarr/compress/algorithms/"
LZ4="${ALGOFOLDER}lz4/lz4" # base
LZ4d=" -d" # decompression
LZ49=" -9" # strong and slow compression

GZIP="${ALGOFOLDER}gzip-1.2.4/gzip"

# ==================
# scan launch script
# * uncomment a line to use different scan *
#===================
SCAN="basicTotScan.sh"
#SCAN="basicDigitalScan.sh"
#SCAN="basicAnalogScan.sh"
#SCAN="basicThresholdScan.sh"

# 1st arg. is compression algo, 2nd is iteration value
# 3th is the output file, 4th for the strong comp param or the split param
# 5th if slit: size of the split

function compression (){

    for((i=1;i<=$2;i++));do # loop with the past argument
	
	# generate one raw data file
	cd ~/git/Bachelor/Yarr/src
	bash $SCAN

	cd $YARRRAWDATA # go to the raw data folder

	# save raw size
	if [ "$4" == "-s" ]||[ "$4" == "--split" ]
	then
	    printf "$5,"  >> $3
	else
	    printf "$(stat --printf="%s" rawData.dat)," >> $3
	fi
	
	#compression
	if [ "$4" == "$LZ49" ]
	then
	    STARTTIME=$(date +%s%N) # starts measure time [nanoseconds]
	    $1 $4 rawData.dat       # compress
	    STOPTIME=$(date +%s%N)  # stop measure time [nanoseconds]
	elif [ "$4" == "-s" ]||[ "$4" == "--split" ]
	then
	    
	    split -d --bytes=$5 rawData.dat
	    
	    STARTTIME=$(date +%s%N)
	    $1 -m --rm x*       # compress all the chunks, remove the original chuncks
	    STOPTIME=$(date +%s%N)
	else	    
	    STARTTIME=$(date +%s%N)
	    $1 rawData.dat       # compress
	    STOPTIME=$(date +%s%N)
	fi
	
	# save compressed size
	if [ "$1" == "$GZIP" ]
	then
	    printf "$(stat --printf="%s" rawData.dat.gz)," >> $3
	
	elif [ "$4" == "-s" ]||[ "$4" == "--split" ]
	then
	    for xcomp in ./x*
	    do
		printf "$(stat --printf="%s" $xcomp)," >> $3
	    done
	elif [ "$1" == "$LZ4" ]||[ "$1" == "$LZ49" ]
	then
	    printf "$(stat --printf="%s" rawData.dat.lz4)," >> $3
	fi
	

	# save the compression time in nanoseconds
	# use bc calculator to know the diff
	printf "$(echo "$STOPTIME - $STARTTIME" | bc)," >> $3

	
	if [ "$1" != "$GZIP" ] # As gzip does not keep the original file
	then
	    rm rawData.dat # removed to avoid trouble when uncompressing
	fi
	
	
	# decompression
	if [ "$1" == "$GZIP" ]
	then
	    STARTTIME=$(date +%s%N) # starts measure time [nanoseconds]
	    $1 -d rawData.dat.gz    # decompress
	    STOPTIME=$(date +%s%N)  # stop measure time [nanoseconds]
	
	elif [ "$4" == "-s" ]||[ "$4" == "--split" ]
	then
	    STARTTIME=$(date +%s%N)
	    $1 -md x*       # compress all the chunks, remove the original chuncks
	    STOPTIME=$(date +%s%N)
	elif [ "$1" == "$LZ4" ]||[ "$1" == "$LZ49" ]
	then
	    STARTTIME=$(date +%s%N) 
	    $1 -d rawData.dat.lz4 
	    STOPTIME=$(date +%s%N)
	fi
	
	    
	# save the decompression time in nanoseconds
	# use bc calculator to know the diff
	printf "$(echo "$STOPTIME - $STARTTIME" | bc)\n" >> $3
	
	rm rawData.dat*
	rm x*
    done

}

# ---------
# HELP text
# ---------
if [ -z "$1" ]||[ "$1" == "--help" ]||[ "$1" == "-h" ] 
then
    echo Usage: ./addData.sh [OPTION]...[ITERATIONS]...
    echo Add size measures from Yarr emulator to a CSV file.
    echo
    echo The ITERATIONS is the number of wanted data. 
    echo
    echo With no OPTION, but an ITERATIONS value,  no compression is applied and raw data size is stored.
    echo
    echo -e "The --split (-s) option need an extra value: the size of the entry data chunk."
    echo Put it right after the iteration value.
    echo
    echo -e "  -lz4                      standard LZ4 compression"
    echo -e "  -lz49                     strong but slow LZ4 compression"
    echo -e "  -gzip                     standard GZIP compression"
    echo -e "  -h,  --help, <nothing>    display this help and exit"
    echo -e "  -p,  --path               display the path to the useful files"
    echo -e "  -s,  --split              LZ4 compression with splited entry"
    echo
    echo When a COMPRESSION MODE is given, the raw size, compressed size, compression time and decompression time are saved.
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
   compression $LZ4 $2  $LZ4CSV


# ------------------------
# Strong and slow LZ4 compression
# ------------------------
elif [ "$1" == "-lz49" ]
then
    compression $LZ4 $2  $LZ49CSV $LZ49 


# ------------------------
# Stanrard GZIP compression
# ------------------------
elif [ "$1" == "-gzip" ]
then
    compression $GZIP $2  $GZIPCSV

    
# ------------------------
# LZ4 with entry splited
# ------------------------
elif [ "$1" == "-s" ]||[ "$1" == "--split" ]
then
    
    compression $LZ4 $2  $LZ4CSV $1 $3 
    
fi
#EOF
