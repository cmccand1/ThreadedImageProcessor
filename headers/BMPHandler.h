#include <stdio.h>
#include "Image.h"

typedef struct {
  uint8_t signature[2];
  uint32_t file_size;
  uint16_t reserved1, reserved2;
  uint32_t offset_pixel_array;
} BMPHeader;

typedef struct {
  uint32_t dib_header_size;
  int32_t image_width_w, image_height_h;
  uint16_t planes, bits_per_pixel;
  int32_t compression, image_size, x_pixels_per_meter, y_pixels_per_meter;
  uint32_t color_table_colors, important_color_count;
} DIBHeader;

/**
 * Read BMP header of a BMP file.
 *
 * @param  file: A pointer to the file being read
 * @param  header: Pointer to the destination BMP header
 */
void readBMPHeader(FILE *file, BMPHeader *header);

/**
 * Write BMP header of a file. Useful for creating a BMP file.
 *
 * @param  file: A pointer to the file being written
 * @param  header: The header to write to the file
 */
void writeBMPHeader(FILE *file, const BMPHeader *header);

/**
 * Read DIB header from a BMP file.
 *
 * @param  file: A pointer to the file being read
 * @param  header: Pointer to the destination DIB header
 */
void readDIBHeader(FILE *file, DIBHeader *header);

/**
 * Write DIB header of a file. Useful for creating a BMP file.
 *
 * @param  file: A pointer to the file being written
 * @param  header: The header to write to the file
 */
void writeDIBHeader(FILE *file, const DIBHeader *header);

/**
 * Make BMP header based on width and height. Useful for creating a BMP file.
 *
 * @param  header: Pointer to the destination DIB header
 * @param  width: Width of the image that this header is for
 * @param  height: Height of the image that this header is for
 */
void makeBMPHeader(BMPHeader *header, uint32_t width, uint32_t height);

/**
* Make new DIB header based on width and height.Useful for creating a BMP file.
*
* @param  header: Pointer to the destination DIB header
* @param  width: Width of the image that this header is for
* @param  height: Height of the image that this header is for
*/
void makeDIBHeader(DIBHeader *header, int32_t width, int32_t height);

/**
 * Read Pixels from BMP file based on width and height.
 *
 * @param  file: A pointer to the file being read
 * @param  pArr: Pixel array to store the pixels being read
 * @param  width: Width of the pixel array of this image
 * @param  height: Height of the pixel array of this image
 */
void readPixels(FILE *file, Pixel **pArr, size_t width, size_t height);

/**
 * Write Pixels from BMP file based on width and height.
 *
 * @param  file: A pointer to the file being read or written
 * @param  pArr: Pixel array of the image to write to the file
 * @param  width: Width of the pixel array of this image
 * @param  height: Height of the pixel array of this image
 */
void writePixels(FILE *file, const Pixel * const *pArr, size_t width, size_t height);
