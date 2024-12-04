/**
* Specification for an image ADT.
*
* Completion time: ?
*
* @author Clint McCandless, Ruben Acuna
* @version 9/9/2021
*/

#ifndef PixelProcessor_H
#define PixelProcessor_H

////////////////////////////////////////////////////////////////////////////////
// Include Files
#include <stdio.h>
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////
// Macro Definitions
#define NUM_HOLES(smallest_dim) (int)((smallest_dim) * (0.08))
#define AVG_RADIUS(smallest_dim) (int)((smallest_dim) * (0.08))
// the modulus with which we'll generate random numbers
#define RAND_MOD 12  // P(0,1,2) = 0.25, P(3,4,5,6,7,8) = 0.5, P(9,10,11) = 0.25
#define min(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })

#define BMP_HEADER_SIZE 14
#define BMP_DIB_HEADER_SIZE 40
#define MAXIMUM_IMAGE_SIZE 4096
#define NEIGHBORHOOD_SIZE 3
#define THREAD_COUNT 12

////////////////////////////////////////////////////////////////////////////////
// Type Definitions
typedef struct Image Image;

struct Image {
  struct Pixel **pixel_array;
  int width;
  int height;
};

struct Pixel {
  unsigned char r; // 8-bit rvalue
  unsigned char g; // 8-bit gvalue
  unsigned char b; // 8-bit bvalue
};

enum Filter { BOXBLUR, CHEESE };

struct thread_info {
  enum Filter filter_type;
  struct Pixel *
      *thread_pixel_array;  // the smaller pixel array that this thread owns
  int width, height;
  Image *og_image;  // pointer to the image we're modifying from this thread
  int start,
      end;  // the index of where this threads window onto the og_image
            // starts/ends
};

////////////////////////////////////////////////////////////////////////////////
//Function Declarations

struct Pixel **create_pixel_array_2d(int width, int height);

void free_pixel_array_2d(struct Pixel **array, int width, int height);

/** Creates a new image and returns it.
* 
 * @param  pArr: Pixel array of this image.
 * @param  width: Width of this image.
 * @param  height: Height of this image.
 * @return A pointer to a new image.
*/
Image *image_create(struct Pixel **pArr, int width, int height);

/** Destroys an image. Does not deallocate internal pixel array.
* 
* @param  img: the image to destroy.
*/
void image_destroy(Image **img);

/** Returns a double pointer to the pixel array.
*
* @param  img: the image.
*/
struct Pixel **image_get_pixels(const Image *img);

/** Returns the width of the image.
*
* @param  img: the image.
*/
int image_get_width(const Image *img);

/** Returns the height of the image.
*
* @param  img: the image.
*/
int image_get_height(const Image *img);

/** Converts the image to grayscale.
*
* @param  img: the image.
*/
void image_apply_bw(const Image *img);

/**
 * Apply a cheese filter to the image. The cheese filter will apply a yellow
 * tint to the image and add random holes to the image.
 * @param img the image to filter
 */
void image_apply_cheese(const Image *img);

void *image_apply_t_boxblur(void *data);

/**
 * Shift color of the internal Pixel array. The dimension of the array is width * height.
 * The shift value of RGB is rShift, gShift，bShift. Useful for color shift.
 *
 * @param  img: the image.
 * @param  rShift: the shift value of color r shift
 * @param  gShift: the shift value of color g shift
 * @param  bShift: the shift value of color b shift
 */
void image_apply_colorshift(const Image *img, int rShift, int gShift, int bShift);

/**
 * Converts the image to grayscale. If the scaling factor is less than 1 the new image will be
 * smaller, if it is larger than 1, the new image will be larger.
 *
 * @param  img: the image.
 * @param  factor: the scaling factor
*/
void image_apply_resize(Image *img, float factor);

#endif