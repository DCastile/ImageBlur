#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


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

typedef struct struct_image_chunk {
    Pixel *pixels;
    uint chunk_height;
    uint chunk_width;
    uint image_height;
    uint image_width;
    uint x_part;
    uint y_part;
} ImageChunk;

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

void box_blur(BMP_Image *image, uint num_threads);
ImageChunk *partition_image(BMP_Image *image, uint num_partitions);
ImageChunk *blur_partition(ImageChunk *chunk);
Pixel *reassemble_partitions(ImageChunk *chunks, uint num_partitions);
void blur(Pixel *neighbors[9], Pixel *new_pixel);


int main(int argc, char *argv[]) {
    char *input_file_name = argv[1];
    char *output_file_name = argv[2];

//    char *input_file_name = "test1puppy.bmp";
    BMP_Image *image = read_bmp(input_file_name);

    print_header(image->header);
    printf("\n");
    print_bip_header(image->bip_header);
    printf("\n");


    box_blur(image, 4);


    write_bmp(output_file_name, image);

    free_bmp_image(image);
    return 1;
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


void box_blur(BMP_Image *image, uint num_threads) {
    ImageChunk *partitions = partition_image(image, num_threads);

    pthread_t threads[num_threads];
    ImageChunk *partition_cursor = partitions;
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, (void *) blur_partition, (void *) partition_cursor);

//        blur_partition(partition_cursor);
        partition_cursor++;
    }

    partition_cursor = partitions;
    for (int j = 0; j < num_threads; j++) {
        pthread_join(threads[j], NULL);

        partition_cursor++;
    }

    free(image->pixels);
    image->pixels = reassemble_partitions(partitions, num_threads);

}


ImageChunk *partition_image(BMP_Image *image, uint num_partitions) {
    uint width = image->bip_header->width;
    uint height = image->bip_header->height;

    uint row_jump = width; // * sizeof(Pixel); //PITFALL!!
    uint col_jump = 1; // sizeof(Pixel);

    Pixel *pixels = image->pixels;

    ImageChunk *partitions = malloc(sizeof(ImageChunk) * num_partitions);
    ImageChunk *partition_cursor = partitions;

    for (int i = 0; i < num_partitions; i++) {
        uint x_part = 0, y_part = 0;
        switch (i) { // hard coded for 4
            case 0:
                y_part = 0;
                x_part = 0;
                break;
            case 1:
                y_part = 0;
                x_part = 1;
                break;
            case 2:
                y_part = 1;
                x_part = 0;
                break;
            case 3:
                y_part = 1;
                x_part = 1;
                break;
        }

        // TODO: what about odd height/width
        uint chunk_height = 1 + (height / 2);
        uint chunk_width = 1 + (width / 2);

        partition_cursor->chunk_height = chunk_height;
        partition_cursor->chunk_width = chunk_width;


        partition_cursor->image_height = height;
        partition_cursor->image_width = width;

        partition_cursor->x_part = x_part;
        partition_cursor->y_part = y_part;

        partition_cursor->pixels = malloc(sizeof(Pixel) * chunk_height * chunk_width);
        Pixel *new_cursor = partition_cursor->pixels;

        for (int j = 0; j < chunk_height; j++) {
            for (int k = 0; k < chunk_width; k++) {
                uint x = (k + (chunk_width * x_part));
                uint y = (j + (chunk_height * y_part));
                x += (x_part == 0) ? 0 : -2;
                y += (y_part == 0) ? 0 : -2;


                *new_cursor = *(pixels + (row_jump * y) + (col_jump * x));
//                printf("%u, %u\t\t%u %u %u\n", x, y, new_cursor->r, new_cursor->g, new_cursor->b);
                new_cursor++;
            }
        }
        partition_cursor++;
    }
    return partitions;
}

