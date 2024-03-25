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


#define MAX_RGB 255
#define MIN_RGB 0

////////////////////////////////////////////////////////////////////////////////
// Include Files
#include <stdio.h>
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////
// Type Definitions
typedef struct Image Image;

struct Image {
	struct Pixel** pArr;
	int width;
	int height;
};

struct Pixel{
    unsigned char r; // 8-bit rvalue
    unsigned char g; // 8-bit gvalue
    unsigned char b; // 8-bit bvalue
};

////////////////////////////////////////////////////////////////////////////////
//Function Declarations

/** Creates a new image and returns it.
* 
 * @param  pArr: Pixel array of this image.
 * @param  width: Width of this image.
 * @param  height: Height of this image.
 * @return A pointer to a new image.
*/
Image* image_create(struct Pixel** pArr, int width, int height);


/** Destroys an image. Does not deallocate internal pixel array.
* 
* @param  img: the image to destroy.
*/
void image_destroy(Image** img);

/** Returns a double pointer to the pixel array.
*
* @param  img: the image.
*/
struct Pixel** image_get_pixels(Image* img);

/** Returns the width of the image.
*
* @param  img: the image.
*/
int image_get_width(Image* img);

/** Returns the height of the image.
*
* @param  img: the image.
*/
int image_get_height(Image* img);

/** Converts the image to grayscale.
*
* @param  img: the image.
*/
void image_apply_bw(Image* img);

/**
 * Shift color of the internal Pixel array. The dimension of the array is width * height.
 * The shift value of RGB is rShift, gShift，bShift. Useful for color shift.
 *
 * @param  img: the image.
 * @param  rShift: the shift value of color r shift
 * @param  gShift: the shift value of color g shift
 * @param  bShift: the shift value of color b shift
 */
void image_apply_colorshift(Image* img, int rShift, int gShift, int bShift);

/**
 * Converts the image to grayscale. If the scaling factor is less than 1 the new image will be
 * smaller, if it is larger than 1, the new image will be larger.
 *
 * @param  img: the image.
 * @param  factor: the scaling factor
*/
void image_apply_resize(Image* img, float factor);


/**
 * Clamps the integer value to fit in an unsigned char.
 * @param value the integer value to clamp
 * @return the clamped value, an unsigned char
 */
int clamp_to_uchar(int value);

#endif