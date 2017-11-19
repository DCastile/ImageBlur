#include <stdio.h>
#include <stdlib.h>


// sizeof(char)  = 1
// sizeof(short) = 2
// sizeof(int)   = 4

// Assumptions
#define BMP_HEADER_SIZE_BYTES 14
#define BMP_DIB_HEADER_SIZE_BYTES 40
#define MAXIMUM_IMAGE_SIZE 256

#define BI_RGB 0

// Types
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

typedef struct struct_bmp_header {
    uchar signature[2];      //ID field
    uint size;               //Size of the BMP file
    ushort reserved1;        //Application specific
    ushort reserved2;        //Application specific
    uint offset_pixel_array; //Offset where the pixel array (bitmap data) can be found
} BMP_Header;

typedef struct struct_bip_header {
    uint header_size;       // size of DIB header
    uint width;             // width of image
    uint height;            // height of image
    ushort planes;          // # of planes
    ushort bpp;             // bits per pixel
    uint compression;       // compression
    uint img_size;          // size of image
    uint x_ppm;             // x pixels per meter
    uint y_ppm;             // y pixels per meter
    uint colors;            // colors in color table
    uint color_count;       // important color count
} BIP_Header;

typedef struct struct_pixel {
    uchar r;
    uchar g;
    uchar b;
} Pixel;

typedef struct struct_bmp_image {
    BMP_Header *header;
    BIP_Header *bip_header;
    Pixel *pixels;
} BMP_Image;

// Forward function declarations
BMP_Header *read_header(FILE *fp);
BIP_Header *read_bip_header(FILE *fp);
Pixel *read_pixel_data(FILE *fp, const uint *width, const uint *height);
BMP_Image *read_bmp(char *input_file_name);

void write_header(FILE *fp, BMP_Header *header);
void write_bip_header(FILE *fp, BIP_Header *header);
void write_pixel_data(FILE *fp, Pixel *pixels, const uint *width, const uint *height);
void write_bmp(char *file_name, BMP_Image *image);

void free_bmp_image(BMP_Image *bmp_image);

void print_header(BMP_Header *header);
void print_bip_header(BIP_Header *bip_header);
void print_pixel_data(BMP_Image *bmp_image);

void box_blur(BMP_Image *image);

void blur(Pixel *neighbors[9], Pixel *new_pixel);


void main(int argc, char *argv[]) {
    //char *input_file_name = argv[1];
    //char *output_file_name = argv[2];

    char *input_file_name = "test1puppy.bmp";
    BMP_Image *image = read_bmp(input_file_name);

    print_header(image->header);
    printf("\n");
    print_bip_header(image->bip_header);
    printf("\n");

//    print_pixel_data(image);

    box_blur(image);

//    print_pixel_data(image);

    write_bmp("out.bmp", image);

    free_bmp_image(image);
}


BMP_Header *read_header(FILE *fp) {
    //read bitmap file header (14 bytes)
    BMP_Header *header = malloc(sizeof(BMP_Header));
    fread(header->signature, sizeof(char) * 2, 1, fp);
    fread(&header->size, sizeof(int), 1, fp);
    fread(&header->reserved1, sizeof(short), 1, fp);
    fread(&header->reserved2, sizeof(short), 1, fp);
    fread(&header->offset_pixel_array, sizeof(int), 1, fp);
    return header;
}

BIP_Header *read_bip_header(FILE *fp) {
    BIP_Header *header = malloc(sizeof(BIP_Header));
    fread(&header->header_size, sizeof(uint), 1, fp);
    fread(&header->width, sizeof(uint), 1, fp);
    fread(&header->height, sizeof(uint), 1, fp);
    fread(&header->planes, sizeof(ushort), 1, fp);
    fread(&header->bpp, sizeof(ushort), 1, fp);
    fread(&header->compression, sizeof(uint), 1, fp);
    fread(&header->img_size, sizeof(uint), 1, fp);
    fread(&header->x_ppm, sizeof(uint), 1, fp);
    fread(&header->y_ppm, sizeof(uint), 1, fp);
    fread(&header->colors, sizeof(uint), 1, fp);
    fread(&header->color_count, sizeof(uint), 1, fp);
    return header;
}

