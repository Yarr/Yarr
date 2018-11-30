#!/bin/bash
#need to have defined connectivity configs: example_rd53a_IL30.json, example_rd53a_IL35.json, example_rd53a_OL45.json and example_rd53a_OL50.json
#if runing Cold, specify Vddd trim and BareChip/Sensor25/Sensor50
#Run ./run outputFolder BareChip/Sensor25/Sensor50 VdddTrim


if [ "$#" -lt 2 ]; then
        echo "You need to specify 3 arguments: PathToOutputFolder BareChip/Sensor50/Sensor25 SldoDigitalTrim"
        exit
fi

TargetTh_IL=1000
TargetTh_OL=1300

if [ ${2} == "Sensor50" ]
then
TargetTh_IL=1200
TargetTh_OL=1500
fi

if [ ${2} == "Sensor25" ]
then
TargetTh_IL=1200
TargetTh_OL=1500
fi

echo "Target Threshold for inner layers is ${TargetTh_IL} and for outer layers ${TargetTh_OL}"

echo "Tune DIFF FE, Vddd trimed at 1.2 V"

./tune-rd53a_DIFF1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL45.json $1 ${TargetTh_IL}
./measure_Current.sh
./tune-rd53a_DIFF1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL50.json $1 ${TargetTh_IL}
./measure_Current.sh
./tune-rd53a_DIFF1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL30.json $1 ${TargetTh_OL}
./measure_Current.sh
./tune-rd53a_DIFF1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL35.json $1 ${TargetTh_OL}
./measure_Current.sh

echo "Tune SYNC FE, Vddd trimed at 1.2 V"

./tune-rd53a_SYN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL45.json $1 ${TargetTh_IL}
./measure_Current.sh
./tune-rd53a_SYN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL50.json $1 ${TargetTh_IL}
./measure_Current.sh
./tune-rd53a_SYN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL30.json $1 ${TargetTh_OL}
./measure_Current.sh
./tune-rd53a_SYN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL35.json $1 ${TargetTh_OL}
./measure_Current.sh


echo "Tune LIN FE, Vddd trimed to 1.2 V"

if [ "$#" -gt 2 ]; then
	echo "Changing the Vddd trim to 1.3"
	value=`grep SldoDigitalTrim configs/rd53a_${2}_OL30.json | grep -o '[0-9]*'`
	sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${3}/g" configs/rd53a_${2}_OL30.json
        value=`grep SldoDigitalTrim configs/rd53a_${2}_OL35.json | grep -o '[0-9]*'`
	sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${3}/g" configs/rd53a_${2}_OL35.json
        value=`grep SldoDigitalTrim configs/rd53a_${2}_OL40.json | grep -o '[0-9]*'`
	sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${3}/g" configs/rd53a_${2}_OL40.json
        value=`grep SldoDigitalTrim configs/rd53a_${2}_IL45.json | grep -o '[0-9]*'`
	sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${3}/g" configs/rd53a_${2}_IL45.json
        value=`grep SldoDigitalTrim configs/rd53a_${2}_IL50.json | grep -o '[0-9]*'`
	sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${3}/g" configs/rd53a_${2}_IL50.json
        value=`grep SldoDigitalTrim configs/rd53a_${2}_IL55.json | grep -o '[0-9]*'`
	sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${3}/g" configs/rd53a_${2}_IL55.json
fi
 
./tune-rd53a_LIN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL45.json $1 ${TargetTh_IL}
./measure_Current.sh
./tune-rd53a_LIN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL50.json $1 ${TargetTh_IL}
./measure_Current.sh
./tune-rd53a_LIN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL30.json $1 ${TargetTh_OL}
./measure_Current.sh
./tune-rd53a_LIN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL35.json $1 ${TargetTh_OL}
./measure_Current.sh


