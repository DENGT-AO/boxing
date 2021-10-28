#!/bin/sh

MODEL=$1
OEM=$2
CFG_DIR=/usr/share/${MODEL}/${OEM}

if [ -e ${CFG_DIR}/nvram.txt ]; then
	cp -f ${CFG_DIR}/nvram.txt /nvram/nvram.txt
elif [ -e /usr/share/nvram.txt ]; then
        cp -f /usr/share/nvram.txt /nvram/nvram.txt
fi

cp -f /usr/share/modems.drv /nvram/

#if [ "$TYPE" == "model" ]; then
#fi	
#if [ "$TYPE" == "oem" ]; then
#	else

#	exit 0;
#	fi
#fi
#echo "using router_fix"
