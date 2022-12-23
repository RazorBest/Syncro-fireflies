CC=g++

all: fireflies fireflies_omp fireflies_mpi fireflies_pthread

fireflies: sequential/main_sequential.cpp cells/cells.cpp server_connection/server_connection.cpp
	$(CC) $(CFLAGS) $^ -o $@

fireflies_omp: omp/main_omp.cpp
	$(CC) $(CFLAGS) $^ -o $@ -fopenmp

fireflies_pthread: pthread/main_pthread.cpp cells/cells.cpp server_connection/server_connection.cpp
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

.PHONY: fireflies_mpi
fireflies_mpi:
	$(MAKE) -C mpi/
	cp mpi/main_mpi fireflies_mpi

.PHONY: fireflies_hybrid
fireflies_mpi:
	$(MAKE) -C hybrid/
	cp hybrid/main_hybrid fireflies_hybrid
