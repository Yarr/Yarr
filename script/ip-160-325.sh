#!/bin/bash

DIR="../ip-cores/kintex7/"

DEV160="xc7k160t"
DEV325="xc7k325t"

for FILE in $(find $DIR -iname '*.xci' -o -iname '*.prj'); do
     # echo $FILE
     sed -i -e "s/$DEV160/$DEV325/g" $FILE
done
