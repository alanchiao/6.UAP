build:
	make blur
	make distributed_blur
	make compare

blur:
	gcc blur.c -lpng16 -o blur 

distributed_blur:
	mpicc distributed_blur.c -lpng16 -o distributed_blur

compare:
	gcc compare.c -lpng16 -o compare

run distributed_blur:
	mpirun -N 1 ./distributed_blur rgb.png distributed_blur_out.png

run blur:
	./blur rgb.png blur_out.png

check:
	# checks to make sure that distributed version has correct output
	./compare distributed_blur_out.png blur_out.png

clean:
	rm blur distributed_blur

