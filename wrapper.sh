#!/usr/bin/env bash

if [ $# -ne 2 ] ; then
	echo -e "Usage : ./wrapper.sh /path/to/ac_model /dev/ttyACMn\n"
	exit 1
fi

HMMDIR=$1
GRAMMAR="./grammar.jsgf"

./roger -hmm $HMMDIR -jsgf $GRAMMAR \
		-pl_window 10\    # optimizations
		-kdmaxxbbi 16\    # optimizations 
		-kdmaxdepth 10\   # optimizations
		-ds 2\            # optimizations
		>$2