Pixel *read_pixel_data(FILE *fp, const uint *width, const uint *height) {
    Pixel *pixels = malloc(sizeof(Pixel) * (*width) * (*height));
    Pixel *cursor = pixels;

    int padding = ((*width) * 3) % 4;

    for (int i = 0; i < *height; i++) {
        for (int j = 0; j < *width; j++) {
            fread(&cursor->b, sizeof(uchar), 1, fp);
            fread(&cursor->g, sizeof(uchar), 1, fp);
            fread(&cursor->r, sizeof(uchar), 1, fp);
            cursor++;
        }
        if (padding != 0) {
            fseek(fp, sizeof(char) * padding, SEEK_CUR);
        }
    }

    return pixels;
}

BMP_Image *read_bmp(char *input_file_name) {
    FILE *fp = fopen(input_file_name, "rb");

    BMP_Image *bmp_image = malloc(sizeof(BMP_Image));
    bmp_image->header = read_header(fp);
    bmp_image->bip_header = read_bip_header(fp);

    bmp_image->pixels = read_pixel_data(fp, &bmp_image->bip_header->width, &bmp_image->bip_header->height);

    fclose(fp);
    return bmp_image;
}


void write_header(FILE *fp, BMP_Header *header) {
    fwrite(header->signature, sizeof(char) * 2, 1, fp);
    fwrite(&header->size, sizeof(int), 1, fp);
    fwrite(&header->reserved1, sizeof(short), 1, fp);
    fwrite(&header->reserved2, sizeof(short), 1, fp);
    fwrite(&header->offset_pixel_array, sizeof(int), 1, fp);
}

void write_bip_header(FILE *fp, BIP_Header *header) {
    fwrite(&header->header_size, sizeof(uint), 1, fp);
    fwrite(&header->width, sizeof(uint), 1, fp);
    fwrite(&header->height, sizeof(uint), 1, fp);
    fwrite(&header->planes, sizeof(ushort), 1, fp);
    fwrite(&header->bpp, sizeof(ushort), 1, fp);
    fwrite(&header->compression, sizeof(uint), 1, fp);
    fwrite(&header->img_size, sizeof(uint), 1, fp);
    fwrite(&header->x_ppm, sizeof(uint), 1, fp);
    fwrite(&header->y_ppm, sizeof(uint), 1, fp);
    fwrite(&header->colors, sizeof(uint), 1, fp);
    fwrite(&header->color_count, sizeof(uint), 1, fp);
}

void write_pixel_data(FILE *fp, Pixel *pixels, const uint *width, const uint *height) {
    int padding = ((*width) * 3) % 4;

    for (int i = 0; i < *height; i++) {
        for (int j = 0; j < *width; j++) {
            fwrite(&pixels->b, sizeof(uchar), 1, fp);
            fwrite(&pixels->g, sizeof(uchar), 1, fp);
            fwrite(&pixels->r, sizeof(uchar), 1, fp);
            pixels++;
        }
        if (padding != 0) {
            for (int k = 0; k < padding; k++) {
                fwrite("", sizeof(uchar), 1, fp);
            }
        }
    }

}

void write_bmp(char *file_name, BMP_Image *image) {
    FILE *fp = fopen(file_name, "wb");

    write_header(fp, image->header);
    write_bip_header(fp, image->bip_header);
    write_pixel_data(fp, image->pixels, &image->bip_header->width, &image->bip_header->height);

    fclose(fp);
}


void free_bmp_image(BMP_Image *bmp_image) {
    free(bmp_image->header);
    free(bmp_image->bip_header);
    free(bmp_image->pixels);
    free(bmp_image);
}


void print_header(BMP_Header *header) {
    printf("signature:\t%c%c\n", header->signature[0], header->signature[1]);
    printf("size:\t\t%d\n", header->size);
    printf("reserved1:\t%d\n", header->reserved1);
    printf("reserved2:\t%d\n", header->reserved2);
    printf("offset:\t\t%d\n", header->offset_pixel_array);
}

