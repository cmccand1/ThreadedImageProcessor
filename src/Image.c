//
// Created by Clint McCandless on 10/26/23.
#include "../headers/Image.h"

#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

// helper functions
static int generate_radius(int min_dimension);
static int pixel_in_bounds(const Image *img, int row, int col);
static unsigned char clamp_to_uchar(int value);
static void flood_fill(const Image *img, int h, int k, int radius, int x,
                       int y);


struct Pixel **create_pixel_array_2d(int width, int height) {
  struct Pixel **pixels;
  if ((pixels = malloc(sizeof(struct Pixel *) * height)) == NULL) {
    perror("Error while allocating memory for 2d pixel array.\n");
    exit(EXIT_FAILURE);
  };
  for (int row = 0; row < height; ++row) {
    if ((pixels[row] = malloc(sizeof(struct Pixel) * width)) == NULL) {
      perror("Error while allocating memory for 2d pixel array.\n");
      exit(EXIT_FAILURE);
    };
  }
  return pixels;
}

void free_pixel_array_2d(struct Pixel **array, int width, int height) {
  for (int i = 0; i < height; ++i) {
    free(array[i]);
    array[i] = NULL;
  }
  free(array);
  array = NULL;
}

/** Creates a new image and returns it.
 *
 * @param  pArr: Pixel array of this image.
 * @param  width: Width of this image.
 * @param  height: Height of this image.
 * @return A pointer to a new image.
 */
Image *image_create(struct Pixel **pArr, int width, int height) {
  // allocate memory for new Image
  Image *img;

  if ((img = malloc(sizeof(Image))) == NULL) {
    perror("Failed to allocate memory in image_create().\n");
    exit(EXIT_FAILURE);
  }

  // initialize new Image
  img->height = height;
  img->width = width;
  img->pixel_array = pArr;
  // return pointer to new Image
  return img;
}

/** Destroys an image. Does not deallocate internal pixel array.
 *
 * @param  img: the image to destroy.
 */
void image_destroy(Image **img) {
  // TODO: comment out pixel array deletion for submission
  struct Pixel **pixels = (*img)->pixel_array;
  for (int i = 0; i < image_get_height((*img)); ++i) {
    free(pixels[i]);
    pixels[i] = NULL;
  }
  free(pixels);
  pixels = NULL;

  free(*img);
  *img = NULL;
}

/** Returns a double pointer to the pixel array.
 * @param  img: the image.
 */
struct Pixel **image_get_pixels(const Image *img) { return img->pixel_array; }

/** Returns the width of the image.
 *
 * @param  img: the image.
 */
int image_get_width(const Image *img) { return img->width; }

/** Returns the height of the image.
 *
 * @param  img: the image.
 */
int image_get_height(const Image *img) { return img->height; }

/** Converts the image to grayscale.
 *
 * @param  img: the image.
 */
void image_apply_bw(const Image *img) {
  // get a pointer to the first pixel in the pixel array
  struct Pixel **pixels = img->pixel_array;
  // for each pixel, change each RGB component to the same value
  for (int i = 0; i < img->width; ++i) {
    for (int j = 0; j < img->height; ++j) {
      // calculate the grayscale value
      const unsigned char GRAYSCALE_VALUE = (0.299 * pixels[i][j].r) +
                                            (0.587 * pixels[i][j].g) +
                                            (0.114 * pixels[i][j].b);
      // set each RGB component to the calculated grayscale value
      pixels[i][j].r = GRAYSCALE_VALUE;
      pixels[i][j].g = GRAYSCALE_VALUE;
      pixels[i][j].b = GRAYSCALE_VALUE;
    }
  }
}

/**
 * Shift color of the internal Pixel array. The dimension of the array is width
 * * height. The shift value of RGB is rShift, gShift，bShift. Useful for color
 * shift.
 *
 * @param  img: the image.
 * @param  rShift: the shift value of color r shift
 * @param  gShift: the shift value of color g shift
 * @param  bShift: the shift value of color b shift
 */
void image_apply_colorshift(const Image *img, int rShift, int gShift,
                            int bShift) {
  // get a pointer to the first pixel in the pixel array
  struct Pixel **pixels = img->pixel_array;
  for (int i = 0; i < img->width; ++i) {
    for (int j = 0; j < img->height; ++j) {
      pixels[i][j].r = clamp_to_uchar(pixels[i][j].r + rShift);
      pixels[i][j].g = clamp_to_uchar(pixels[i][j].g + gShift);
      pixels[i][j].b = clamp_to_uchar(pixels[i][j].b + bShift);
    }
  }
}

/**
 * Applies the swiss cheese filter to the image.
 * @param img the image to filter
 */
