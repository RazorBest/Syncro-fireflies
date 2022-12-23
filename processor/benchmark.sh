#!/bin/bash

make 

for n_threads in {1..24}; do
    export n_threads
    times=$({ time ./fireflies_omp ${n_threads} 5000; } 2>&1 >/dev/null)
    echo ${times} | awk '{printf "%s,%s,%s,%s\n",ENVIRON["n_threads"],$2,$4,$6}'
done
