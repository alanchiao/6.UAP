#include <unistd.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>

void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;
png_bytep * row_pointers_post_bh; // bh(x,y)
png_bytep * row_pointers_post_bv; // bv(x,y)

void read_png_file(char* file_name)
{
	char header[8];    // 8 is the maximum size that can be checked

	/* open file and test for it being a png */
	FILE *fp = fopen(file_name, "rb");
	if (!fp)
		abort_("[read_png_file] File %s could not be opened for reading", file_name);
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
		abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
		abort_("[read_png_file] png_create_read_struct failed");

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		abort_("[read_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during init_io");

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	/* read file */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during read_image");

	// initialize data structures
	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (y=0; y<height; y++)
		row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

	row_pointers_post_bh = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (y=0; y<height; y++)
		row_pointers_post_bh[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

	row_pointers_post_bv = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (y=0; y<height; y++)
		row_pointers_post_bv[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

	png_read_image(png_ptr, row_pointers);

	fclose(fp);
}


void write_png_file(char* file_name)
{
	/* create file */
	FILE *fp = fopen(file_name, "wb");
	if (!fp)
		abort_("[write_png_file] File %s could not be opened for writing", file_name);

	/* initialize stuff */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
		abort_("[write_png_file] png_create_write_struct failed");

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		abort_("[write_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during init_io");

	png_init_io(png_ptr, fp);


	/* write header */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing header");

	png_set_IHDR(png_ptr, info_ptr, width, height,
				 bit_depth, color_type, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);


	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing bytes");

	png_write_image(png_ptr, row_pointers_post_bv);


	/* end write */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during end of write");

	png_write_end(png_ptr, NULL);

	/* cleanup heap allocation */
	for (y=0; y<height; y++)
		free(row_pointers[y]);
		free(row_pointers_post_bh[y]);
		free(row_pointers_post_bv[y]);
	free(row_pointers);
	free(row_pointers_post_bh[y]);
	free(row_pointers_post_bv[y]);

	fclose(fp);
}


void blur(void)
{

	// horizontal blur
	for (y=0; y<height; y++) {
		png_byte* row = row_pointers[y];
		png_byte* bh_row = row_pointers_post_bh[y];
		for (x=0; x<width; x++) {
			png_byte* bh_ptr = &(bh_row[x*3]);
			
			if (x == 0) {
				png_byte* row_ptr_b = &(row[x*3]);
				png_byte* row_ptr_c = &(row[(x+1)*3]);

				for (int i=0; i<=2; i++) {
					bh_ptr[i] = (row_ptr_b[i] + row_ptr_c[i])/2;
				}
			} else if (x == width - 1) {
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
	for (y = 0; y<height; y++) {
		png_byte* bv_row  = row_pointers_post_bv[y];
		if (y == 0) {
			for (x = 0; x < width; x++) {
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
		} else if (y == height - 1) {
			for (x = 0; x < width; x++) {
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
			for (x = 0; x < width; x++) {
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


int main(int argc, char **argv)
{
	if (argc != 3)
		abort_("Usage: program_name <file_in> <file_out>");

	read_png_file(argv[1]);
	blur();
	write_png_file(argv[2]);
	return 0;
}