void print_bip_header(BIP_Header *bip_header) {
    printf("header_size: %d\n", bip_header->header_size);
    printf("width: %d\n", bip_header->width);
    printf("height: %d\n", bip_header->height);
    printf("planes: %d\n", bip_header->planes);
    printf("bpp: %d\n", bip_header->bpp);
    printf("compression: %d\n", bip_header->compression);
    printf("img_size: %d\n", bip_header->img_size);
    printf("x_ppm: %d\n", bip_header->x_ppm);
    printf("y_ppm: %d\n", bip_header->y_ppm);
    printf("colors: %d\n", bip_header->colors);
    printf("color_count: %d\n", bip_header->color_count);
}

void print_pixel_data(BMP_Image *bmp_image) {
    Pixel *pixel_cursor = bmp_image->pixels;

    for (int i = 0; i < bmp_image->bip_header->height; i++) {
        for (int j = 0; j < bmp_image->bip_header->width; j++) {

            printf("%.2x%.2x%.2x\t", pixel_cursor->b, pixel_cursor->g, pixel_cursor->r);
            pixel_cursor++;
        }
        printf("\n");
    }
}


void box_blur(BMP_Image *image) {
    int width = image->bip_header->width;
    int height = image->bip_header->height;

    int row_jump = width; // * sizeof(Pixel); //TODO figure out why the jumps are * 3
    int col_jump = 1; //sizeof(Pixel);


    printf("Head: 0x%x\n",(unsigned int) image->pixels);
    printf("Len: 0x%x\n", (unsigned int) sizeof(Pixel) * width * height);

    Pixel *new_pixels = malloc(sizeof(Pixel) * image->bip_header->width * image->bip_header->height);
    Pixel *new_cursor = new_pixels;

    Pixel *cursor = image->pixels;

    Pixel *neighbors[9];
    //  [0][1][2]
    //  [3][4][5]
    //  [6][7][8]


    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

//            neighbors[0] = cursor - row_jump - col_jump;
//            neighbors[1] = cursor - row_jump;
//            neighbors[2] = cursor - row_jump + col_jump;
//
//            neighbors[3] = cursor - col_jump;
//            neighbors[4] = cursor;
//            neighbors[5] = cursor + col_jump;
//
//            neighbors[6] = cursor + row_jump - col_jump;
//            neighbors[7] = cursor + row_jump;
//            neighbors[8] = cursor + row_jump + col_jump;

            if(i == 0) {
                neighbors[0] = NULL;
                neighbors[1] = NULL;
                neighbors[2] = NULL;
            } else {
                neighbors[0] = j == 0 ? NULL :  cursor - row_jump - col_jump;
                neighbors[1] = cursor - row_jump;
                neighbors[2] = j == width - 1 ? NULL : cursor - row_jump + col_jump;
            }

            neighbors[3] = j == 0 ? NULL : cursor - col_jump;
            neighbors[4] = cursor;
            neighbors[5] = j == width - 1 ? NULL : cursor + col_jump;

            if (i == height - 1) {
                neighbors[6] = NULL;
                neighbors[7] = NULL;
                neighbors[8] = NULL;
            } else {
                neighbors[6] = j == 0 ? NULL : cursor + row_jump - col_jump;
                neighbors[7] = cursor + row_jump;
                neighbors[8] = j == width - 1 ? NULL : cursor + row_jump + col_jump;
            }

            blur(neighbors, new_cursor);

            cursor++;
            new_cursor++;
        }
        printf("");
    }

    free(image->pixels);
    image->pixels = new_pixels;
}


void blur(Pixel *neighbors[9], Pixel *new_pixel) {
    uint b = 0, g = 0, r = 0;

    uint count = 0;
    for(int i = 0; i < 9; i++) {
        if (neighbors[i] != NULL) {
            b += neighbors[i]->b;
            g += neighbors[i]->g;
            r += neighbors[i]->r;
            count++;
        }
    }
    new_pixel->b = b / count;
    new_pixel->g = g / count;
    new_pixel->r = r / count;
}