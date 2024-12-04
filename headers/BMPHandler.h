/**
* Implementation of several functions to manipulate BMP files.
*
* Completion time: ?
*
* @author Clint McCandless, Ruben Acuna
* @version 1.0
*/

#include <stdio.h>
#include "Image.h"

struct BMP_Header {
  char signature[2];
  int file_size;
  short reserved1, reserved2;
  int offset_pixel_array;
};

struct DIB_Header {
  int dib_header_size;
  int image_width_w, image_height_h;
  short planes, bits_per_pixel;
  int compression, image_size, x_pixels_per_meter, y_pixels_per_meter;
  int color_table_colors, important_color_count;
};

/**
 * Read BMP header of a BMP file.
 *
 * @param  file: A pointer to the file being read
 * @param  header: Pointer to the destination BMP header
 */
void readBMPHeader(FILE *file, struct BMP_Header *header);

/**
 * Write BMP header of a file. Useful for creating a BMP file.
 *
 * @param  file: A pointer to the file being written
 * @param  header: The header to write to the file
 */
void writeBMPHeader(FILE *file, const struct BMP_Header *header);

/**
 * Read DIB header from a BMP file.
 *
 * @param  file: A pointer to the file being read
 * @param  header: Pointer to the destination DIB header
 */
void readDIBHeader(FILE *file, struct DIB_Header *header);

/**
 * Write DIB header of a file. Useful for creating a BMP file.
 *
 * @param  file: A pointer to the file being written
 * @param  header: The header to write to the file
 */
void writeDIBHeader(FILE *file, const struct DIB_Header *header);

/**
 * Make BMP header based on width and height. Useful for creating a BMP file.
 *
 * @param  header: Pointer to the destination DIB header
 * @param  width: Width of the image that this header is for
 * @param  height: Height of the image that this header is for
 */
void makeBMPHeader(struct BMP_Header *header, int width, int height);

/**
* Make new DIB header based on width and height.Useful for creating a BMP file.
*
* @param  header: Pointer to the destination DIB header
* @param  width: Width of the image that this header is for
* @param  height: Height of the image that this header is for
*/
void makeDIBHeader(struct DIB_Header *header, int width, int height);

/**
 * Read Pixels from BMP file based on width and height.
 *
 * @param  file: A pointer to the file being read
 * @param  pArr: Pixel array to store the pixels being read
 * @param  width: Width of the pixel array of this image
 * @param  height: Height of the pixel array of this image
 */
void readPixelsBMP(FILE *file, struct Pixel **pArr, int width, int height);

/**
 * Write Pixels from BMP file based on width and height.
 *
 * @param  file: A pointer to the file being read or written
 * @param  pArr: Pixel array of the image to write to the file
 * @param  width: Width of the pixel array of this image
 * @param  height: Height of the pixel array of this image
 */
void writePixelsBMP(FILE *file, struct Pixel **pArr, int width, int height);