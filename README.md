An implementation of [multi-party set reconciliation](http://arxiv.org/abs/1311.2037) using [Inverible Bloom Lookup Tables](http://www.eecs.harvard.edu/~michaelm/E210/IBLT.pdf). Handles an arbitrary number of parties (with optimizations for the two-party case), and multiple key formats (either primitives or `std::string`s of arbitrary length), both through template parameters. 

To test the multi-party IBLTS:
```bash
$ make
$ ./bin/multi_testing
```

Upper bounding the set difference is an important problem in set reconciliation. We implement the Strata Estimator designed [here](https://www.ics.uci.edu/~eppstein/pubs/EppGooUye-SIGCOMM-11.pdf). 

IBLTs and Strata Estimators are used as sub-component of our file synchronization protocol, which shares many similarities to the one explained in [this paper](http://cis.poly.edu/suel/papers/recon.pdf). 

To test the file synchronization protocol:
```bash
$ ./bin/file_sync_testing [file1] [file2]
```

Requirements:
1) Google's [Protocol Buffer](https://code.google.com/p/protobuf/), which we use for (de)serialization of messages

2) [zlib](http://www.zlib.net/), which we use for compression

3) [openssl](https://www.openssl.org/) which we use for SHA-1 hash.
```bash
$ sudo apt-get install libssl-dev
```

4) [boost](http://www.boost.org/)
```bash
$ sudo apt-get instal libboost-all-dev
```
This repo also contains implementations of basic [tabluation hashing](http://people.csail.mit.edu/mip/papers/charhash/charhash.pdf), "field" arithmetic, and document fingerprinting using [winnowing](http://theory.stanford.edu/~aiken/publications/papers/sigmod03.pdf). 

Each can be tested separately (refer to `Makefile` for appropriate commands).
