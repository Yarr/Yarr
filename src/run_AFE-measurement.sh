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

OvrDrvSyn30OL=0
OvrDrvSyn35OL=0
OvrDrvSyn40OL=0
OvrDrvSyn40IL=0
OvrDrvSyn50IL=0
OvrDrvSyn55IL=0

OvrDrvLin30OL=300
OvrDrvLin35OL=300
OvrDrvLin40OL=300
OvrDrvLin40IL=200
OvrDrvLin50IL=200
OvrDrvLin55IL=200

OvrDrvDiff30OL=0
OvrDrvDiff35OL=0
OvrDrvDiff40OL=0
OvrDrvDiff40IL=0
OvrDrvDiff50IL=0
OvrDrvDiff55IL=0

if [ ${2} == "Sensor50" ]
then
TargetTh_IL=1200
TargetTh_OL=1500

OvrDrvSyn30OL=350
OvrDrvSyn35OL=230
OvrDrvSyn40OL=100
OvrDrvSyn40IL=0
OvrDrvSyn50IL=0
OvrDrvSyn55IL=0

OvrDrvLin30OL=600
OvrDrvLin35OL=600
OvrDrvLin40OL=500
OvrDrvLin40IL=400
OvrDrvLin50IL=350
OvrDrvLin55IL=300

OvrDrvDiff30OL=100
OvrDrvDiff35OL=70
OvrDrvDiff40OL=70
OvrDrvDiff40IL=0
OvrDrvDiff50IL=0
OvrDrvDiff55IL=0
fi

if [ ${2} == "Sensor25" ]
then
TargetTh_IL=1200
TargetTh_OL=1500

OvrDrvSyn30OL=350
OvrDrvSyn35OL=230
OvrDrvSyn40OL=100
OvrDrvSyn40IL=0
OvrDrvSyn50IL=0
OvrDrvSyn55IL=0

OvrDrvLin30OL=600
OvrDrvLin35OL=600
OvrDrvLin40OL=500
OvrDrvLin40IL=400
OvrDrvLin50IL=350
OvrDrvLin55IL=300

OvrDrvDiff30OL=100
OvrDrvDiff35OL=70
OvrDrvDiff40OL=70
OvrDrvDiff40IL=0
OvrDrvDiff50IL=0
OvrDrvDiff55IL=0
fi

echo "Target Threshold for inner layers is ${TargetTh_IL} and for outer layers ${TargetTh_OL}"

echo "Tune DIFF FE, Vddd trimed at 1.2 V"

./tune-rd53a_DIFF1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL50.json $1 $[${TargetTh_IL} - ${OvrDrvDiff40IL}]
sed -i "s/14,14,14,14,13,13,13,13,12,12,12,12,13,12,12,12,12/14,14,14,14,13,13,13,13,13,12,12,12,13,12,12,12,12/g" configs/scans/rd53a/diff_*.json
./tune-rd53a_DIFF1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL45.json $1 $[${TargetTh_IL} - ${OvrDrvDiff50IL}]
sed -i "s/14,14,14,14,13,13,13,13,13,12,12,12,13,12,12,12,12/12,12,12,12,11,11,11,11,11,10,10,10,11,10,10,10,10/g" configs/scans/rd53a/diff_*.json
./tune-rd53a_DIFF1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL30.json $1 $[${TargetTh_OL} - ${OvrDrvDiff30OL}]
sed -i "s/12,12,12,12,11,11,11,11,11,10,10,10,11,10,10,10,10/12,12,12,12,12,11,11,11,11,10,10,10,11,10,10,10,10/g" configs/scans/rd53a/diff_*.json
./tune-rd53a_DIFF1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL35.json $1 $[${TargetTh_OL} - ${OvrDrvDiff35OL}]
sed -i "s/12,12,12,12,12,11,11,11,11,10,10,10,11,10,10,10,10/14,14,14,14,13,13,13,13,12,12,12,12,13,12,12,12,12/g" configs/scans/rd53a/diff_*.json

echo "Tune SYNC FE, Vddd trimed at 1.2 V"

./tune-rd53a_SYN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL50.json $1 $[${TargetTh_IL} - ${OvrDrvSyn40IL}]
sed -i "s/11,11,11,11,11,12,12,12,12,12,12,12,12,12,11,11/10,10,10,10,10,11,10,10,11,11,11,11,11,11,10,10/g" configs/scans/rd53a/syn_*.json
./tune-rd53a_SYN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL45.json $1 $[${TargetTh_IL} - ${OvrDrvSyn50IL}]
sed -i "s/10,10,10,10,10,11,10,10,11,11,11,11,11,11,10,10/7,7,7,7,7,8,8,7,8,8,8,8,8,8,7,7/g" configs/scans/rd53a/syn_*.json
./tune-rd53a_SYN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL30.json $1 $[${TargetTh_OL} - ${OvrDrvSyn30OL}]
sed -i "s/7,7,7,7,7,8,8,7,8,8,8,8,8,8,7,7/8,8,9,9,9,9,9,9,9,9,10,9,9,9,9,9/g" configs/scans/rd53a/syn_*.json
./tune-rd53a_SYN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL35.json $1 $[${TargetTh_OL} - ${OvrDrvSyn35OL}]
sed -i "s/8,8,9,9,9,9,9,9,9,9,10,9,9,9,9,9/11,11,11,11,11,12,12,12,12,12,12,12,12,12,11,11/g" configs/scans/rd53a/syn_*.json

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

./tune-rd53a_LIN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL50.json $1 $[${TargetTh_IL} - ${OvrDrvLin40IL}]
sed -i "s/14,14,14,14,14,14,14,14,13,13,13,13,13,13,13,12,12/13,13,13,13,12,12,12,12,12,12,12,12,12,12,12,11,11/g" configs/scans/rd53a/lin_*.json
./tune-rd53a_LIN1000.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_IL45.json $1 $[${TargetTh_IL} - ${OvrDrvLin50IL}]
sed -i "s/13,13,13,13,12,12,12,12,12,12,12,12,12,12,12,11,11/12,12,12,12,12,12,12,11,11,11,11,11,11,11,11,10,10/g" configs/scans/rd53a/lin_*.json
./tune-rd53a_LIN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL30.json $1 $[${TargetTh_OL} - ${OvrDrvLin30OL}]
sed -i "s/12,12,12,12,12,12,12,11,11,11,11,11,11,11,11,10,10/13,13,13,13,12,12,12,12,12,12,12,12,11,11,11,11,11/g" configs/scans/rd53a/lin_*.json
./tune-rd53a_LIN1300.sh configs/controller/specCfg.json configs/connectivity/example_rd53a_OL35.json $1 $[${TargetTh_OL} - ${OvrDrvLin35OL}]
sed -i "s/13,13,13,13,12,12,12,12,12,12,12,12,11,11,11,11,11/14,14,14,14,14,14,14,14,13,13,13,13,13,13,13,12,12/g" configs/scans/rd53a/lin_*.json
