# 6.UAP Distributed Halide Performance Experimentation Work
#
# MAIN
# BLUR - APPLICATION 1
# blur.c : serial version of blur
# distributed-blur.c : distributed version of blur
# distributed-blur-segments-a.c : potential optimization. Send data between horizontal and vertical blur and receive in vertical blur when needed.
# distributed-blur-segments-b.c : potential optimization. Send data in horizontal blur when computed and receive in vertical blur when needed.
#
# HELPER
# read-write-png.c : uses libpng to read and write to png files
# compare.c : used to compare two png files A and B and make sure that they are equivalent pixel by pixel
#
# RUNNING THINGS
# Distributed Halide. WITH-MPI=1, make-distributed-blur.
#											MV2-ENABLE-AFFINITY=0 srun ...... (width, height)
#	Distributed Blur.   make distributed-blur. make run-distributed-blur
#
#
