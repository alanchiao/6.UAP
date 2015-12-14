/////////////////////////////////////////////////////////
// Distributed Blur Version A - Receive border exchange data as
// segments. Send border exchange data as blocks.

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
#define NUM_BYTES_IN_PIXEL 3
#define NUM_SEGMENTS 1

int world_rank;
int world_size;

png_bytep * row_pointers;    // not used for computations, only in call to read_png_file
char * image; 							 // each pixel = 3 bytes = 3 chars. 
char * image_post_bh; // bh(x,y)
char * image_post_bv; // bv(x,y)

void blur(png_info_t *png_info, distributed_info_t *distributed_info) 
{
	int local_width = distributed_info->local_max_width - distributed_info->local_min_width;
	// horizontal blur : NEED REGION = HAVE REGION.
	for (int y = distributed_info->local_min_height; y < distributed_info->local_max_height; y++) {
		int row_offset = y * local_width;
		for (int x = distributed_info->local_min_width; x < distributed_info->local_max_width; x++) {
			int pixel_offset = row_offset + x * NUM_BYTES_IN_PIXEL;
			if (x == distributed_info->local_min_width) {
				for (int pixel_byte = 0; pixel_byte < NUM_BYTES_IN_PIXEL; pixel_byte++) {
					image_post_bh[pixel_offset + pixel_byte] = (image[pixel_offset + pixel_byte] + 										   \
																											image[pixel_offset + NUM_BYTES_IN_PIXEL + pixel_byte]) >> 1;
				}
			}	else if (x == distributed_info->local_max_width - 1) {
				for (int pixel_byte = 0; pixel_byte < NUM_BYTES_IN_PIXEL; pixel_byte++) {
					image_post_bh[pixel_offset + pixel_byte] = (image[pixel_offset - NUM_BYTES_IN_PIXEL + pixel_byte] +  \
																										  image[pixel_offset + pixel_byte] ) >> 1;
				}
			} else {
				for (int pixel_byte = 0; pixel_byte < NUM_BYTES_IN_PIXEL; pixel_byte++) {
					image_post_bh[pixel_offset + pixel_byte] = (image[pixel_offset - NUM_BYTES_IN_PIXEL + pixel_byte] +  \
																										  image[pixel_offset + pixel_byte] + 											 \
																											image[pixel_offset + NUM_BYTES_IN_PIXEL + pixel_byte]) / 3;
				}
			}
		}
	}

	// vertical blur : NEED REGION = HAVE REGION  + ONE LINE ABOVE AND BELOW.
	// Communication of Intersect(N, H) to rank - 1 and + 1.
	// Receiving necessary data from rank - 1 and + 1 also.
	char* top_row = malloc(local_width * sizeof(char) * NUM_BYTES_IN_PIXEL);
	char* bottom_row = malloc(local_width * sizeof(char) * NUM_BYTES_IN_PIXEL);
	int segment_size = local_width / NUM_SEGMENTS;

	if (world_rank != world_size - 1) {
		// Send data as NUM_SEGMENTS chunks
		for (int i = 0; i < NUM_SEGMENTS; i++) {
			MPI_Send(&(image_post_bh[(distributed_info->local_max_height - 1) * local_width + i * segment_size * NUM_BYTES_IN_PIXEL]), segment_size * NUM_BYTES_IN_PIXEL,
					 MPI_BYTE, world_rank + 1, BLUR_COMM_TAG, 
							 MPI_COMM_WORLD);
		}
		/**
		MPI_Recv(top_row, local_width * 3 ,
				 MPI_BYTE, world_rank + 1, BLUR_COMM_TAG, 
				 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		**/
	}

	if (world_rank != 0) {
		for (int i = 0; i < NUM_SEGMENTS; i++) {
			MPI_Send(&(image_post_bh[(distributed_info->local_min_height) * local_width + i * segment_size * NUM_BYTES_IN_PIXEL]), segment_size * NUM_BYTES_IN_PIXEL,
				 MPI_BYTE, world_rank - 1, BLUR_COMM_TAG, 
				 MPI_COMM_WORLD);
		}
		/**
		for (int i = 0; i < NUM_SEGMENTS; i++) {
			MPI_Recv(bottom_row + i * segment_size * NUM_BYTES_IN_PIXEL, segment_size * NUM_BYTES_IN_PIXEL,
					 MPI_BYTE, world_rank - 1, BLUR_COMM_TAG, 
					 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		**/
	}


	// Actual computations
	for (int y = distributed_info->local_min_height; y < distributed_info->local_max_height; y++) {
		int row_offset = y * local_width;
		if (y == distributed_info->local_min_height) {
			for (int x = distributed_info->local_min_width; x < distributed_info->local_max_width; x+= segment_size) {
				int pixel_offset = row_offset + x * NUM_BYTES_IN_PIXEL;
				if (world_rank != 0) {
					MPI_Recv(bottom_row + x * NUM_BYTES_IN_PIXEL, segment_size * NUM_BYTES_IN_PIXEL,
						MPI_BYTE, world_rank - 1, BLUR_COMM_TAG,
						MPI_COMM_WORLD, MPI_STATUS_IGNORE);	

					for (int segment_offset = 0; segment_offset < segment_size; segment_offset++) {
						pixel_offset += segment_offset * NUM_BYTES_IN_PIXEL;
						for (int pixel_byte = 0; pixel_byte < NUM_BYTES_IN_PIXEL; pixel_byte++) {
							image_post_bv[pixel_offset + pixel_byte] = (bottom_row[x * NUM_BYTES_IN_PIXEL + pixel_byte]    +    \
																													image_post_bh[pixel_offset + pixel_byte] + 							\
																													image_post_bh[pixel_offset + local_width + pixel_byte]) / 3;
						}
					}
				} else {
					for (int pixel_byte = 0; pixel_byte < NUM_BYTES_IN_PIXEL; pixel_byte++) {
						image_post_bv[pixel_offset + pixel_byte] = (image_post_bh[pixel_offset + pixel_byte] + 							\
																												image_post_bh[pixel_offset + local_width + pixel_byte]) >> 1;
					}
				}
			}
		} else if (y == distributed_info->local_max_height - 1) {
			// Receive chunk by chunk
			for (int x = distributed_info->local_min_width; x < distributed_info->local_max_width; x+= segment_size) {
				int pixel_offset = row_offset + x * NUM_BYTES_IN_PIXEL;
				if (world_rank != world_size - 1) {
					MPI_Recv(top_row + x * NUM_BYTES_IN_PIXEL, segment_size * NUM_BYTES_IN_PIXEL, 
						 MPI_BYTE, world_rank + 1, BLUR_COMM_TAG, 
						 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

					for (int segment_offset = 0; segment_offset < segment_size; segment_offset++) {
						pixel_offset += segment_offset * NUM_BYTES_IN_PIXEL;
						for (int pixel_byte = 0; pixel_byte < NUM_BYTES_IN_PIXEL; pixel_byte++) {
							image_post_bv[pixel_offset + pixel_byte] = (image_post_bh[pixel_offset - local_width + pixel_byte] +  \
																													image_post_bh[pixel_offset + pixel_byte] +                \
																													top_row[x * NUM_BYTES_IN_PIXEL + pixel_byte]) / 3;
						}
					}
				} else {
					for (int pixel_byte = 0; pixel_byte < NUM_BYTES_IN_PIXEL; pixel_byte++) {
						image_post_bv[pixel_offset + pixel_byte] = (image_post_bh[pixel_offset - local_width + pixel_byte] +  \
																												image_post_bh[pixel_offset + pixel_byte]) >> 1;
					}
				}
			}
		} else {
			for (int x = distributed_info->local_min_width; x < distributed_info->local_max_width; x++) {
				int pixel_offset = row_offset + x * NUM_BYTES_IN_PIXEL;
				for (int pixel_byte = 0; pixel_byte < NUM_BYTES_IN_PIXEL; pixel_byte++) {
					image_post_bv[pixel_offset + pixel_byte] = (image_post_bh[pixel_offset - local_width + pixel_byte] +  \
																											image_post_bh[pixel_offset + pixel_byte] + 							  \
																										  image_post_bh[pixel_offset + local_width + pixel_byte]) / 3;
				}
			}
		}
	}
		
	free(top_row);
	free(bottom_row);
}

////////////////////////////////////////////////////////////////
// Main setup

int main(int argc, char **argv)
{
	if (argc != 3)
		abort_("Usage: program_name <file_in> <file_out>");

	// MPI Initialization
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
	read_png_file(&row_pointers, png_info, fp);

	int total_size = png_info->width * png_info->height * sizeof(char) * 3;
	image = malloc(total_size);
	image_post_bh = malloc(total_size);
	image_post_bv = malloc(total_size);
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
	for (int iter = 0; iter < 1000; iter++) {
		blur(png_info, distributed_info); // region goes from local_min_height to local_max_height - 1
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	printf("average total time: %f seconds\n", ((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1e9)/1000);
	
	// Write file + cleanup logic
	// write_png_file(argv[2], &image_post_bv, png_info);

	free(distributed_info);
	free(image);
	free(row_pointers);
	free(image_post_bh);
	free(image_post_bv);
	free(png_info);

  MPI_Finalize();
	return 0;
}

