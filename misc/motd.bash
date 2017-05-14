#!/bin/bash

pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd -P`
popd > /dev/null

motdfile="$SCRIPTPATH/motdoffensive"

# Version sofisticada
#list=$(cowsay -l)
#files=${list##*:}

# Version bestia
files="head-in sodomized-sheep"
word=$(echo -e $files | tr ' ' '\n' | shuf -n 1)

fortune -o | cowsay -f $word > $motdfile
