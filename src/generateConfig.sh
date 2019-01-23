#!/bin/bash
# Takes a config file and creates six config files with different settings for AFE review measurement
# Run: config ChipID SldoAnalogTrim SldoDigitalTrim BareChip/Sensor50/Sensor25 Cold/Warm
### ./generateConfig configs/rd53a_test.json 0x0786 28 20 BareChip Cold

if [ "$#" -ne 6 ]; then
	echo "You need to specify 6 arguments: initialConfig ChipID SldoAnalogTrim SldoDigitalTrim BareChip/Sensor50/Sensor25 Cold/Warm"
	exit
fi

echo "Original config is $1"
echo "ChipID is $2"
echo "SldoAnalogTrim is $3"
echo "SldoDigitalTrim is $4"
echo "This files are generated for $5 for testing at $6"

cp $1 configs/rd53a_${5}_OL30.json
cp $1 configs/rd53a_${5}_OL35.json
cp $1 configs/rd53a_${5}_OL40.json
cp $1 configs/rd53a_${5}_IL45.json
cp $1 configs/rd53a_${5}_IL50.json
cp $1 configs/rd53a_${5}_IL55.json

cp configs/connectivity/example_rd53a_setup.json configs/connectivity/example_rd53a_OL30.json
cp configs/connectivity/example_rd53a_setup.json configs/connectivity/example_rd53a_OL35.json
cp configs/connectivity/example_rd53a_setup.json configs/connectivity/example_rd53a_OL40.json
cp configs/connectivity/example_rd53a_setup.json configs/connectivity/example_rd53a_IL45.json
cp configs/connectivity/example_rd53a_setup.json configs/connectivity/example_rd53a_IL50.json
cp configs/connectivity/example_rd53a_setup.json configs/connectivity/example_rd53a_IL55.json

value=`grep configs configs/connectivity/example_rd53a_setup.json | grep -o "configs/.*.json"`
sed -i "s:${value}:configs/rd53a_${5}_OL30.json:g" configs/connectivity/example_rd53a_OL30.json
sed -i "s:${value}:configs/rd53a_${5}_OL35.json:g" configs/connectivity/example_rd53a_OL35.json
sed -i "s:${value}:configs/rd53a_${5}_OL40.json:g" configs/connectivity/example_rd53a_OL40.json
sed -i "s:${value}:configs/rd53a_${5}_IL45.json:g" configs/connectivity/example_rd53a_IL45.json
sed -i "s:${value}:configs/rd53a_${5}_IL50.json:g" configs/connectivity/example_rd53a_IL50.json
sed -i "s:${value}:configs/rd53a_${5}_IL55.json:g" configs/connectivity/example_rd53a_IL55.json

value=`grep SldoAnalogTrim configs/rd53a_test.json | grep -o '[0-9]*'`
sed -i "s/\"SldoAnalogTrim\": ${value}/\"SldoAnalogTrim\": ${3}/g" configs/rd53a_${5}_OL30.json
sed -i "s/\"SldoAnalogTrim\": ${value}/\"SldoAnalogTrim\": ${3}/g" configs/rd53a_${5}_OL35.json
sed -i "s/\"SldoAnalogTrim\": ${value}/\"SldoAnalogTrim\": ${3}/g" configs/rd53a_${5}_OL40.json
sed -i "s/\"SldoAnalogTrim\": ${value}/\"SldoAnalogTrim\": ${3}/g" configs/rd53a_${5}_IL45.json
sed -i "s/\"SldoAnalogTrim\": ${value}/\"SldoAnalogTrim\": ${3}/g" configs/rd53a_${5}_IL50.json
sed -i "s/\"SldoAnalogTrim\": ${value}/\"SldoAnalogTrim\": ${3}/g" configs/rd53a_${5}_IL55.json

value=`grep SldoDigitalTrim configs/rd53a_test.json | grep -o '[0-9]*'`
sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${4}/g" configs/rd53a_${5}_OL30.json
sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${4}/g" configs/rd53a_${5}_OL35.json
sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${4}/g" configs/rd53a_${5}_OL40.json
sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${4}/g" configs/rd53a_${5}_IL45.json
sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${4}/g" configs/rd53a_${5}_IL50.json
sed -i "s/\"SldoDigitalTrim\": ${value}/\"SldoDigitalTrim\": ${4}/g" configs/rd53a_${5}_IL55.json

sed -i "s/\"Name\": \"JohnDoe\"/\"Name\": \"${2}\"/g" configs/rd53a_${5}_OL30.json
sed -i "s/\"Name\": \"JohnDoe\"/\"Name\": \"${2}\"/g" configs/rd53a_${5}_OL35.json
sed -i "s/\"Name\": \"JohnDoe\"/\"Name\": \"${2}\"/g" configs/rd53a_${5}_OL40.json
sed -i "s/\"Name\": \"JohnDoe\"/\"Name\": \"${2}\"/g" configs/rd53a_${5}_IL45.json
sed -i "s/\"Name\": \"JohnDoe\"/\"Name\": \"${2}\"/g" configs/rd53a_${5}_IL50.json
sed -i "s/\"Name\": \"JohnDoe\"/\"Name\": \"${2}\"/g" configs/rd53a_${5}_IL55.json

