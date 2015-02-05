#!/bin/bash

sudo apt-get install libssl-dev
sudo apt-get instal libboost-all-dev

make
mkdir tmp/ A/ B/ plot/ bin/ obj/

python benchmarks.py -g -r --error-prob 0.01 --block-start 10 --block-end 1000
python benchmarks.py -g -r --error-prob 0.001 --block-start 10 --block-end 2000
python benchmarks.py -g -r --error-prob 0.0001 --block-start 10 --block-end 10000
python benchmarks.py -g -r --error-prob 0.00001 --block-start 10 --block-end 20000


python benchmarks.py -g -b --num-changes 1 --block-start 10 --block-end 20000
python benchmarks.py -g -b --num-changes 10 --block-start 10 --block-end 10000
python benchmarks.py -g -b --num-changes 100 --block-start 10 --block-end 1000
python benchmarks.py -g -b --num-changes 1000 --block-start 10 --block-end 1000
python benchmarks.py -g -b --num-changes 10000 --block-start 10 --block-end 1000
python benchmarks.py -g -b --num-changes 100000 --block-start 10 --block-end 1000

python benchmarks.py -g -a --project sharelatex/sharelatex --tag1 v0.1.1 --tag2 v0.1.0 --block-start 10 --block-end 10000
python benchmarks.py -g -a --project mongodb/mongo --tag1 r2.6.6 --tag2 r2.7.6 --block-start 10 --block-end 50000
python benchmarks.py -g -a --project PredictionIO/PredictionIO --tag1 v0.8.1 --tag2 v0.8.0 --block-start 10 --block-end 30000
python benchmarks.py -g -a --project emacs-mirror/emacs --tag1 emacs-24.1 --tag2 emacs-23.1 --block-start 10 --block-end 50000
