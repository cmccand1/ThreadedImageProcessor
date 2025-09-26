#include "../headers/Image.h"

#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <sys/errno.h>

#include "../headers/macros.h"

// helper functions
static int generate_radius(int min_dimension);

static bool pixel_in_bounds(const Image *img, int row, int col);

static unsigned char clamp_to_pixel(int value);

static void flood_fill(Image *img,
                       int h,
                       int k,
                       int radius,
                       int x,
                       int y);

Pixel **create_pixel_array_2d(size_t width, size_t height) {
  Pixel **pixels = nullptr;
  MALLOC_2D(pixels, height, width, Pixel, fail);
  // initialize to 0 pixels
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      pixels[i][j].r = 0;
      pixels[i][j].g = 0;
      pixels[i][j].b = 0;
    }
  }
  return pixels;
fail:
  return nullptr;
}

void free_pixel_array_2d(Pixel **array, size_t height) {
  for (size_t i = 0; i < height; ++i) {
    FREE(array[i]);
  }
  FREE(array);
}

/** Creates a new image and returns it.
*
* @param  pArr: Pixel array of this image.
* @param  width: Width of this image.
* @param  height: Height of this image.
* @return A pointer to a new image.
*/
Image *image_create(Pixel **pArr, int32_t width, int32_t height) {
  Image *img = nullptr;

  if ((img = malloc(sizeof(Image))) == nullptr) {
    perror("Failed to allocate memory in image_create().\n");
    return nullptr;
  }

  // initialize new Image
  img->height = height;
  img->width = width;
  img->pixel_array = pArr;

  // return pointer to new Image
  return img;
}

/**
 * Destroy an image and deallocate its memory. This includes the pixel array.
 *
 * @param  img: the image to destroy.
 */
void image_destroy(Image **img) {
  // free the pixel array
  free_pixel_array_2d((*img)->pixel_array, (size_t) (*img)->height);
  // free the image
  FREE(*img);
}

/** Returns a double pointer to the pixel array.
 * @param  img: the image.
 */
Pixel **image_get_pixels(const Image *img) { return img->pixel_array; }

/** Returns the width of the image.
 *
 * @param  img: the image.
 */
int32_t image_get_width(const Image *img) { return img->width; }

/** Returns the height of the image.
 *
 * @param  img: the image.
 */
int32_t image_get_height(const Image *img) { return img->height; }

/**
 * Applies the swiss cheese filter to the image.
 * @param img the image to filter
 */
void image_apply_cheese(Image *img) {
  // tint the image yellow ;P
  // image_apply_bw(img);
  // image_apply_colorshift(img, 150, 150, 0);
  const int32_t MIN_DIM = (img->width < img->height) ? img->width : img->height;
  const int32_t HOLE_COUNT = (int32_t) (MIN_DIM * 0.08);

  srand((unsigned) time(nullptr));
  for (int32_t i = 0; i < HOLE_COUNT; ++i) {
    // pick random center points (h,k)
    const int32_t h = rand() % MIN_DIM;
    const int32_t k = rand() % MIN_DIM;
    // for each center point, generate a random radius
    const int32_t radius = generate_radius(MIN_DIM);
    // flood fill from center to fill the circle
    flood_fill(img, h, k, radius, h, k);
  }
  printf("swiss cheese applied\n");
}

void *image_apply_t_colorshift(void *data) {
  const ThreadData *thread_data = (ThreadData *) data;

  Pixel **read_pixels = thread_data->og_image->pixel_array;
  Pixel **write_pixels = thread_data->thread_pixel_array;

  for (size_t i = 0; i < thread_data->height; ++i) {
    for (size_t j = thread_data->start; j <= thread_data->end; ++j) {
      write_pixels[i][j - thread_data->start].r =
          clamp_to_pixel(read_pixels[i][j].r + thread_data->rShift);
      write_pixels[i][j - thread_data->start].g =
          clamp_to_pixel(read_pixels[i][j].g + thread_data->gShift);
      write_pixels[i][j - thread_data->start].b =
          clamp_to_pixel(read_pixels[i][j].b + thread_data->bShift);
    }
  }
  pthread_exit(nullptr);
}

void *image_apply_t_bw(void *data) {
  const ThreadData *thread_data = (ThreadData *) data;

  Pixel **read_pixels = thread_data->og_image->pixel_array;
  Pixel **write_pixels = thread_data->thread_pixel_array;

  for (size_t i = 0; i < (size_t) thread_data->height; ++i) {
    for (size_t j = thread_data->start; j <= thread_data->end; ++j) {
      const rgb_value GRAYSCALE_VALUE = (rgb_value) (
        (0.299 * read_pixels[i][j].r) +
        (0.587 * read_pixels[i][j].g) +
        (0.114 * read_pixels[i][j].b));

      // set each RGB component to the calculated grayscale value
      write_pixels[i][j - thread_data->start].r = GRAYSCALE_VALUE;
      write_pixels[i][j - thread_data->start].g = GRAYSCALE_VALUE;
      write_pixels[i][j - thread_data->start].b = GRAYSCALE_VALUE;
    }
  }
  pthread_exit(nullptr);
}

