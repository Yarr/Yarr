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

#change fine delay values
sed -i "s/9,9,8,8,8,8,8,8,8,7,7,6,7,6,6,7,6/9,8,8,8,8,7,7,7,7,7,7,6,7,6,6,6,6/g" configs/scans/rd53a/diff_intimeanalogscan.json
sed -i "s/9,9,8,8,8,8,8,8,8,7,7,6,7,6,6,7,6/9,8,8,8,8,7,7,7,7,7,7,6,7,6,6,6,6/g" configs/scans/rd53a/diff_intimethresholdscan.json
./tune-rd53a_DIFF1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL45.json $1 $[${TargetTh_IL}+50]
./measure_Current.sh
sed -i "s/9,8,8,8,8,7,7,7,7,7,7,6,7,6,6,6,6/9,8,8,8,8,8,8,7,8,7,7,6,7,6,6,7,6/g" configs/scans/rd53a/diff_intimeanalogscan.json
sed -i "s/9,8,8,8,8,7,7,7,7,7,7,6,7,6,6,6,6/9,8,8,8,8,8,8,7,8,7,7,6,7,6,6,7,6/g" configs/scans/rd53a/diff_intimethresholdscan.json
./tune-rd53a_DIFF1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL50.json $1 $[${TargetTh_IL}+50]
./measure_Current.sh
sed -i "s/9,8,8,8,8,8,8,7,8,7,7,6,7,6,6,7,6/7,7,6,7,7,6,6,6,6,6,6,5,6,5,5,5,5/g" configs/scans/rd53a/diff_intimeanalogscan.json
sed -i "s/9,8,8,8,8,8,8,7,8,7,7,6,7,6,6,7,6/7,7,6,7,7,6,6,6,6,6,6,5,6,5,5,5,5/g" configs/scans/rd53a/diff_intimethresholdscan.json
./tune-rd53a_DIFF1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL30.json $1 $[${TargetTh_OL}+50]
./measure_Current.sh
sed -i "s/7,7,6,7,7,6,6,6,6,6,6,5,6,5,5,5,5/7,7,6,6,7,6,6,6,6,6,6,5,6,5,5,5,5/g" configs/scans/rd53a/diff_intimeanalogscan.json
sed -i "s/7,7,6,7,7,6,6,6,6,6,6,5,6,5,5,5,5/7,7,6,6,7,6,6,6,6,6,6,5,6,5,5,5,5/g" configs/scans/rd53a/diff_intimethresholdscan.json
./tune-rd53a_DIFF1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL35.json $1 $[${TargetTh_OL}+50]
./measure_Current.sh
sed -i "s/7,7,6,6,7,6,6,6,6,6,6,5,6,5,5,5,5/9,9,8,8,8,8,8,8,8,7,7,6,7,6,6,7,6/g" configs/scans/rd53a/diff_intimeanalogscan.json
sed -i "s/7,7,6,6,7,6,6,6,6,6,6,5,6,5,5,5,5/9,9,8,8,8,8,8,8,8,7,7,6,7,6,6,7,6/g" configs/scans/rd53a/diff_intimethresholdscan.json

echo "Tune SYNC FE, Vddd trimed at 1.2 V"

