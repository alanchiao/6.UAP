PRODUCT = mpi_hello mpi_send_receive

build:
	make mpi_hello
	make mpi_send_receive

mpi_hello:
	mpicc mpi_hello.c -o hello

mpi_send_receive:
	mpicc mpi_send_receive.c -o send_receive

run:
	srun -N 2 ./${EXE}

clean:
	rm hello send_receive
