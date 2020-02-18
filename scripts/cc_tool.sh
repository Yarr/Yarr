#!/bin/bash

#  Makes using gcc code coverage a little easier to use
#  Binaries to be checkedfor coverage need to be in <binary_folder>.
#  They must have also been compiled and linked with the --coverage options and without any optimization.
#
#  Warning:  Can clear all .gcda files under <binary_folder> first,
#            so be sure they don't need to be preserved.

if [ $# -eq 1 ]; then
  echo
  echo "Usage:  $0 [-cc_i <binary_folder>] [-cc_o <output_folder>] [-cc_s <test_script_params>]"
  echo
  exit
fi

#  First 2 default parameters:
binary_folder=build/src/CMakeFiles
test_script_params="scripts/cc_scan_test.sh"

args=("$@")
iarg=-1
for arg in $*; do
  iarg=`expr $iarg + 1`
  next_iarg=`expr $iarg + 1`
  case $arg in
    -cc_i)
       binary_folder=${args[next_iarg]}
    ;;
    -cc_o)
       output_folder=${args[next_iarg]}
    ;;
    -cc_s)
       shift $next_iarg
       test_script_params="$*"
       break
    ;;
  esac
done

test_script=`echo $test_script_params | cut -d ' ' -f 1`
#  Default output_folder based on test_script
if [ x"$output_folder" == x ]; then
  output_folder="`basename -s .sh $test_script`"
fi

echo

echo binary_folder=$binary_folder
if [ ! -d $binary_folder ]; then
  echo
  echo Binary folder $binary_folder does not exist.
  echo Nothing done!
  echo
  exit 101
fi

echo test_script_params=$test_script_params
if [ ! -f $test_script ]; then
  echo
  echo Test script $test_script does not exist.
  echo Nothing done!
  echo
  exit 102
fi

echo output_folder=$output_folder

echo

if ( ( [ -d $binary_folder ] ) && ( [ `find $binary_folder -name "*.gcda" -print | wc -l` -gt 0 ] ) ); then
   echo "Are you sure you want to remove all already existing .gcda files in $binary_folder [Y|N] (N)?"
   read YN
   if [ `echo x"$YN" | cut -c 1-2` != xY ]; then
     echo
     echo Nothing done!
     echo
     exit 103
   fi
fi

if ( ( [ -d $output_folder ] ) && ( [ `ls -a $output_folder | wc -l` -gt 2 ] ) ); then
   echo "Are you sure you want to put results into the already existing non-empty contents of $output_folder [Y|N] (N)?"
   read YN
   if [ `echo x"$YN" | cut -c 1-2` == xY ]; then
     rm -rf $output_folder/*
   else
     echo
     echo Nothing done!
     echo
     exit 104
   fi
fi

lcov -z -d $binary_folder
ec=$?
if [ $ec -ne 0 ]; then
  exit $ec
fi

$test_script_params
ec=$?
if [ $ec -ne 0 ]; then
  exit $ec
fi

lcov -q -c -d $binary_folder -b . -o $output_folder.info --no-external
ec=$?
if [ $ec -ne 0 ]; then
  exit $ec
fi

lcov -q -r $output_folder.info "*src/external/src/*"  \
        -r $output_folder.info "*libUtil/include/spdlog*" \
        -r $output_folder.info "*libUtil/include/json.hpp" \
        -r $output_folder.info "*libUtil/include/catch.hpp" \
     -o ${output_folder}n.info
ec=$?
if [ $ec -ne 0 ]; then
  exit $ec
fi
mv ${output_folder}n.info $output_folder.info

genhtml $output_folder.info -o $output_folder