void image_apply_cheese(const Image *img) {
  // tint the image yellow ;P
  image_apply_bw(img);
  image_apply_colorshift(img, 150, 150, 0);
  const int min_dimension = min(img->width, img->height);
  srand(time(NULL));
  for (int i = 0; i < NUM_HOLES(min_dimension); ++i) {
    // pick random center points (h,k)
    const int h = rand() % min_dimension;
    const int k = rand() % min_dimension;
    // for each center point, generate a random radius
    const int radius = generate_radius(min_dimension);
    // flood fill from center to fill the circle
    flood_fill(img, h, k, radius, h, k);
  }
  printf("swiss cheese applied\n");
}


void *image_apply_t_boxblur(void *data) {
  const struct thread_info *thread_info = (struct thread_info *)data;
  int valid_neighbors;   // the number of valid og_pixels
  int rSum, gSum, bSum;  // sum total of each RGB component

  // get a pointer to the first pixel in the original image
  struct Pixel **og_pixels = thread_info->og_image->pixel_array;

  // apply box blur to each pixel in the original array (that this thread can
  // see)
  for (int row = 0; row < thread_info->height; ++row) {
    for (int col = thread_info->start + 1; col < thread_info->end;
         ++col) {  // we're ignoring the first and last columns
      // keep track of RGB component sums and valid pixel count
      rSum = gSum = bSum = valid_neighbors = 0;
      // search the 3x3 neighborhood (offsets: -1, 0, 1)
      for (int i = -1; i < NEIGHBORHOOD_SIZE - 1; ++i) {
        for (int j = -1; j < NEIGHBORHOOD_SIZE - 1; ++j) {
          // when we find a valid neighbor, add its RGB values to RGB sums, and
          // increment valid neighbors count
          if (pixel_in_bounds(thread_info->og_image, row + i, col + j) == 0) {
            ++valid_neighbors;
            // add the sum of the pixel at the offset
            rSum += thread_info->thread_pixel_array[row + i][col + j].r;
            gSum += thread_info->thread_pixel_array[row + i][col + j].g;
            bSum += thread_info->thread_pixel_array[row + i][col + j].b;
          }
        }
      }
      // update the og_pixels in the original pixel array
      og_pixels[row][col].r = clamp_to_uchar(rSum /= valid_neighbors);
      og_pixels[row][col].g = clamp_to_uchar(gSum /= valid_neighbors);
      og_pixels[row][col].b = clamp_to_uchar(bSum /= valid_neighbors);
    }
  }
  pthread_exit(NULL);
}

/**
 * Converts the image to grayscale. If the scaling factor is less than 1 the new
 * image will be smaller, if it is larger than 1, the new image will be larger.
 *
 * @param  img: the image.
 * @param  factor: the scaling factor
 */
void image_apply_resize(Image *img, float factor) {
  // TODO: implement this
}

/**
 * Generates a random radius that is some fraction of the smallest image
 * dimension
 * @param min_dimension
 * @return
 */
static int generate_radius(int min_dimension) {
  // Constraint: radius less than 1/6 the min dimension, at least 5
  return (rand() % (min_dimension / 6)) + 5;
}

/**
 * Checks whether the pixel to look at is in the bounds of the image.
 * Used to prevent accessing memory that doesn't belong to the image array.
 * @param img: the image we're checking against
 * @param row: the row index of the pixel to check
 * @param col: the column index of the pixel to check
 * @return: 0 if the pixel is a valid pixel, 1 otherwise
 */
static int pixel_in_bounds(const Image *img, int row, int col) {
  if (row < 0 || row >= img->height || col < 0 || col >= img->width)
    return 1;  // indices are out of bounds
  return 0;    // indices are within bounds
}

/**
 * Clamps the integer value to fit in an unsigned char.
 * @param value the integer value to clamp
 * @return the clamped value, an unsigned char
 */
static unsigned char clamp_to_uchar(int value) {
  const unsigned char MIN_RGB = 0;
  const unsigned char MAX_RGB = UCHAR_MAX;

  if (value < MIN_RGB) {
    return MIN_RGB;
  } else if (value > MAX_RGB) {
    return MAX_RGB;
  } else {
    return value;
  }
}

static void flood_fill(const Image *const img, int h, int k, int radius, int x,
                       int y) {
  // if the pixel is out of bounds, return
  if (pixel_in_bounds(img, x, y) == 0) return;
  // get the distance between pixels
  const int distance_from_center = sqrt((pow(x - h, 2)) + (pow(y - k, 2)));
  if (distance_from_center > radius) return;
  // if the pixel is already black, return to avoid infinite loop
  if (img->pixel_array[x][y].r == 0 && img->pixel_array[x][y].g == 0 &&
      img->pixel_array[x][y].b == 0)
    return;
  // if the pixel is within the radius, paint it black
  img->pixel_array[x][y].r = 0;
  img->pixel_array[x][y].g = 0;
  img->pixel_array[x][y].b = 0;
  // do the same to its 4 neighbors
  flood_fill(img, h, k, radius, x - 1, y);
  flood_fill(img, h, k, radius, x + 1, y);
  flood_fill(img, h, k, radius, x, y - 1);
  flood_fill(img, h, k, radius, x, y + 1);
  return;
}