#For BareChip Sensor50 and Sensor25
#LinFE
        value=`grep LinFcBias configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"LinFcBias\": ${value}/\"LinFcBias\": 20/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"LinFcBias\": ${value}/\"LinFcBias\": 20/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"LinFcBias\": ${value}/\"LinFcBias\": 20/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"LinFcBias\": ${value}/\"LinFcBias\": 20/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"LinFcBias\": ${value}/\"LinFcBias\": 20/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"LinFcBias\": ${value}/\"LinFcBias\": 20/g" configs/rd53a_${5}_IL55.json
        
        value=`grep LinComp configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"LinComp\": ${value}/\"LinComp\": 110/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"LinComp\": ${value}/\"LinComp\": 110/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"LinComp\": ${value}/\"LinComp\": 110/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"LinComp\": ${value}/\"LinComp\": 110/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"LinComp\": ${value}/\"LinComp\": 110/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"LinComp\": ${value}/\"LinComp\": 110/g" configs/rd53a_${5}_IL55.json
if [ ${6} == "Cold" ]
then
        value=`grep LinLdac configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 185/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 185/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 185/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 185/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 185/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 185/g" configs/rd53a_${5}_IL55.json
        value=`grep LinPaInBias configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 160/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 220/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 270/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 270/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 370/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 420/g" configs/rd53a_${5}_IL55.json
fi
if [ ${6} == "Warm" ]
then
        value=`grep LinLdac configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 130/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 130/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 130/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 130/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 130/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"LinLdac\": ${value}/\"LinLdac\": 130/g" configs/rd53a_${5}_IL55.json
        value=`grep LinPaInBias configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 140/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 200/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 250/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 250/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 350/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"LinPaInBias\": ${value}/\"LinPaInBias\": 400/g" configs/rd53a_${5}_IL55.json
fi
        value=`grep LinRefKrum configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"LinRefKrum\": ${value}/\"LinRefKrum\": 300/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"LinRefKrum\": ${value}/\"LinRefKrum\": 300/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"LinRefKrum\": ${value}/\"LinRefKrum\": 300/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"LinRefKrum\": ${value}/\"LinRefKrum\": 300/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"LinRefKrum\": ${value}/\"LinRefKrum\": 300/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"LinRefKrum\": ${value}/\"LinRefKrum\": 300/g" configs/rd53a_${5}_IL55.json
#DiffFE
        value=`grep DiffPrmp configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"DiffPrmp\": ${value}/\"DiffPrmp\": 383/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"DiffPrmp\": ${value}/\"DiffPrmp\": 511/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"DiffPrmp\": ${value}/\"DiffPrmp\": 639/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"DiffPrmp\": ${value}/\"DiffPrmp\": 639/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"DiffPrmp\": ${value}/\"DiffPrmp\": 895/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"DiffPrmp\": ${value}/\"DiffPrmp\": 1023/g" configs/rd53a_${5}_IL55.json
        value=`grep DiffFol configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"DiffFol\": ${value}/\"DiffFol\": 542/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"DiffFol\": ${value}/\"DiffFol\": 542/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"DiffFol\": ${value}/\"DiffFol\": 542/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"DiffFol\": ${value}/\"DiffFol\": 542/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"DiffFol\": ${value}/\"DiffFol\": 542/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"DiffFol\": ${value}/\"DiffFol\": 542/g" configs/rd53a_${5}_IL55.json
        value=`grep DiffPrecomp configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"DiffPrecomp\": ${value}/\"DiffPrecomp\": 512/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"DiffPrecomp\": ${value}/\"DiffPrecomp\": 512/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"DiffPrecomp\": ${value}/\"DiffPrecomp\": 512/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"DiffPrecomp\": ${value}/\"DiffPrecomp\": 512/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"DiffPrecomp\": ${value}/\"DiffPrecomp\": 512/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"DiffPrecomp\": ${value}/\"DiffPrecomp\": 512/g" configs/rd53a_${5}_IL55.json
        value=`grep DiffComp configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"DiffComp\": ${value}/\"DiffComp\": 700/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"DiffComp\": ${value}/\"DiffComp\": 700/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"DiffComp\": ${value}/\"DiffComp\": 700/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"DiffComp\": ${value}/\"DiffComp\": 700/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"DiffComp\": ${value}/\"DiffComp\": 700/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"DiffComp\": ${value}/\"DiffComp\": 700/g" configs/rd53a_${5}_IL55.json
        value=`grep DiffVth2 configs/rd53a_test.json | grep -o '[0-9]*' | sed -n 2p`
        sed -i "s/\"DiffVth2\": ${value}/\"DiffVth2\": 100/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"DiffVth2\": ${value}/\"DiffVth2\": 100/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"DiffVth2\": ${value}/\"DiffVth2\": 100/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"DiffVth2\": ${value}/\"DiffVth2\": 100/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"DiffVth2\": ${value}/\"DiffVth2\": 100/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"DiffVth2\": ${value}/\"DiffVth2\": 100/g" configs/rd53a_${5}_IL55.json
        value=`grep DiffLcc configs/rd53a_test.json | grep -o '[0-9]*' | sed -n 1p`
        sed -i "s/\"DiffLcc\": ${value}/\"DiffLcc\": 20/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"DiffLcc\": ${value}/\"DiffLcc\": 20/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"DiffLcc\": ${value}/\"DiffLcc\": 20/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"DiffLcc\": ${value}/\"DiffLcc\": 20/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"DiffLcc\": ${value}/\"DiffLcc\": 20/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"DiffLcc\": ${value}/\"DiffLcc\": 20/g" configs/rd53a_${5}_IL55.json


if [ ${5} == "BareChip" ]
then
        value=`grep SyncIbiasp1 configs/rd53a_test.json | grep -o '[0-9]*' | sed -n 2p`
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 50/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 60/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 75/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 75/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 90/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 115/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIbiasp2 configs/rd53a_test.json | grep -o '[0-9]*' | sed -n 2p`
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 75/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 90/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 110/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 110/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 140/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 170/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIbiasSf configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 40/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 50/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 50/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 50/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 80/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 80/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIbiasDisc configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 140/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 180/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 240/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 240/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 300/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 320/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIctrlSynct configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncVbl configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 380/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncVrefKrum configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_IL55.json
fi

