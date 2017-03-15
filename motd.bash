#!/bin/bash

motdfile="motd"

list=$(cowsay -l)
files=${list##*:}

#files="head-in sodomized-sheep"
word=$(echo -e $files | tr ' ' '\n' | shuf -n 1)

fortune -o | cowsay -f $word > $motdfile
