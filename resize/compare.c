#include <stdlib.h>
#include <assert.h>
#include "read_write_png.h"

int main(int argc, char **argv)
{
	puts("running pixel-by-pixel checker");
	printf("file 1 : %s\n", argv[1]);
	printf("file 2 : %s\n", argv[2]);
	png_info_t *png_info_1 = malloc(sizeof(png_info_t));
	png_info_t *png_info_2 = malloc(sizeof(png_info_t));
	FILE* fp1 = read_png_file_stats(argv[1], png_info_1);
	FILE* fp2 = read_png_file_stats(argv[2], png_info_2);

	png_bytep* row_pointers_1 = (png_bytep*) malloc(sizeof(png_bytep) * png_info_1->height);
	for (int y = 0; y < png_info_1->height; y++)
		row_pointers_1[y] = (png_byte*) malloc(png_get_rowbytes(png_info_1->png_ptr,png_info_1->info_ptr));
	png_bytep* row_pointers_2 = (png_bytep*) malloc(sizeof(png_bytep) * png_info_2->height);
	for (int y = 0; y < png_info_2->height; y++)
		row_pointers_2[y] = (png_byte*) malloc(png_get_rowbytes(png_info_2->png_ptr,png_info_2->info_ptr));

	read_png_file(&row_pointers_1, png_info_1, fp1);
	read_png_file(&row_pointers_2, png_info_2, fp2);

	assert(png_info_1->height == png_info_2->height);
	assert(png_info_1->width  == png_info_2->width);

	int height =  png_info_1->height;
	int width  =  png_info_1->width;
	for (int y = 0; y < height; y++) {
		png_byte* row_1 = row_pointers_1[y];
		png_byte* row_2 = row_pointers_2[y];
		for (int x = 0; x < width; x++) {
			png_byte* ptr_1 = &(row_1[x*3]);
			png_byte* ptr_2 = &(row_2[x*3]);
			assert(ptr_1[0] == ptr_2[0]);
			assert(ptr_1[1] == ptr_2[1]);
			assert(ptr_1[2] == ptr_2[2]);
		}
	}

	return 0;
}
