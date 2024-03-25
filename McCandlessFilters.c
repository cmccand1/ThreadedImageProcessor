/**
* (basic description of the program or class)
*
* Completion time: (estimation of hours spent on this program)
*
* @author Clint McCandless
* @version 11/8/23
*/
//TODO: refactor functions into proper header/source files. Currently a little convoluted
////////////////////////////////////////////////////////////////////////////////
//INCLUDES
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "BMPHandler.h"
#include "Image.h"

struct Pixel **create_pixel_array_2d(int width, int height);

void free_pixel_array_2d(struct Pixel **pArr, int width, int height);

void image_apply_cheese(Image *img);

void image_apply_boxblur(Image *img);

void *image_apply_t_boxblur(void *data);

void image_apply_t_cheese(Image *img);

int pixel_in_bounds(Image *img, int row, int col);

typedef enum Filter Filter;

void filter(Image *img, Filter type);

void *t_filter(void *data);

int same_array(struct Pixel **a1, struct Pixel **a2, int lower, int upper);

void flood_fill(Image *img, int h, int k, int radius, int x, int y);

int generate_radius(int min_dimenstion);

////////////////////////////////////////////////////////////////////////////////
//MACRO DEFINITIONS
#define NUM_HOLES(smallest_dim) (int) ((smallest_dim)*(0.08))
#define AVG_RADIUS(smallest_dim) (int) ((smallest_dim)*(0.08))
// the modulus with which we'll generate random numbers
#define RAND_MOD 12 // P(0,1,2) = 0.25, P(3,4,5,6,7,8) = 0.5, P(9,10,11) = 0.25
#define min(a, b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

//problem assumptions
#define BMP_HEADER_SIZE 14
#define BMP_DIB_HEADER_SIZE 40
#define MAXIMUM_IMAGE_SIZE 4096
#define NEIGHBORHOOD_SIZE 3
#define THREAD_COUNT 12

//TODO: finish me


////////////////////////////////////////////////////////////////////////////////
//DATA STRUCTURES

enum Filter {
  BOXBLUR,
  CHEESE
};

struct thread_info {
  Filter filter_type;
  struct Pixel **t_pArr; // the smaller pixel array that this thread owns
  int t_pArr_width, t_pArr_height;
  Image *og_img; // pointer to the image we're modifying from this thread
  int img_start, img_end; // the index of where this threads window onto the og_img starts/ends
};

////////////////////////////////////////////////////////////////////////////////
//MAIN PROGRAM CODE


Filter filter_type;


