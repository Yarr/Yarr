#!/bin/bash

if [ ! `echo ${0} | grep bash` ]; then
    echo -e "Use 'source'"
    exit
fi


read -sp "Input mongodb account's username: " pass1
echo ""
read -sp "Input mongodb account's password: " pass2
echo ""
export username=${pass1}
export password=${pass2}

echo "[LDB]Username and password are saved."
