#!/bin/bash

./fireflies 16 16 20 1 > out1
./fireflies_omp 8 16 16 20 1 > out2
mpirun -np 17 --oversubscribe fireflies_mpi 4 4 16 16 20 1 > out3
./fireflies_pthread 8 4 2 16 16 20 1 > out4
mpirun -np 17 --oversubscribe fireflies_hybrid 4 4 4 16 16 20 1 > out5

diff out1 out2
diff out1 out3
diff out1 out4
#diff out1 out5

./fireflies 16 16 1000 1 > out1
./fireflies_omp 8 16 16 1000 1 > out2
mpirun -np 17 --oversubscribe fireflies_mpi 4 4 16 16 1000 1 > out3
./fireflies_pthread 8 4 2 16 16 1000 1 > out4

diff out1 out2
diff out1 out3
diff out1 out4

./fireflies 100 100 2000 1 > out1
./fireflies_omp 8 100 100 2000 1 > out2
mpirun -np 17 --oversubscribe fireflies_mpi 4 4 100 100 2000 1 > out3
./fireflies_pthread 8 4 2 100 100 2000 1 > out4

diff out1 out2
diff out1 out3
diff out1 out4