void *image_apply_t_cheese(void *data) {
  // TODO: implement this
  (void) data;
  pthread_exit(nullptr);
}

void *image_apply_t_boxblur(void *data) {
  const ThreadData *thread_data = (ThreadData *) data;

  // Validate input parameters
  if (!thread_data || !thread_data->og_image ||
      !thread_data->og_image->pixel_array || !thread_data->thread_pixel_array) {
    fprintf(stderr, "Invalid thread data or image pointers\n");
    pthread_exit(nullptr);
  }

  Pixel **og_pixels = thread_data->og_image->pixel_array;
  Pixel **new_pixels = thread_data->thread_pixel_array;

  // Process each pixel in thread's section
  for (size_t row = 0; row < thread_data->height; ++row) {
    for (size_t col = thread_data->start; col <= thread_data->end; ++col) {
      int rSum = 0, gSum = 0, bSum = 0;
      int valid_neighbors = 0;

      // Process the neighborhood of the current pixel
      int half_kernel = KERNEL_SIZE / 2;
      for (int i = -half_kernel; i <= half_kernel; ++i) {
        for (int j = -half_kernel; j <= half_kernel; ++j) {
          int row_offset = (int) row + i;
          int col_offset = (int) col + j;

          if (pixel_in_bounds(thread_data->og_image, row_offset, col_offset)) {
            ++valid_neighbors;
            rSum += og_pixels[row_offset][col_offset].r;
            gSum += og_pixels[row_offset][col_offset].g;
            bSum += og_pixels[row_offset][col_offset].b;
          }
        }
      }

      // Avoid division by zero
      if (valid_neighbors == 0) {
        fprintf(stderr,
                "Warning: No valid neighbors at position [%zu,%zu]\n",
                row,
                col);
        // Copy original pixel value as fallback
        new_pixels[row][col - thread_data->start].r = og_pixels[row][col].r;
        new_pixels[row][col - thread_data->start].g = og_pixels[row][col].g;
        new_pixels[row][col - thread_data->start].b = og_pixels[row][col].b;
        continue;
      }

      // Store averaged values in thread's local array
      new_pixels[row][col - thread_data->start].r =
          clamp_to_pixel(rSum / valid_neighbors);
      new_pixels[row][col - thread_data->start].g =
          clamp_to_pixel(gSum / valid_neighbors);
      new_pixels[row][col - thread_data->start].b =
          clamp_to_pixel(bSum / valid_neighbors);
    }
  }

  pthread_exit(nullptr);
}

/**
 * Converts the image to grayscale. If the scaling factor is less than 1 the new
 * image will be smaller, if it is larger than 1, the new image will be larger.
 *
 * @param  img: the image.
 * @param  factor: the scaling factor
 */
// void image_apply_resize(Image *img, float factor) {
//   // TODO: implement this
// }

/**
 * Generates a random radius that is some fraction of the smallest image
 * dimension
 * @param min_dimension
 * @return
 */
static int generate_radius(int32_t min_dimension) {
  // Constraint: radius less than 1/6 the min dimension, at least 5
  return (rand() % (min_dimension / 6)) + 5; // NOLINT(*-msc50-cpp)
}

/**
 * Checks whether the pixel to look at is in the bounds of the image.
 * Used to prevent accessing memory that doesn't belong to the image array.
 * @param img: the image we're checking against
 * @param row: the row index of the pixel to check
 * @param col: the column index of the pixel to check
 * @return: 0 if the pixel is a valid pixel, 1 otherwise
 */
static bool pixel_in_bounds(const Image *img, int row, int col) {
  if (row < 0 || row >= (int) img->height || col < 0 || col >= (int) img->
      width) {
    return false;
  }
  return true;
}

/**
 * Clamps the integer value to fit in an RGB value
 * @param value the integer value to clamp
 * @return the clamped value, an unsigned char
 */
static rgb_value clamp_to_pixel(int value) {
  if (value <= 0) {
    return 0;
  }
  return UCHAR_MAX;
}

static void flood_fill(Image *img,
                       int h,
                       int k,
                       int radius,
                       int x,
                       int y) {
  // if the pixel is out of bounds, return
  if (!pixel_in_bounds(img, x, y)) return;
  // get the distance between pixels
  const double distance_from_center = sqrt((pow(x - h, 2)) + (pow(y - k, 2)));
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
}