sed -i "s/13,13,13,13,13,14,14,14,14,14,14,14,14,14,13,13/11,11,12,12,12,12,12,12,12,12,12,12,12,12,11,11/g" configs/scans/rd53a/syn_intimeanalogscan.json
sed -i "s/13,13,13,13,13,14,14,14,14,14,14,14,14,14,13,13/11,11,12,12,12,12,12,12,12,12,12,12,12,12,11,11/g" configs/scans/rd53a/syn_intimethresholdscan.json
./tune-rd53a_SYN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL45.json $1 ${TargetTh_IL}
./measure_Current.sh
sed -i "s/11,11,12,12,12,12,12,12,12,12,12,12,12,12,11,11/12,12,13,13,13,13,13,13,13,13,13,13,13,13,13,13/g" configs/scans/rd53a/syn_intimeanalogscan.json
sed -i "s/11,11,12,12,12,12,12,12,12,12,12,12,12,12,11,11/12,12,13,13,13,13,13,13,13,13,13,13,13,13,13,13/g" configs/scans/rd53a/syn_intimethresholdscan.json
./tune-rd53a_SYN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL50.json $1 ${TargetTh_IL}
./measure_Current.sh
sed -i "s/12,12,13,13,13,13,13,13,13,13,13,13,13,13,13,13/9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10/g" configs/scans/rd53a/syn_intimeanalogscan.json
sed -i "s/12,12,13,13,13,13,13,13,13,13,13,13,13,13,13,13/9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10/g" configs/scans/rd53a/syn_intimethresholdscan.json
./tune-rd53a_SYN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL30.json $1 ${TargetTh_OL}
./measure_Current.sh
sed -i "s/9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10/10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11/g" configs/scans/rd53a/syn_intimeanalogscan.json
sed -i "s/9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10/10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11/g" configs/scans/rd53a/syn_intimethresholdscan.json
./tune-rd53a_SYN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL35.json $1 ${TargetTh_OL}
./measure_Current.sh
sed -i "s/10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11/13,13,13,13,13,14,14,14,14,14,14,14,14,14,13,13/g" configs/scans/rd53a/syn_intimeanalogscan.json
sed -i "s/10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11/13,13,13,13,13,14,14,14,14,14,14,14,14,14,13,13/g" configs/scans/rd53a/syn_intimethresholdscan.json


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

sed -i "s/14,14,14,14,14,14,14,14,14,14,14,14,13,13,13,13,13/12,12,12,12,12,12,11,11,11,11,11,11,11,11,10,10,10/g" configs/scans/rd53a/lin_intimeanalogscan.json
sed -i "s/14,14,14,14,14,14,14,14,14,14,14,14,13,13,13,13,13/12,12,12,12,12,12,11,11,11,11,11,11,11,11,10,10,10/g" configs/scans/rd53a/lin_intimethresholdscan.json
./tune-rd53a_LIN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL45.json $1 ${TargetTh_IL}
./measure_Current.sh
sed -i "s/12,12,12,12,12,12,11,11,11,11,11,11,11,11,10,10,10/13,13,12,12,12,12,12,12,12,11,11,11,11,11,11,11,11/g" configs/scans/rd53a/lin_intimeanalogscan.json
sed -i "s/12,12,12,12,12,12,11,11,11,11,11,11,11,11,10,10,10/13,13,12,12,12,12,12,12,12,11,11,11,11,11,11,11,11/g" configs/scans/rd53a/lin_intimethresholdscan.json
./tune-rd53a_LIN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL50.json $1 ${TargetTh_IL}
./measure_Current.sh
sed -i "s/13,13,12,12,12,12,12,12,12,11,11,11,11,11,11,11,11/11,11,11,11,10,10,10,10,10,10,10,9,9,9,9,9,9/g" configs/scans/rd53a/lin_intimeanalogscan.json
sed -i "s/13,13,12,12,12,12,12,12,12,11,11,11,11,11,11,11,11/11,11,11,11,10,10,10,10,10,10,10,9,9,9,9,9,9/g" configs/scans/rd53a/lin_intimethresholdscan.json
./tune-rd53a_LIN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL30.json $1 ${TargetTh_OL}
./measure_Current.sh
sed -i "s/11,11,11,11,10,10,10,10,10,10,10,9,9,9,9,9,9/12,12,12,12,11,11,11,11,11,11,11,10,10,10,10,10,10/g" configs/scans/rd53a/lin_intimeanalogscan.json
sed -i "s/11,11,11,11,10,10,10,10,10,10,10,9,9,9,9,9,9/12,12,12,12,11,11,11,11,11,11,11,10,10,10,10,10,10/g" configs/scans/rd53a/lin_intimethresholdscan.json
./tune-rd53a_LIN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL35.json $1 ${TargetTh_OL}
./measure_Current.sh
sed -i "s/12,12,12,12,11,11,11,11,11,11,11,10,10,10,10,10,10/14,14,14,14,14,14,14,14,14,14,14,14,13,13,13,13,13/g" configs/scans/rd53a/lin_intimeanalogscan.json
sed -i "s/12,12,12,12,11,11,11,11,11,11,11,10,10,10,10,10,10/14,14,14,14,14,14,14,14,14,14,14,14,13,13,13,13,13/g" configs/scans/rd53a/lin_intimethresholdscan.json

