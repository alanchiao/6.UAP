build:
	make read_write_png
	make blur
	make distributed_blur
	make compare

read_write_png:
	gcc read_write_png.c -lpng16 -c -o read_write_png.o

blur:
	gcc blur.c -lpng16 -o blur 

distributed_blur:
	mpicc distributed_blur.c -lpng16 -o distributed_blur read_write_png.o

compare:
	gcc compare.c -lpng16 -o compare -g -O0 -gdwarf-3 read_write_png.o

run_distributed_blur:
	mpirun -N 2 ./distributed_blur rgb.png distributed_blur_out.png

run_blur:
	./blur rgb.png blur_out.png

check:
	# checks to make sure that distributed version has correct output
	./compare distributed_blur_out.png blur_out.png

clean:
	rm blur distributed_blur compare read_write_png.o

