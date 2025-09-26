#ifndef PixelProcessor_H
#define PixelProcessor_H

#include <stdio.h>
#include <stdlib.h>

#define BMP_HEADER_SIZE 14
#define BMP_DIB_HEADER_SIZE 40
#define MAXIMUM_IMAGE_SIZE 4096
#define KERNEL_SIZE 5  // NxN kernel size for box blur. Must be odd to be square
#define THREAD_COUNT 12

typedef unsigned char rgb_value;

typedef struct {
  rgb_value r; // 8-bit rvalue
  rgb_value g; // 8-bit gvalue
  rgb_value b; // 8-bit bvalue
} Pixel;

typedef struct {
  Pixel **pixel_array;
  int32_t width;
  int32_t height;
} Image;

typedef struct {
  Pixel **thread_pixel_array; // the smaller pixel array that this thread owns
  size_t width, height;
  const Image *og_image;
  size_t start, end;
  // the index of where this threads window onto the og_image starts/ends
  int rShift, gShift, bShift;
} ThreadData;


Pixel **create_pixel_array_2d(size_t width, size_t height);

void free_pixel_array_2d(Pixel **array, size_t height);

/** Creates a new image and returns it.
 *
 * @param  pArr: Pixel array of this image.
 * @param  width: Width of this image.
 * @param  height: Height of this image.
 * @return A pointer to a new image.
 */
Image *image_create(Pixel **pArr, int32_t width, int32_t height);

/** Destroys an image and deallocates its memory. This includes the pixel array
 *
 * @param  img: the image to destroy.
 */
void image_destroy(Image **img);

/** Returns a double pointer to the pixel array.
 *
 * @param  img: the image.
 */
Pixel **image_get_pixels(const Image *img);

/** Returns the width of the image.
 *
 * @param  img: the image.
 */
int32_t image_get_width(const Image *img);

/** Returns the height of the image.
 *
 * @param  img: the image.
 */
int32_t image_get_height(const Image *img);

/**
 * Apply a cheese filter to the image. The cheese filter will apply a yellow
 * tint to the image and add random holes to the image.
 * @param img the image to filter
 */
void image_apply_cheese(Image *img);

void *image_apply_t_bw(void *data);

void *image_apply_t_cheese(void *data);

void *image_apply_t_boxblur(void *data);

void *image_apply_t_colorshift(void *data);

/**
 * Converts the image to grayscale. If the scaling factor is less than 1 the new
 * image will be smaller, if it is larger than 1, the new image will be larger.
 *
 * @param  img: the image.
 * @param  factor: the scaling factor
 */
void image_apply_resize(Image *img, float factor);

#endif
