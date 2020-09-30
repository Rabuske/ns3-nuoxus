#!/bin/bash

NS_GLOBAL_VALUE="RngRun=2" ./waf --run scratch/DEFAULT
mkdir NuoxusDefault
mv *.txt ./NuoxusDefault/

NS_GLOBAL_VALUE="RngRun=2" ./waf --run scratch/LRU
mkdir NuoxusLRU
mv *.txt ./NuoxusLRU/

NS_GLOBAL_VALUE="RngRun=2" ./waf --run scratch/LFU
mkdir NuoxusLFU
mv *.txt ./NuoxusLFU/

NS_GLOBAL_VALUE="RngRun=2" ./waf --run scratch/NC
mkdir NuoxusNoCache
mv *.txt ./NuoxusNoCache/