int main(int argc, char *argv[]) {

  struct BMP_Header BMP;
  struct DIB_Header DIB;
  FILE *input_file, *output_file;
  Image *img;
  struct Pixel **pixels;
  int opt;
  pthread_t tids[THREAD_COUNT];


  // get command line arguments (-i (input file), -o (output file), -f (filter))
  // ASSUME: all CLA's will be provided and all files exist
  optarg = NULL;
  while ((opt = getopt(argc, argv, "i:o:f:")) != -1) {
    if (argc != 6 + 1) {
      fprintf(stderr, "Expected 6 arguments, got %d instead.\nUsage: -i <input file> -o <output file> -f <filter>\n", argc - 1);
      return EXIT_FAILURE;
    }
    switch (opt) {
      case 'i':
        // open the input file to read binary
        if ((input_file = fopen(optarg, "rb")) == NULL) {
          perror("Input file could not be opened.");
          return EXIT_FAILURE;
        }
        break;
      case 'o':
        // create an output file to write binary
        if ((output_file = fopen(optarg, "wb")) == NULL) {
          perror("Output file could not be opened.");
          return EXIT_FAILURE;
        }
        break;
      case 'f':
        if (strcmp(optarg, "b") == 0)
          filter_type = BOXBLUR;
        else if (strcmp(optarg, "c") == 0)
          filter_type = CHEESE;
        else
          // do nothing
          break;
      case '?':
      default:
        fprintf(stderr, "Usage: %s -i <input file> -o <output file> -f <filter>\n", argv[0]);

    }
  }

  // read BMP header and DIB headerfrom input file
  readBMPHeader(input_file, &BMP);
  readDIBHeader(input_file, &DIB);

  // create pixel array
  if((pixels = create_pixel_array_2d(DIB.image_width_w, DIB.image_height_h)) == NULL){
        return EXIT_FAILURE;
  };

  // readPixels into pixel array
  readPixelsBMP(input_file, pixels, DIB.image_width_w, DIB.image_height_h);
  // close input file
  fclose(input_file);
  // create a new image
  img = image_create(pixels, DIB.image_width_w, DIB.image_height_h);

  // allocate enough memory for THREAD_COUNT pointers to thread_info structs
  struct thread_info **infos;
  if ((infos = malloc(sizeof(struct thread_info *) * THREAD_COUNT)) == NULL) {
    perror("Error while allocating memory for thread_info pointers.");
    return EXIT_FAILURE;
  };

  // for each 'tid', allocate memory for a pointer to a thread_info struct
  for (int i = 0; i < THREAD_COUNT; ++i) {
    if ((infos[i] = malloc(sizeof(struct thread_info))) == NULL){
        perror("Error while allocating memory for thread_info struct.\n");
        return EXIT_FAILURE;
    };
    infos[i]->t_pArr_width =
        (img->width / THREAD_COUNT) + 2; // 2 extra columns for padding
    infos[i]->t_pArr_height = img->height;
    // alloate memory for this threads pixel array
    if ((infos[i]->t_pArr = create_pixel_array_2d(infos[i]->t_pArr_width,
                                             infos[i]->t_pArr_height)) == NULL) {
      return EXIT_FAILURE;
    };
    infos[i]->og_img = img; // reference to the og image
    // get the image indicies that correspond to this threads window bounds
    if (i == 0) // first thread (hanging off left edge, i.e., -1)
      infos[i]->img_start = -1;
    else
      infos[i]->img_start = infos[i - 1]->img_end - 1;

    infos[i]->img_end = (infos[i]->img_start + infos[i]->t_pArr_width) - 1;


    // copy the image pixels from this threads range into its pixel array
    for (int j = 0; j < img->height; ++j) {
      for (int k = infos[i]->img_start; k < infos[i]->t_pArr_width; ++k) {
        if (pixel_in_bounds(img, j, k) == 0) {
          infos[i]->t_pArr[j][k].r = img->pArr[j][k].r;
          infos[i]->t_pArr[j][k].g = img->pArr[j][k].g;
          infos[i]->t_pArr[j][k].b = img->pArr[j][k].b;
        }
      }
    }
    // check that the data was copied successfully
    // same_array(infos[i]->t_pArr, img->pArr, infos[i]->img_start, infos[i]->t_pArr_width);

  }

  for (int i = 0; i < THREAD_COUNT; ++i) {
    // create the threads, which call filter
    pthread_create(&tids[i], NULL, t_filter, infos[i]);
  }

  for (int i = 0; i < THREAD_COUNT; ++i) {
    pthread_join(tids[i], NULL);
  }

  // write new headers and pixels to output file
  writeBMPHeader(output_file, &BMP);
  writeDIBHeader(output_file, &DIB);
  writePixelsBMP(output_file,
                 pixels,
                 image_get_width(img),
                 image_get_height(img));

  // destroy the image
  image_destroy(&img);
  fclose(output_file);

  // clean up
  for (int i = 0; i < THREAD_COUNT; ++i) {
    free_pixel_array_2d(infos[i]->t_pArr,
                        infos[i]->t_pArr_width,
                        infos[i]->t_pArr_height);
    free(infos[i]);
    infos[i] = NULL;
  }
  free(infos);
  infos = NULL;
  return 0;
}

int same_array(struct Pixel **p1, struct Pixel **p2, int lower, int upper) {
  for (int i = 0; i < 152; ++i) {
    for (int j = lower; j < upper; ++j) {
      if (p1[i][j].r != p2[i][j].r || p1[i][j].g != p2[i][j].g ||
          p1[i][j].b != p2[i][j].b) {
        printf("arrays different\n");
        return 1;
      }
    }
  }
  printf("same arrays\n");
  return 0;
}