ImageChunk *blur_partition(ImageChunk *chunk) {
    uint width = chunk->chunk_width;
    uint height = chunk->chunk_height;


    uint row_jump = width; // * sizeof(Pixel); //PITFALL!!
    uint col_jump = 1; //sizeof(Pixel);

    // trim the fat
    chunk->chunk_height -= 1;
    chunk->chunk_width -= 1;

    Pixel *new_pixels = malloc(sizeof(Pixel) * chunk->chunk_width * chunk->chunk_height);
    Pixel *new_cursor = new_pixels;

    Pixel *head = chunk->pixels;
    Pixel *cursor;

    Pixel *neighbors[9];
    //  [0][1][2]
    //  [3][4][5]
    //  [6][7][8]

    uint x_part = chunk->x_part;
    uint y_part = chunk->y_part;

    uint global_height = chunk->image_height;
    uint global_width = chunk->image_width;


    for (int y = 0; y < chunk->chunk_height; y++) {
        for (int x = 0; x < chunk->chunk_width; x++) {

            int x_offset = (x_part == 0) ? 0 : 1;
            int y_offset = (y_part == 0) ? 0 : row_jump;

            cursor = head + (y * row_jump + y_offset) + (x * col_jump + x_offset); // point cursor to same pixel(relatively) as new_cursor
//            cursor = head + (y * row_jump) + (x * col_jump); // point cursor to same pixel(relatively) as new_cursor

            uint global_y = y + (y_part * chunk->chunk_height);
            uint global_x = x + (x_part * chunk->chunk_width);
//            printf("%u, %u\t\t%u\t%u\t%u\n", global_x, global_y, cursor->r, cursor->g, cursor->b);


            if (x == 0 & x_part == 1) {
                printf("");
            }
            if (y == 0 & y_part == 1) {
                printf("");
            }



            if (global_y == 0) {
                neighbors[0] = NULL;
                neighbors[1] = NULL;
                neighbors[2] = NULL;
            } else {
                neighbors[0] = global_x == 0 ? NULL : cursor - row_jump - col_jump;
                neighbors[1] = cursor - row_jump;
                neighbors[2] = global_x == global_width - 1 ? NULL : cursor - row_jump + col_jump;
            }

            neighbors[3] = global_x == 0 ? NULL : cursor - col_jump;
            neighbors[4] = cursor;
            neighbors[5] = global_x == global_width - 1 ? NULL : cursor + col_jump;

            if (global_y == global_height - 1) {
                neighbors[6] = NULL;
                neighbors[7] = NULL;
                neighbors[8] = NULL;
            } else {
                neighbors[6] = global_x == 0 ? NULL : cursor + row_jump - col_jump;
                neighbors[7] = cursor + row_jump;
                neighbors[8] = global_x == global_width - 1 ? NULL : cursor + row_jump + col_jump;
            }

            blur(neighbors, new_cursor);

//            printf("%u, %u\t%u\t%u\t%u\n", global_x, global_y, new_cursor->r, new_cursor->g, new_cursor->b);


//            *new_cursor = *cursor;

            new_cursor++;
        }
    }


    free(chunk->pixels);
    chunk->pixels = new_pixels;
    return chunk;
}

Pixel *reassemble_partitions(ImageChunk *chunks, uint num_partitions) {
    Pixel *pixels = malloc(sizeof(Pixel) * chunks->image_width * chunks->image_height);

    uint row_jump = chunks->image_width; // * sizeof(Pixel); //PITFALL!!
    uint col_jump = 1; // sizeof(Pixel);

    ImageChunk *chunk_cursor = chunks;
    for (int i = 0; i < num_partitions; i++) {

        uint x_part = chunk_cursor->x_part;
        uint y_part = chunk_cursor->y_part;

        uint chunk_height = chunk_cursor->chunk_height;
        uint chunk_width = chunk_cursor->chunk_width;


        Pixel *old_cursor = chunk_cursor->pixels;
        for (int j = 0; j < chunk_height; j++) {
            for (int k = 0; k < chunk_width; k++) {
                uint x = (k + (chunk_width * x_part));
                uint y = (j + (chunk_height * y_part));
//                x += (x_part == 0) ? 0 : -1;
//                y += (y_part == 0) ? 0 : -1;


                *(pixels + (row_jump * y) + (col_jump * x)) = *old_cursor;
                old_cursor++;
            }
        }
        free(chunk_cursor->pixels);
        chunk_cursor++;
    }
    free(chunks);
    return pixels;
}

void blur(Pixel *neighbors[9], Pixel *new_pixel) {
    uint b = 0, g = 0, r = 0;

    uint count = 0;
    for (int i = 0; i < 9; i++) {
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


