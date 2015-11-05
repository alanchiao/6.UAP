build:
	make read_write_png
	make blur
	make compare

read_write_png:
	gcc read_write_png.c -lpng16 -c -o read_write_png.o

blur:
	gcc resize.c -lpng16 -o resize

distributed_resize:
	mpicc distributed_resize.c -lpng16 -o distributed_resize read_write_png.o

compare:
	gcc compare.c -lpng16 -o compare -g -O0 -gdwarf-3 read_write_png.o

run_distributed_resize:
	mpirun -N 2 ./distributed_resize rgb.png distributed_resize_out.png

run_resize:
	./resize rgb.png resize_out.png

check:
	# checks to make sure that distributed version has correct output
	./compare distributed_resize_out.png resize_out.png

clean:
	rm resize distributed_resize compare read_write_png.o

