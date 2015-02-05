#!/bin/bash

rsync -vcz --no-whole-file --block-size=$1 --progress $2 A \
	| awk '/sent/ {gsub(/,/,"", $2); gsub(/,/,"",$5); print $2 + $5; }'


