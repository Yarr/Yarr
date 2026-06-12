#Rebuild YARR
#cd build
#make -j4
#cd ..
#Run eye diagram

################
#     Help     #
################
Help()
{
   # Display Help
   echo "## Core Column Script ##"
   echo
   echo "Usage: $0 [-h] [-r <controller config> -c <connectivity config>]"
   echo "options:"
   echo "-h                        Print this Help."
   echo "-c <controller config>    Path to connectivity config"
   echo "-r <connectivity config>  Path to controller config"
   echo
}

# Get the options
while getopts ":hr:c:" option; do
   case $option in
      h) # display Help
         Help
         exit;;
      r) controllerCfg=$OPTARG;;
      c) connectivityCfg=$OPTARG;;
   esac
done

if [[ "$controllerCfg" = "" || "$connectivityCfg" = "" ]]
then
  Help
  exit 1
fi



#Run 5 Combination Scans

echo "Running rd53b Column Test 5 Times" 
timeout 100 ./bin/rd53bColumnTest -r $controllerCfg -c $connectivityCfg -n 5 -w -e 1
for i in {1..5}
do
    echo "Running corecolumnscan" $i
    timeout 300 ./bin/scanConsole -r $controllerCfg -c $connectivityCfg -s configs/scans/rd53b/corecolumnscan.json 
done