if [ ${5} == "Sensor50" ]
then
        value=`grep SyncIbiasp1 configs/rd53a_test.json | grep -o '[0-9]*' | sed -n 2p`
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 50/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 60/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 75/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 80/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 100/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 115/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIbiasp2 configs/rd53a_test.json | grep -o '[0-9]*' | sed -n 2p`
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 75/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 90/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 110/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 120/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 150/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 170/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIbiasSf configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 40/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 50/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 50/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 50/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 80/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 80/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIbiasDisc configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 140/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 180/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 240/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 200/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 280/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 320/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIctrlSynct configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncVbl configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncVrefKrum configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_IL55.json
fi

if [ ${5} == "Sensor25" ]
then
        value=`grep SyncIbiasp1 configs/rd53a_test.json | grep -o '[0-9]*' | sed -n 2p`
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 50/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 60/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 60/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 70/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 110/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasp1\": ${value}/\"SyncIbiasp1\": 115/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIbiasp2 configs/rd53a_test.json | grep -o '[0-9]*' | sed -n 2p`
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 75/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 90/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 90/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 105/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 165/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasp2\": ${value}/\"SyncIbiasp2\": 170/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIbiasSf configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 45/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 50/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 50/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 50/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 80/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasSf\": ${value}/\"SyncIbiasSf\": 80/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIbiasDisc configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 140/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 180/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 180/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 250/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 240/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIbiasDisc\": ${value}/\"SyncIbiasDisc\": 280/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncIctrlSynct configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncIctrlSynct\": ${value}/\"SyncIctrlSynct\": 100/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncVbl configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncVbl\": ${value}/\"SyncVbl\": 360/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncVrefKrum configs/rd53a_test.json | grep -o '[0-9]*'`
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncVrefKrum\": ${value}/\"SyncVrefKrum\": 450/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncSelC4F configs/rd53a_test.json | grep -o '[0-9]*' | sed -n 2p`
        sed -i "s/\"SyncSelC4F\": ${value}/\"SyncSelC4F\": 0/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncSelC4F\": ${value}/\"SyncSelC4F\": 0/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncSelC4F\": ${value}/\"SyncSelC4F\": 1/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncSelC4F\": ${value}/\"SyncSelC4F\": 0/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncSelC4F\": ${value}/\"SyncSelC4F\": 0/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncSelC4F\": ${value}/\"SyncSelC4F\": 1/g" configs/rd53a_${5}_IL55.json
        value=`grep SyncSelC2F configs/rd53a_test.json | grep -o '[0-9]*' | sed -n 2p`
        sed -i "s/\"SyncSelC2F\": ${value}/\"SyncSelC2F\": 1/g" configs/rd53a_${5}_OL30.json
        sed -i "s/\"SyncSelC2F\": ${value}/\"SyncSelC2F\": 1/g" configs/rd53a_${5}_OL35.json
        sed -i "s/\"SyncSelC2F\": ${value}/\"SyncSelC2F\": 0/g" configs/rd53a_${5}_OL40.json
        sed -i "s/\"SyncSelC2F\": ${value}/\"SyncSelC2F\": 1/g" configs/rd53a_${5}_IL45.json
        sed -i "s/\"SyncSelC2F\": ${value}/\"SyncSelC2F\": 1/g" configs/rd53a_${5}_IL50.json
        sed -i "s/\"SyncSelC2F\": ${value}/\"SyncSelC2F\": 0/g" configs/rd53a_${5}_IL55.json

fi


