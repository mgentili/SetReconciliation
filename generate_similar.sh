#!/bin/bash

mkdir tmp
cd tmp
git clone https://github.com/$1

FILE="$(ls | head -1)"
cd $FILE

git checkout master
find . -maxdepth 5 -type f -print0 | sort -z |  xargs -0 cat -- > ../../${1///}.new${2-1}

git checkout master~${2-1}

find . -maxdepth 5 -type f -print0 | sort -z |  xargs -0 cat -- > ../../${1///}.old${2-1}

cd ../
#rm -rf tmp


