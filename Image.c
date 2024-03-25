//
// Created by Clint McCandless on 10/26/23.

#include "Image.h"

#define MAX_RGB 255
#define MIN_RGB 0



/** Creates a new image and returns it.
*
* @param  pArr: Pixel array of this image.
* @param  width: Width of this image.
* @param  height: Height of this image.
* @return A pointer to a new image.
*/
Image* image_create(struct Pixel** pArr, int width, int height) {
    // allocate memory for new Image
    Image *img;

    if ((img = malloc(sizeof(Image))) == NULL) {
        perror("Failed to allocate memory in image_create().\n");
        exit(EXIT_FAILURE);
    }
    //TODO: should we also handle the pixel array here? Or leave it in main?

    // initialize new Image
    img->height = height;
    img->width = width;
    img->pArr = pArr;
    // return pointer to new Image
    return img;
}



/** Destroys an image. Does not deallocate internal pixel array.
*
* @param  img: the image to destroy.
*/
void image_destroy(Image** img) {
    // TODO: comment out pixel array deletion for submission
    struct Pixel **pixels = (*img)->pArr;
    for (int i = 0; i < image_get_height((*img)); ++i) {
        free(pixels[i]);
        pixels[i] = NULL;
    }
    free(pixels);
    pixels = NULL;
    // deallocate the memory for this image
    free(*img);
    // prevent use-after-free bug
    *img = NULL;
}

/** Returns a double pointer to the pixel array.
* @param  img: the image.
*/
struct Pixel** image_get_pixels(Image* img) {
    return img->pArr;
}

/** Returns the width of the image.
*
* @param  img: the image.
*/
int image_get_width(Image* img) {
    return img->width;
}

/** Returns the height of the image.
*
* @param  img: the image.
*/
int image_get_height(Image* img) {
    return img->height;
}

/** Converts the image to grayscale.
*
* @param  img: the image.
*/
void image_apply_bw(Image* img) {
    // get a pointer to the first pixel in the pixel array
    struct Pixel **pixels = img->pArr;
    // for each pixel, change each RGB component to the same value
    for (int i = 0; i < img->width; ++i) {
        for (int j = 0; j < img->height; ++j) {
            // calculate the grayscale value
            const unsigned char GRAYSCALE_VALUE = (0.299*pixels[i][j].r) + (0.587*pixels[i][j].g) + (0.114*pixels[i][j].b);
            // set each RGB component to the calculated grayscale value
            pixels[i][j].r = GRAYSCALE_VALUE;
            pixels[i][j].g = GRAYSCALE_VALUE;
            pixels[i][j].b = GRAYSCALE_VALUE;
        }
    }
}

/**
 * Shift color of the internal Pixel array. The dimension of the array is width * height.
 * The shift value of RGB is rShift, gShift，bShift. Useful for color shift.
 *
 * @param  img: the image.
 * @param  rShift: the shift value of color r shift
 * @param  gShift: the shift value of color g shift
 * @param  bShift: the shift value of color b shift
 */
void image_apply_colorshift(Image* img, int rShift, int gShift, int bShift) {
    // get a pointer to the first pixel in the pixel array
    struct Pixel **pixels = img->pArr;
    for (int i = 0; i < img->width; ++i) {
        for (int j = 0; j < img->height; ++j) {
            pixels[i][j].r = clamp_to_uchar(pixels[i][j].r + rShift);
            pixels[i][j].g = clamp_to_uchar(pixels[i][j].g + gShift);
            pixels[i][j].b = clamp_to_uchar(pixels[i][j].b + bShift);
        }
    }
}

/**
 * Converts the image to grayscale. If the scaling factor is less than 1 the new image will be
 * smaller, if it is larger than 1, the new image will be larger.
 *
 * @param  img: the image.
 * @param  factor: the scaling factor
*/
void image_apply_resize(Image* img, float factor) {
    //TODO: implement this
}

/**
 * Clamps the integer value to fit in an unsigned char.
 * @param value the integer value to clamp
 * @return the clamped value, an unsigned char
 */
int clamp_to_uchar(int value) {
    if (value < MIN_RGB) {
        return MIN_RGB;
    } else if (value > MAX_RGB) {
        return MAX_RGB;
    } else {
        return value;
    }
}
