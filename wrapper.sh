#!/usr/bin/env bash
# pass the serial device name as the first argument to this script

if [ $# -ne 1 ] ; then
	echo -e "Usage : ./wrapper.sh /dev/ttyACMn\n"
	exit 1
fi

HMMDIR="./hub4wsj_sc_8kadapt/"
GRAMMAR="./grammar.jsgf"
./roger -hmm $HMMDIR -jsgf $GRAMMAR \
		-pl_window 10\
		-kdmaxxbbi 16\
		-kdmaxdepth 10\
		-ds 2\
		>$1
