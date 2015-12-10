# 6.UAP Distributed Halide Performance Experimentation Work
#
# MAIN
# BLUR - APPLICATION 1
# blur.c : serial version of blur
# distributed-blur.c : distributed version of blur
# v1: horizontal -> vertical blur. Wait for complete communication of Intersect(N, I) before proceeding.
# v2: horizontal -> vertical blur. Communicate small chunks of width / N, where N is tuned. Get performance numbers as results.
# v3: vertical -> horizontal blur. Communicate small chunks of width / N, where N is tuned. Get performance numbers as results.
#
# RESIZE - APPLICATION 2
# resize.c : serial version of resize
# distributed-resize: distributed version of resize
# v1: Wait for complete communication of Intersect (N, I) before proceeding.
# v2: Communicate small chunks of width / N, where N is tuned. Get performance numbers as results.
#:
# HELPER
# read-write-png.c : uses libpng to read and write to png files
# compare.c : used to compare two png files A and B and make sure that they are equivalent pixel by pixel
#
# RUNNING THINGS
# Distributed Halide. WITH-MPI=1, make-distributed-blur.
#											MV2-ENABLE-AFFINITY=0 srun ...... (width, height)
#	Distributed Blur.   make distributed-blur. make run-distributed-blur
#
# Current Runtimes. Image = width 768, height 1280
# - Distributed Blur - w/o array init optimization. : 0.009201 seconds
# - Distributed Halide.															: 
#
#
# HIGH LEVEL ANALYSIS - PERFORMANCE TRADEOFFS TO CONSIDER
#
# 1) MPI Distributed Logic
# - naive
# - communicating chunk by chunk and processing on what you have received so far
# - communicating whole in background and processing on what you have received so far
#
# 2) Performance Numbers for distributed / etc on actual Halide usage.
# - Explain results. Compute-at vs Compute-root
# - Explain results. no distribute vs distribute.



