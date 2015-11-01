#include <mpi.h>

#include <unistd.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>
#include "read_write_png.h"

png_bytep * row_pointers;
png_bytep * row_pointers_post_bh; // bh(x,y)
png_bytep * row_pointers_post_bv; // bv(x,y)

void blur(png_info_t *png_info) 
{
	// horizontal blur
	for (int y=0; y < png_info->height; y++) {
		png_byte* row = row_pointers[y];
		png_byte* bh_row = row_pointers_post_bh[y];
		for (int x=0; x < png_info->width; x++) {
			png_byte* bh_ptr = &(bh_row[x*3]);
			
			if (x == 0) {
				png_byte* row_ptr_b = &(row[x*3]);
				png_byte* row_ptr_c = &(row[(x+1)*3]);
				for (int i=0; i<=2; i++) {
					bh_ptr[i] = (row_ptr_b[i] + row_ptr_c[i])/2;
				}
			} else if (x == png_info->width - 1) {
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
				// printf("Pixel at position [ %d - %d ] has RGB values: %d - %d - %d\n",
			    //		   x, y, row_ptr_a[0], row_ptr_a[1], row_ptr_a[2]);

				// iterate over r, g, and b
				for (int i=0; i<=2; i++) {
					bh_ptr[i] = (row_ptr_a[i] + row_ptr_b[i] + row_ptr_c[i])/3;
				}
			}
		}
	}

	// vertical blur
	for (int y = 0; y < png_info->height; y++) {
		png_byte* bv_row  = row_pointers_post_bv[y];
		if (y == 0) {
			for (int x = 0; x < png_info->width; x++) {
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
		} else if (y == png_info->height - 1) {
			for (int x = 0; x < png_info->width; x++) {
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
			for (int x = 0; x < png_info->width; x++) {
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
}

////////////////////////////////////////////////////////////////
// Main setup

int main(int argc, char **argv)
{
	if (argc != 3)
		abort_("Usage: program_name <file_in> <file_out>");

	MPI_Init(&argc, &argv);
	
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);

    printf( "Hello world from processor %s, rank %d of %d\n", processor_name, world_rank, world_size);


	png_info_t* png_info = malloc(sizeof(png_info_t));
	FILE *fp = read_png_file_stats(argv[1], png_info);
	/* initial heap allocation */
	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * png_info->height);
	for (int y=0; y < png_info->height; y++)
		row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_info->png_ptr,png_info->info_ptr));
	row_pointers_post_bh = (png_bytep*) malloc(sizeof(png_bytep) * png_info->height);
	for (int y=0; y < png_info->height; y++)
		row_pointers_post_bh[y] = (png_byte*) malloc(png_get_rowbytes(png_info->png_ptr,png_info->info_ptr));
	row_pointers_post_bv = (png_bytep*) malloc(sizeof(png_bytep) * png_info->height);
	for (int y=0; y < png_info->height; y++)
		row_pointers_post_bv[y] = (png_byte*) malloc(png_get_rowbytes(png_info->png_ptr,png_info->info_ptr));
	read_png_file(&row_pointers, png_info);
	fclose(fp);

	blur(png_info);

	write_png_file(argv[2], &row_pointers_post_bv, png_info);
	/* cleanup heap allocation */
	for (int y = 0; y < png_info->height; y++) {
		free(row_pointers[y]);
		free(row_pointers_post_bh[y]);
		free(row_pointers_post_bv[y]);
	}
	free(row_pointers);
	free(row_pointers_post_bh);
	free(row_pointers_post_bv);

	MPI_Finalize();
	return 0;
}

