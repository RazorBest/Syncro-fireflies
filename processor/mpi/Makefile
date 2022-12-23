GCC=mpic++
SRC=main_mpi.cpp master.cpp worker.cpp communication.cpp node.cpp ../cells/cells.cpp ../server_connection/server_connection.cpp
HEADERS=master.h worker.h communication.h node.h ../cells/cells.h ../server_connection/server_connection.h

main_mpi: $(SRC) $(HEADERS)
	$(GCC) $(SRC) -o $@
	
