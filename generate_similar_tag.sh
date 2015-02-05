#!/bin/bash

rm A/* B/*
mkdir tmp
cd tmp
git clone https://github.com/$1
echo $(basename $1)
cd $(basename $1)

#gcc, gcc-4_9_0-release, gcc-4_3_0-release
git checkout tags/$2
#find . -maxdepth 5 -type f -print0 | sort -z |  xargs -0 cat -- > ../../${1///}.new${2-1}

find . -type f -print0 | sort -z |  xargs -0 cat -- > ../../A/${1///}


git checkout tags/$3

#find . -maxdepth 5 -type f -print0 | sort -z |  xargs -0 cat -- > ../../${1///}.old${2-1}

find . -type f -print0 | sort -z |  xargs -0 cat -- > ../../B/${1///}

cd ../
#rm -rf tmp


