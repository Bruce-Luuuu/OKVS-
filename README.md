# OKVSplus: a smaller Oblivious Key-Value Store

## Introduction
In CRYPTO'21, Garimella et al. proposed the concept of Oblibious Key-Value Store (OKVS) in their [paper](https://eprint.iacr.org/2021/883) and gave a fastest OKVS construcion (3H-GCT) to date. Later, Raghuraman et al. improved 3H-GCT via algorithmic changes along with new techniques for obtaining both
asymptotic and tight concrete bounds on its failure probability.<br>
I extract the OKVS part from their [implementation](https://github.com/Visa-Research/volepsi) and further improve it in terms of rate.

## Specifications
- OS: Linux x64
- Language: C++
- Requires: libOTe


## Build
```shell
git clone https://github.com/Bruce-Luuuu/OKVSplus.git
cd OKVSplus
mkdir build && cd build
cmake ..
make
./main
```