struct Pixel **create_pixel_array_2d(int width, int height) {
  struct Pixel **pixels;
  if ((pixels = malloc(sizeof(struct Pixel *) * height)) == NULL) {
    perror("Error while allocating memory for 2d pixel array.\n");
    exit(EXIT_FAILURE);
  };
  for (int i = 0; i < height; ++i) {
    if((pixels[i] = malloc(sizeof(struct Pixel) * width)) == NULL) {
        perror("Error while allocating memory for 2d pixel array.\n");
        exit(EXIT_FAILURE);
    };
  }
  return pixels;
}

void free_pixel_array_2d(struct Pixel **pArr, int width, int height) {
  //TODO: add error handling (when pointer can't be freed...)
  for (int i = 0; i < height; ++i) {
    free(pArr[i]);
    pArr[i] = NULL;
  }
  free(pArr);
  pArr = NULL;
}

void flood_fill(Image *img, int h, int k, int radius, int x, int y) {
  int distance_from_center;
  // if the pixel is out of bounds, return
  if (pixel_in_bounds(img, x, y) == 0)
    return;
  // get the distance between pixels
  distance_from_center = sqrt((pow(x - h, 2)) + (pow(y - k, 2)));
  if (distance_from_center > radius)
    return;
  // if the pixel is already black, return to avoid infinite loop
  if (img->pArr[x][y].r == 0 && img->pArr[x][y].g == 0 &&
      img->pArr[x][y].b == 0)
    return;
  // if the pixel is within the radius, paint it black
  img->pArr[x][y].r = 0;
  img->pArr[x][y].g = 0;
  img->pArr[x][y].b = 0;
  // do the same to its 4 neighbors
  flood_fill(img, h, k, radius, x - 1, y);
  flood_fill(img, h, k, radius, x + 1, y);
  flood_fill(img, h, k, radius, x, y - 1);
  flood_fill(img, h, k, radius, x, y + 1);
  return;
}

/**
 * Generates a random radius that is some fraction of the smallest image dimension
 * @param min_dimension
 * @return
 */
int generate_radius(int min_dimension) {
  // Constraint: radius less than 1/6 the min dimension, at least 5
  return (rand() % (min_dimension / 6)) + 5;
}

/**
 * Applies the swiss cheese filter to the image.
 * @param img the image to filter
 */
void image_apply_cheese(Image *img) {
  int h, k, radius, min_dimension, seed;
  // tint the image yellow ;P
  image_apply_bw(img);
  image_apply_colorshift(img, 150, 150, 0);
  min_dimension = min(img->width, img->height);
  srand(time(NULL));
  for (int i = 0; i < NUM_HOLES(min_dimension); ++i) {
    // pick random center points (h,k)
    h = rand() % min_dimension; // this is bad for important applications!
    k = rand() % min_dimension;
    // for each center point, generate a random radius
    radius = generate_radius(min_dimension);
    // flood fill from center to fill the circle
    flood_fill(img, h, k, radius, h, k);
  }
  printf("swiss cheese applied\n");
}

/**
 * Checks whether the pixel to look at is in the bounds of the image.
 * Used to prevent accessing memory that doesn't belong to the image array.
 * @param img: the image we're checking against
 * @param row: the row index of the pixel to check
 * @param col: the column index of the pixel to check
 * @return: 0 if the pixel is a valid pixel, 1 otherwise
 */
int pixel_in_bounds(Image *img, int row, int col) {
  if (row < 0 || row >= img->height || col < 0 || col >= img->width)
    return 1;  // indices are out of bounds
  return 0;      // indices are within bounds
}


