////////////////////////////////////////////////////////////////////////////////
//INCLUDES
#include <stdio.h>

//TODO: finish me


////////////////////////////////////////////////////////////////////////////////
//MACRO DEFINITIONS

#pragma warning(disable: 4996)

//problem assumptions
#define BMP_HEADER_SIZE_BYTES 14
#define BMP_DIB_HEADER_SIZE_BYTES 40
#define MAXIMUM_IMAGE_SIZE 256

//bmp compression methods
//none:
#define BI_RGB 0

//TODO: finish me


////////////////////////////////////////////////////////////////////////////////
//DATA STRUCTURES

struct BMP_Header {
	char signature[2];		//ID field
	int size;		//Size of the BMP file
	short reserved1;		//Application specific
	short reserved2;		//Application specific
	int offset_pixel_array;  //Offset where the pixel array (bitmap data) can be found
};

//TODO: finish me



////////////////////////////////////////////////////////////////////////////////
//MAIN PROGRAM CODE


//TODO: finish me




void main(int argc, char* argv[]) {

	//TODO: finish me


	//sample code to read first 14 bytes of BMP file format
	FILE* file = fopen("test2.bmp", "rb");
	struct BMP_Header header;

	//read bitmap file header (14 bytes)
	fread(&header.signature, sizeof(char)*2, 1, file);
	fread(&header.size, sizeof(int), 1, file);
	fread(&header.reserved1, sizeof(short), 1, file);
	fread(&header.reserved2, sizeof(short), 1, file);
	fread(&header.offset_pixel_array, sizeof(int), 1, file);

	printf("signature: %c%c\n", header.signature[0], header.signature[1]);
	printf("size: %d\n", header.size);
	printf("reserved1: %d\n", header.reserved1);
	printf("reserved2: %d\n", header.reserved2);
	printf("offset_pixel_array: %d\n", header.offset_pixel_array);

	fclose(file);
}