#ifndef READ_WRITE_PNG_H
#define READ_WRITE_PNG_H

#include <png.h>

typedef struct png_info_t {
	int width;
	int height;
	png_byte color_type;
	png_byte bit_depth;
	png_structp png_ptr;
	png_infop info_ptr;
} png_info_t;

void abort_(const char * s, ...);
void read_png_file(png_bytep **row_pointers, png_info_t *png_info, FILE* fp);
FILE* read_png_file_stats(char* file_name, png_info_t *png_info);
void write_png_file(char* file_name, png_bytep **row_pointers, png_info_t* png_info);

#endif