void *image_apply_t_boxblur(void *data) {
  struct thread_info *t_info = (struct thread_info *) data;
  int valid_neighbors;  // the number of valid pixels
  int rSum, gSum, bSum; // sum total of each RGB component
  struct Pixel **pixels;

  // get a pointer to the first pixel in the original image
  pixels = t_info->og_img->pArr;

  // apply box blur to each pixel in the original array (that this thread can see)
  for (int i = 0; i < t_info->t_pArr_height; ++i) {
    for (int j = t_info->img_start + 1; j < t_info->t_pArr_width; ++j) {
      //printf("i, j: %d, %d\n", i, j);
      // keep track of RGB component sums and valid pixel count
      rSum = gSum = bSum = valid_neighbors = 0;
      // search the 3x3 neighborhood (offsets: -1, 0, 1)
      for (int k = -1; k < NEIGHBORHOOD_SIZE - 1; ++k) {
        for (int l = -1; l < NEIGHBORHOOD_SIZE - 1; ++l) {
          // when we find a valid neighbor, add its RGB values to RGB sums, and increment valid neighbors count
          if (pixel_in_bounds(t_info->og_img, i + k, j + l) == 0) {
            ++valid_neighbors;
            // add the sum of the pixel at the offset
            rSum += t_info->t_pArr[i + k][j + l].r;
            gSum += t_info->t_pArr[i + k][j + l].g;
            bSum += t_info->t_pArr[i + k][j + l].b;
          }
        }
      }
      // update the pixels in the original pixel array
      pixels[i][j].r = clamp_to_uchar(rSum /= valid_neighbors);
      pixels[i][j].g = clamp_to_uchar(gSum /= valid_neighbors);
      pixels[i][j].b = clamp_to_uchar(bSum /= valid_neighbors);
    }
  }
  pthread_exit(NULL);
}

/**
 * Applies the boxblur filter to the image.
 * @param img the image to filter
 */
void image_apply_boxblur(Image *img) {
  int valid_neighbors;  // the number of valid pixels
  int rSum, gSum, bSum; // sum total of each RGB component
  struct Pixel **pixels, **temp;

  // get a pointer to the first pixel in the pixel array
  pixels = img->pArr;
  // allocate temporary memory for a copy of the pixels
  if ((temp = create_pixel_array_2d(img->width, img->height)) == NULL) {
    fprintf(stderr, "Error while allocating memory for temporary pixel array.\n");
    return;
  }

  // copy the pixels to temp array
  for (int i = 0; i < img->height; ++i) {
    for (int j = 0; j < img->width; ++j) {
      temp[i][j].r = pixels[i][j].r;
      temp[i][j].g = pixels[i][j].g;
      temp[i][j].b = pixels[i][j].b;
    }
  }

  // apply box blur to each pixel in the original array
  for (int i = 0; i < img->height; ++i) {
    for (int j = 0; j < img->width; ++j) {
      // keep track of RGB component sums and valid pixel count
      rSum = gSum = bSum = valid_neighbors = 0;
      // search the 3x3 neighborhood (offsets: -1, 0, 1)
      for (int k = -1; k < NEIGHBORHOOD_SIZE - 1; ++k) {
        for (int l = -1; l < NEIGHBORHOOD_SIZE - 1; ++l) {
          // when we find a valid neighbor, add its RGB values to RGB sums, and increment valid neighbors count
          if (pixel_in_bounds(img, i + k, j + l) == 1) {
            ++valid_neighbors;
            // add the sum of the pixel at the offset
            rSum += temp[i + k][j + l].r;
            gSum += temp[i + k][j + l].g;
            bSum += temp[i + k][j + l].b;
          }
        }
      }
      // update the pixels in the original pixel array
      pixels[i][j].r = clamp_to_uchar(rSum /= valid_neighbors);
      pixels[i][j].g = clamp_to_uchar(gSum /= valid_neighbors);
      pixels[i][j].b = clamp_to_uchar(bSum /= valid_neighbors);
    }
  }
  // free the temp array
  free_pixel_array_2d(temp, img->width, img->height);

  printf("boxblur applied\n");
}

void *t_filter(void *data) {
  struct thread_info *t_info = (struct thread_info *) data;
  switch (t_info->filter_type) {
    case BOXBLUR:image_apply_t_boxblur(data);
      break;
    case CHEESE:
      //image_apply_t_cheese(data);
      break;
    default:break;
  }
  pthread_exit(0);
}

/**
 * Applies the filter specified by the command line arguments to the image.
 * @param img the image to filter
 */
void filter(Image *img, Filter type) {
  switch (type) {
    case BOXBLUR:image_apply_boxblur(img);
      break;
    case CHEESE:image_apply_cheese(img);
      break;
  }
}