#include "../headers/BMPHandler.h"

/**
 * Read BMP header of a BMP file.
 *
 * @param  file: A pointer to the file being read
 * @param  header: Pointer to the destination BMP header
 */
void readBMPHeader(FILE *file, BMPHeader *header) {
  fread(&header->signature, sizeof(uint8_t) * 2, 1, file);
  fread(&header->file_size, sizeof(uint32_t), 1, file);
  fread(&header->reserved1, sizeof(uint16_t), 1, file);
  fread(&header->reserved2, sizeof(uint16_t), 1, file);
  fread(&header->offset_pixel_array, sizeof(uint32_t), 1, file);
}

/**
 * Write BMP header of a file. Useful for creating a BMP file.
 *
 * @param  file: A pointer to the file being written
 * @param  header: The header to write to the file
 */
void writeBMPHeader(FILE *file, const BMPHeader *header) {
  fwrite(&header->signature, sizeof(uint8_t) * 2, 1, file);
  fwrite(&header->file_size, sizeof(uint32_t), 1, file);
  fwrite(&header->reserved1, sizeof(uint16_t), 1, file);
  fwrite(&header->reserved2, sizeof(uint16_t), 1, file);
  fwrite(&header->offset_pixel_array, sizeof(uint32_t), 1, file);
}

/**
 * Read DIB header from a BMP file.
 *
 * @param  file: A pointer to the file being read
 * @param  header: Pointer to the destination DIB header
 */
void readDIBHeader(FILE *file, DIBHeader *header) {
  fread(&header->dib_header_size, sizeof(uint32_t), 1, file);
  fread(&header->image_width_w, sizeof(int32_t), 1, file);
  fread(&header->image_height_h, sizeof(int32_t), 1, file);
  fread(&header->planes, sizeof(uint16_t), 1, file);
  fread(&header->bits_per_pixel, sizeof(uint16_t), 1, file);
  fread(&header->compression, sizeof(int32_t), 1, file);
  fread(&header->image_size, sizeof(int32_t), 1, file);
  fread(&header->x_pixels_per_meter, sizeof(int32_t), 1, file);
  fread(&header->y_pixels_per_meter, sizeof(int32_t), 1, file);
  fread(&header->color_table_colors, sizeof(uint32_t), 1, file);
  fread(&header->important_color_count, sizeof(uint32_t), 1, file);
}

/**
 * Write DIB header of a file. Useful for creating a BMP file.
 *
 * @param  file: A pointer to the file being written
 * @param  header: The header to write to the file
 */
void writeDIBHeader(FILE *file, const DIBHeader *header) {
  fwrite(&header->dib_header_size, sizeof(uint32_t), 1, file);
  fwrite(&header->image_width_w, sizeof(int32_t), 1, file);
  fwrite(&header->image_height_h, sizeof(int32_t), 1, file);
  fwrite(&header->planes, sizeof(uint16_t), 1, file);
  fwrite(&header->bits_per_pixel, sizeof(uint16_t), 1, file);
  fwrite(&header->compression, sizeof(int32_t), 1, file);
  fwrite(&header->image_size, sizeof(int32_t), 1, file);
  fwrite(&header->x_pixels_per_meter, sizeof(int32_t), 1, file);
  fwrite(&header->y_pixels_per_meter, sizeof(int32_t), 1, file);
  fwrite(&header->color_table_colors, sizeof(uint32_t), 1, file);
  fwrite(&header->important_color_count, sizeof(uint32_t), 1, file);
}

/**
 * Make BMP header based on width and height. Useful for creating a BMP file.
 *
 * @param  header: Pointer to the destination DIB header
 * @param  width: Width of the image that this header is for
 * @param  height: Height of the image that this header is for
 */
void makeBMPHeader(BMPHeader *header, uint32_t width, uint32_t height) {
  header->signature[0] = 'B';
  header->signature[1] = 'M';
  header->reserved1 = 0;
  header->reserved2 = 0;
  header->offset_pixel_array = 54;
  uint32_t image_size = width * height;
  header->file_size = header->offset_pixel_array + image_size;
  // TODO: add padding to this equation ^^
}

/**
 * Make new DIB header based on width and height.Useful for creating a BMP file.
 *
 * @param  header: Pointer to the destination DIB header
 * @param  width: Width of the image that this header is for
 * @param  height: Height of the image that this header is for
 */
void makeDIBHeader(DIBHeader *header, int32_t width, int32_t height) {
  header->dib_header_size = sizeof(DIBHeader);
  header->image_width_w = width;
  header->image_height_h = height;
  header->planes = 1;
  header->bits_per_pixel = 24;
  // TODO: calculate compression
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
void readPixels(FILE *file, Pixel **pArr, size_t width, size_t height) {
  // navigate to the start of the pixel array
  const long PIXELS_START = 54;
  fseek(file, PIXELS_START, SEEK_SET);
  const size_t padding = width % 4;
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      fread(&pArr[i][j].b, sizeof(rgb_value), 1, file);
      fread(&pArr[i][j].g, sizeof(rgb_value), 1, file);
      fread(&pArr[i][j].r, sizeof(rgb_value), 1, file);
    }
    // skip the padding goes here
    fseek(file, (long) (sizeof(rgb_value) * padding), SEEK_CUR);
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
void writePixels(FILE *file, const Pixel * const *pArr, size_t width, size_t height) {
  const long PIXELS_START = 54;
  // navigate to the start of the pixel array
  fseek(file, PIXELS_START, SEEK_SET);

  const size_t padding = width % 4;
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      fwrite(&pArr[i][j].b, sizeof(rgb_value), 1, file);
      fwrite(&pArr[i][j].g, sizeof(rgb_value), 1, file);
      fwrite(&pArr[i][j].r, sizeof(rgb_value), 1, file);
    }
    // skip the padding
    fseek(file, (long) (sizeof(rgb_value) * padding), SEEK_CUR);
  }
}
