build:
	make read_write_png
	make blur
	make distributed_blur
	make distributed_blur_segments
	make compare

read_write_png:
	gcc read_write_png.c -lpng12 -c -o read_write_png.o

blur:
	gcc -std=c99 blur.c -lpng12 -o blur 

distributed_blur:
	mpicc distributed_blur.c -std=c99 -lpng12 -lrt -o distributed_blur read_write_png.o

distributed_blur_segments:
	mpicc distributed_blur_segments.c -std=c99 -lpng12 -o distributed_blur_segments read_write_png.o

compare:
	gcc compare.c -std=c99 -lpng12 -o compare -g -O0 -gdwarf-3 read_write_png.o

run_distributed_blur:
	srun --exclusive -N 2 ./distributed_blur rgb.png distributed_blur_out.png
	
run_distributed_blur_segments:
	srun --exclusive -N 2 ./distributed_blur_segments rgb.png distributed_blur_segments_out.png

run_blur:
	./blur rgb.png blur_out.png

check:
	# checks to make sure that distributed version has correct output
	./compare distributed_blur_out.png blur_out.png

clean:
	rm blur distributed_blur compare read_write_png.o distributed_blur_segments

