build:
	make gb	
	make blur
	make distributed_blur

gb:	
	gcc gb.c -lpng16 -o gb

blur:
	gcc blur.c -lpng16 -o blur 

distributed_blur:
	mpicc distributed_blur.c -lpng16 -o distributed_blur

run:
	mpirun -N 3 ./distributed_blur rgb.png distributed_blur_out.png
	#./blur rgb.png blur_out.png

clean:
	rm gb blur distributed_blur

