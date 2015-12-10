#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <mpi.h>
#include <time.h>

#include <unistd.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>
#include "read_write_png.h"
#include "distributed_blur.h"

#define BLUR_COMM_TAG 5



int world_rank;
int world_size;
/**
png_bytep * row_pointers;
png_bytep * row_pointers_post_bh; // bh(x,y)
png_bytep * row_pointers_post_bv; // bv(x,y)
**/

char * image;
char * row_pointers_post_bh; // bh(x,y)
char * row_pointers_post_bv; // bv(x,y)

void blur(png_info_t *png_info, distributed_info_t *distributed_info) 
{
	// horizontal blur : NEED REGION = HAVE REGION.
	for (int y = distributed_info->local_min_height; y < distributed_info->local_max_height; y++) {
		png_byte* row = row_pointers[y];
		png_byte* bh_row = row_pointers_post_bh[y];
		for (int x = distributed_info->local_min_width; x < distributed_info->local_max_width; x++) {
			png_byte* bh_ptr = &(bh_row[x*3]);
			if (x == distributed_info->local_min_width) {
				png_byte* row_ptr_b = &(row[x*3]);
				png_byte* row_ptr_c = &(row[(x+1)*3]);
				for (int i=0; i<=2; i++) {
					bh_ptr[i] = (row_ptr_b[i] + row_ptr_c[i])/2;
				}
			} else if (x == distributed_info->local_max_width - 1) {
				png_byte* row_ptr_a = &(row[(x-1)*3]);
				png_byte* row_ptr_b = &(row[x*3]);
				for (int i=0; i<=2; i++) {
					bh_ptr[i] = (row_ptr_a[i] + row_ptr_b[i])/2;
				}
			} else {
				// row_ptr pixels before, at, and after
				png_byte* row_ptr_a = &(row[(x-1)*3]);
				png_byte* row_ptr_b = &(row[x*3]);
				png_byte* row_ptr_c = &(row[(x+1)*3]);

				// iterate over r, g, and b
				for (int i=0; i<=2; i++) {
					bh_ptr[i] = (row_ptr_a[i] + row_ptr_b[i] + row_ptr_c[i])/3;
				}
			}
		}
	}

	// vertical blur : NEED REGION = HAVE REGION  + ONE LINE ABOVE AND BELOW.
	// Communication of Intersect(N, H) to rank - 1 and + 1.
	// Receiving necessary data from rank - 1 and + 1 also.
	png_bytep * top_row_pointers = (png_bytep *) malloc(sizeof(png_bytep));
	top_row_pointers[0] = (png_byte*) malloc(png_get_rowbytes(png_info->png_ptr,png_info->info_ptr));
	png_bytep * bottom_row_pointers = (png_bytep *) malloc(sizeof(png_bytep));
	bottom_row_pointers[0] = (png_byte*) malloc(png_get_rowbytes(png_info->png_ptr,png_info->info_ptr) + 1);

	if (world_rank != world_size - 1) {
		MPI_Send(&(row_pointers_post_bv[distributed_info->local_max_height - 1]), png_get_rowbytes(png_info->png_ptr,png_info->info_ptr),
				 MPI_BYTE, world_rank + 1, BLUR_COMM_TAG, 
    		     MPI_COMM_WORLD);
		MPI_Recv(top_row_pointers[0], png_get_rowbytes(png_info->png_ptr,png_info->info_ptr),
				 MPI_BYTE, world_rank + 1, BLUR_COMM_TAG, 
				 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	if (world_rank != 0) {
		MPI_Send(&(row_pointers_post_bv[distributed_info->local_min_height]), png_get_rowbytes(png_info->png_ptr,png_info->info_ptr),
				 MPI_BYTE, world_rank - 1, BLUR_COMM_TAG, 
				 MPI_COMM_WORLD);
		MPI_Recv(bottom_row_pointers[0], png_get_rowbytes(png_info->png_ptr, png_info->info_ptr),
				 MPI_BYTE, world_rank - 1, BLUR_COMM_TAG, 
				 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	// Actual computations
	for (int y = distributed_info->local_min_height; y < distributed_info->local_max_height; y++) {
		png_byte* bv_row  = row_pointers_post_bv[y];
		if (y == distributed_info->local_min_height) {
			for (int x = distributed_info->local_min_width; x < distributed_info->local_max_width; x++) {
				png_byte* bv_ptr = &(bv_row[x*3]);
				// iterate over bh rows needed
				for (int j = 0; j <= 1; j++) {
					// iterate over r, g, and b
					for (int i = 0; i <= 2; i++) {
						png_byte* bh_row_ptr = &(row_pointers_post_bh[y + j][x * 3]);
						bv_ptr[i] += bh_row_ptr[i]/3;
					}	
				}
			}
		} else if (y == distributed_info->local_max_height - 1) {
			for (int x = distributed_info->local_min_width; x < distributed_info->local_max_width; x++) {
				png_byte* bv_ptr = &(bv_row[x*3]);
				// iterate over bh rows needed
				for (int j =-1; j <= 0; j++) {
					// iterate over r, g, and b
					for (int i = 0; i <= 2; i++) {
						png_byte* bh_row_ptr = &(row_pointers_post_bh[y + j][x * 3]);
						bv_ptr[i] += bh_row_ptr[i]/3;
					}	
				}
			}
		} else {
			for (int x = distributed_info->local_min_width; x < distributed_info->local_max_width; x++) {
				png_byte* bv_ptr = &(bv_row[x*3]);
				// iterate over bh rows needed
				for (int j = -1; j <= 1; j++) {
					// iterate over r, g, and b
					for (int i = 0; i <= 2; i++) {
						png_byte* bh_row_ptr = &(row_pointers_post_bh[y + j][x * 3]);
						bv_ptr[i] += bh_row_ptr[i]/3;
					}	
				}
			}
		}
	}
	free(top_row_pointers[0]);
	free(bottom_row_pointers[0]);
	free(top_row_pointers);
	free(bottom_row_pointers);
}

////////////////////////////////////////////////////////////////
// Main setup

int main(int argc, char **argv)
{
	if (argc != 3)
		abort_("Usage: program_name <file_in> <file_out>");

	MPI_Init(&argc, &argv);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);

    printf( "Hello world from processor %s, rank %d of %d\n", processor_name, world_rank, world_size);

	// Read file + setup logic
	png_info_t* png_info = malloc(sizeof(png_info_t));
	FILE *fp = read_png_file_stats(argv[1], png_info);
	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * png_info->height);
	for (int y=0; y < png_info->height; y++)
		row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_info->png_ptr,png_info->info_ptr));
	row_pointers_post_bh = (png_bytep*) malloc(sizeof(png_bytep) * png_info->height);
	for (int y=0; y < png_info->height; y++)
		row_pointers_post_bh[y] = (png_byte*) malloc(png_get_rowbytes(png_info->png_ptr,png_info->info_ptr));
	row_pointers_post_bv = (png_bytep*) malloc(sizeof(png_bytep) * png_info->height);
	for (int y=0; y < png_info->height; y++)
		row_pointers_post_bv[y] = (png_byte*) malloc(png_get_rowbytes(png_info->png_ptr,png_info->info_ptr));
	read_png_file(&row_pointers, png_info, fp);

	int total_size = png_info->width * png_info->height * sizeof(char) * 3;
	image = malloc(total_size);
	memset(image, 0, total_size);

	// Distributed preparation logic
	// Distribute chunks of png
	int global_height = png_info->height;
	int global_width  = png_info->width;
	int local_min_height;
	int local_max_height;
	int local_min_width;
	int local_max_width;	
	if (world_rank < world_size) {
		local_min_height = global_height / world_size * (world_rank);
		local_max_height = (global_height / world_size * (world_rank + 1));
	} else { // last rank - takes extra
		local_min_height = global_height / world_size * (world_rank);
		local_max_height = global_height;
	}
	local_min_width = 0;
	local_max_width = global_width;

	distributed_info_t *distributed_info = malloc(sizeof(distributed_info_t));
	distributed_info->local_min_height = local_min_height;
	distributed_info->local_max_height = local_max_height;
	distributed_info->local_min_width = local_min_width;
	distributed_info->local_max_width = local_max_width;
	// Define Need and Have regions (N and H)
	// Blur logic
	
	struct timespec start;
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (int iter = 0; iter < 30; iter++) {
		blur(png_info, distributed_info); // region goes from local_min_height to local_max_height - 1
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	printf("average total time: %f seconds\n", ((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1e9)/30);
	
	// Write file + cleanup logic
	write_png_file(argv[2], &row_pointers_post_bv, png_info);

	free(distributed_info);
	for (int y = 0; y < png_info->height; y++) {
		free(row_pointers[y]);
		free(row_pointers_post_bh[y]);
		free(row_pointers_post_bv[y]);
	}
	free(row_pointers);
	free(row_pointers_post_bh);
	free(row_pointers_post_bv);
	free(png_info);

	MPI_Finalize();
	return 0;
}

