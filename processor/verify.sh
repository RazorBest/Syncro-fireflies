#!/bin/bash

./fireflies > out1
./fireflies_omp 8 > out2
diff out1 out2
