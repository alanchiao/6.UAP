mpi_hello:
	mpicc mpi_hello.c -o hello

mpi_send_receive:
	mpicc mpi_send_receive.c -o send_receive

mpi_run:
	mpirun -np 16 ./send_receive

clean:
	rm hello send_receive
