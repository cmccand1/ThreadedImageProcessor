//
// Created by Clint McCandless on 10/25/23.
//

#include "BMPHandler.h"

/**
 * Read BMP header of a BMP file.
 *
 * @param  file: A pointer to the file being read
 * @param  header: Pointer to the destination BMP header
 */
void readBMPHeader(FILE* file, struct BMP_Header* header) {
    fread(&header->signature, sizeof(char)*2, 1, file);
    fread(&header->file_size, sizeof(int), 1, file);
    fread(&header->reserved1, sizeof(short), 1, file);
    fread(&header->reserved2, sizeof(short), 1, file);
    fread(&header->offset_pixel_array, sizeof(int), 1, file);

//    printf("BMP HEADER\n");
//    printf("signature: %c%c\n", header->signature[0], header->signature[1]);
//    printf("file_size: %d\n", header->file_size);
//    printf("reserved1: %d\n", header->reserved1);
//    printf("reserved2: %d\n", header->reserved2);
//    printf("offset_pixel_array: %d\n", header->offset_pixel_array);
}

/**
 * Write BMP header of a file. Useful for creating a BMP file.
 *
 * @param  file: A pointer to the file being written
 * @param  header: The header to write to the file
 */
void writeBMPHeader(FILE* file, struct BMP_Header* header) {
    fwrite(&header->signature, sizeof(char)*2, 1, file);
    fwrite(&header->file_size, sizeof(int), 1, file);
    fwrite(&header->reserved1, sizeof(short), 1, file);
    fwrite(&header->reserved2, sizeof(short), 1, file);
    fwrite(&header->offset_pixel_array, sizeof(int), 1, file);
}

/**
 * Read DIB header from a BMP file.
 *
 * @param  file: A pointer to the file being read
 * @param  header: Pointer to the destination DIB header
 */
void readDIBHeader(FILE* file, struct DIB_Header* header) {
    fread(&header->dib_header_size, sizeof(int), 1, file);
    fread(&header->image_width_w, sizeof(int), 1, file);
    fread(&header->image_height_h, sizeof(int), 1, file);
    fread(&header->planes, sizeof(short), 1, file);
    fread(&header->bits_per_pixel, sizeof(short), 1, file);
    fread(&header->compression, sizeof(int), 1, file);
    fread(&header->image_size, sizeof(int), 1, file);
    fread(&header->x_pixels_per_meter, sizeof(int), 1, file);
    fread(&header->y_pixels_per_meter, sizeof(int), 1, file);
    fread(&header->color_table_colors, sizeof(int), 1, file);
    fread(&header->important_color_count, sizeof(int), 1, file);

//    printf("\nDIB HEADER\n");
//    printf("dib header file_size: %d\n", header->dib_header_size);
//    printf("image width: %d\n", header->image_width_w);
//    printf("image height: %d\n", header->image_height_h);
//    printf("planes: %hu\n", header->planes);
//    printf("bits per pixel: %hu\n", header->bits_per_pixel);
//    printf("compression: %d\n", header->compression);
//    printf("image file_size: %d\n", header->image_size);
//    printf("x pixels per meter: %d\n", header->x_pixels_per_meter);
//    printf("y pixels per meter: %d\n", header->y_pixels_per_meter);
//    printf("color table colors: %d\n", header->color_table_colors);
//    printf("important color count: %d\n", header->important_color_count);
}

/**
 * Write DIB header of a file. Useful for creating a BMP file.
 *
 * @param  file: A pointer to the file being written
 * @param  header: The header to write to the file
 */
void writeDIBHeader(FILE* file, struct DIB_Header* header) {
    fwrite(&header->dib_header_size, sizeof(int), 1, file);
    fwrite(&header->image_width_w, sizeof(int), 1, file);
    fwrite(&header->image_height_h, sizeof(int), 1, file);
    fwrite(&header->planes, sizeof(short), 1, file);
    fwrite(&header->bits_per_pixel, sizeof(short), 1, file);
    fwrite(&header->compression, sizeof(int), 1, file);
    fwrite(&header->image_size, sizeof(int), 1, file);
    fwrite(&header->x_pixels_per_meter, sizeof(int), 1, file);
    fwrite(&header->y_pixels_per_meter, sizeof(int), 1, file);
    fwrite(&header->color_table_colors, sizeof(int), 1, file);
    fwrite(&header->important_color_count, sizeof(int), 1, file);
}

/**
 * Make BMP header based on width and height. Useful for creating a BMP file.
 *
 * @param  header: Pointer to the destination DIB header
 * @param  width: Width of the image that this header is for
 * @param  height: Height of the image that this header is for
 */
void makeBMPHeader(struct BMP_Header* header, int width, int height) {
    header->signature[0] = 'B';
    header->signature[1] = 'M';
    header->reserved1 = 0;
    header->reserved2 = 0;
    header->offset_pixel_array = 54;
    int image_size = width*height;
    header->file_size = header->offset_pixel_array + image_size;
    //TODO: add padding to this equation ^^

}

/**
* Make new DIB header based on width and height.Useful for creating a BMP file.
*
* @param  header: Pointer to the destination DIB header
* @param  width: Width of the image that this header is for
* @param  height: Height of the image that this header is for
*/
void makeDIBHeader(struct DIB_Header* header, int width, int height) {
    header->dib_header_size = sizeof(struct DIB_Header);
    header->image_width_w = width;
    header->image_height_h = height;
    header->planes = 1;
    header->bits_per_pixel = sizeof(unsigned char)*3;
    //TODO: calculate compression
    header->image_size = width * height;
    header->x_pixels_per_meter = 3780;
    header->x_pixels_per_meter = 3780;
    header->color_table_colors = 0;
    header->important_color_count = 0;

}

/**
 * Read Pixels from BMP file based on width and height.
 *
 * @param  file: A pointer to the file being read
 * @param  pArr: Pixel array to store the pixels being read
 * @param  width: Width of the pixel array of this image
 * @param  height: Height of the pixel array of this image
 */
void readPixelsBMP(FILE* file, struct Pixel** pArr, int width, int height) {
    // navigate to the start of the pixel array
    fseek(file, 54, SEEK_SET);
    int padding = width % 4;
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            fread(&pArr[i][j].b, sizeof(unsigned char), 1, file);
            fread(&pArr[i][j].g, sizeof(unsigned char), 1, file);
            fread(&pArr[i][j].r, sizeof(unsigned char), 1, file);
        }
        // skip the padding goes here
        fseek(file, sizeof(unsigned char) * padding, SEEK_CUR);
    }
}
/**
 * Write Pixels from BMP file based on width and height.
 *
 * @param  file: A pointer to the file being read or written
 * @param  pArr: Pixel array of the image to write to the file
 * @param  width: Width of the pixel array of this image
 * @param  height: Height of the pixel array of this image
 */
void writePixelsBMP(FILE* file, struct Pixel** pArr, int width, int height) {
    // navigate to the start of the pixel array
    fseek(file, 54, SEEK_SET);

    int padding = width % 4;
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            fwrite(&pArr[i][j].b, sizeof(unsigned char), 1, file);
            fwrite(&pArr[i][j].g, sizeof(unsigned char), 1, file);
            fwrite(&pArr[i][j].r, sizeof(unsigned char), 1, file);
        }
        // skip the padding
        fseek(file, sizeof(unsigned char) * padding, SEEK_CUR);
    }
}


