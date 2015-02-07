#!/bin/bash

#sudo apt-get --assume-yes install libssl-dev
#sudo apt-get --assume-yes install libboost-all-dev
#sudo apt-get --assume-yes install python-pip
#sudo apt-get --assume-yes install libpng-dev
#sudo apt-get --assume-yes install libfreetype6-dev
#sudo apt-get --assume-yes install pkg-config
#sudo pip install matplotlib
#sudo apt-get --assume-yes install libprotobuf-dev
#
#sudo add-apt-repository --assume-yes ppa:cp/bug-fixes
#sudo apt-get --assume-yes update
#sudo apt-get --assume-yes upgrade
#
#make
#mkdir tmp/ A/ B/ plot/ bin/ obj/
#
python benchmarks.py -r --error-prob 0.01 --block-start 10 --block-end 1000
python benchmarks.py -r --error-prob 0.001 --block-start 10 --block-end 2000
python benchmarks.py -r --error-prob 0.0001 --block-start 10 --block-end 10000
python benchmarks.py -r --error-prob 0.00001 --block-start 10 --block-end 20000


python benchmarks.py -b --num-changes 1 --block-start 10 --block-end 20000
python benchmarks.py -b --num-changes 10 --block-start 10 --block-end 10000
python benchmarks.py -b --num-changes 100 --block-start 10 --block-end 1000
python benchmarks.py -b --num-changes 1000 --block-start 10 --block-end 1000
python benchmarks.py -b --num-changes 10000 --block-start 10 --block-end 1000
python benchmarks.py -b --num-changes 100000 --block-start 10 --block-end 1000

python benchmarks.py -a --project sharelatex/sharelatex --tag1 v0.1.1 --tag2 v0.1.0 --block-start 10 --block-end 10000

python benchmarks.py -a --project PredictionIO/PredictionIO --tag1 v0.8.1 --tag2 v0.8.0 --block-start 10 --block-end 10000
python benchmarks.py -a --project mongodb/mongo --tag1 r2.7.5 --tag2 r2.7.6 --block-start 10 --block-end 20000
python benchmarks.py -a --project emacs-mirror/emacs --tag1 emacs-24.1 --tag2 emacs-24.2 --block-start 10 --block-end 40000
