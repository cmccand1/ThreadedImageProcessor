/**
 * (basic description of the program or class)
 *
 * Completion time: (estimation of hours spent on this program)
 *
 * @author Clint McCandless
 * @version 11/8/23
 */

////////////////////////////////////////////////////////////////////////////////
// INCLUDES
#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "headers/BMPHandler.h"
#include "headers/Image.h"

int pixel_in_bounds(const Image *img, int row, int col);
typedef enum Filter Filter;
void *t_filter(void *data);
int init_thread_info(struct thread_info ***thread_infos, Image *image);

Filter filter_type;

int main(int argc, char *argv[]) {
  struct BMP_Header BMP;
  struct DIB_Header DIB;
  FILE *input_file = NULL;
  FILE *output_file = NULL;
  Image *image;
  int opt;
  pthread_t tids[THREAD_COUNT];

  // get command line arguments (-i (input file), -o (output file), -f (filter))
  // ASSUME: all CLA's will be provided and all files exist
  optarg = NULL;
  while ((opt = getopt(argc, argv, "i:o:f:")) != -1) {
    if (argc != 6 + 1) {
      fprintf(stderr,
              "Expected 6 arguments, got %d instead.\nUsage: -i <input file> "
              "-o <output file> -f <filter>\n",
              argc - 1);
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
        // set the filter type
        switch (optarg[0]) {
          case 'b':
            filter_type = BOXBLUR;
            break;
          case 'c':
            filter_type = CHEESE;
            break;
          default:
            fprintf(stderr,
                    "Usage: %s -i <input file> -o <output file> -f <filter>\n",
                    argv[0]);
        }
        break;
      default:
        fprintf(stderr,
                "Usage: %s -i <input file> -o <output file> -f <filter>\n",
                argv[0]);
    }
  }

  // read BMP header and DIB headerfrom input file
  readBMPHeader(input_file, &BMP);
  readDIBHeader(input_file, &DIB);

  // create a 2d pixel array to hold the pixels of the image
  struct Pixel **pixels =
      create_pixel_array_2d(DIB.image_width_w, DIB.image_height_h);

  // readPixels into pixel array
  readPixelsBMP(input_file, pixels, DIB.image_width_w, DIB.image_height_h);
  // close input file
  fclose(input_file);
  // create a new image
  image = image_create(pixels, DIB.image_width_w, DIB.image_height_h);

  // allocate enough memory for THREAD_COUNT pointers to thread_info structs
  struct thread_info **thread_infos;
  if (init_thread_info(&thread_infos, image) == 1) {
    perror("Error initializing thread info.");
    return EXIT_FAILURE;
  }

  // create the threads, which call filter
  for (int i = 0; i < THREAD_COUNT; ++i) {
    pthread_create(&tids[i], NULL, t_filter, thread_infos[i]);
  }

  // wait for all threads to finish
  for (int i = 0; i < THREAD_COUNT; ++i) {
    pthread_join(tids[i], NULL);
  }

  // write new headers and pixels to output file
  writeBMPHeader(output_file, &BMP);
  writeDIBHeader(output_file, &DIB);
  writePixelsBMP(output_file, pixels, image_get_width(image),
                 image_get_height(image));

  // destroy the image
  image_destroy(&image);
  fclose(output_file);

  // clean up
  for (int i = 0; i < THREAD_COUNT; ++i) {
    free_pixel_array_2d(thread_infos[i]->thread_pixel_array,
                        thread_infos[i]->width, thread_infos[i]->height);
    // free the thread_info struct
    free(thread_infos[i]);
    thread_infos[i] = NULL;
  }
  free(thread_infos);
  thread_infos = NULL;
  return 0;
}

/**
 * Checks whether the pixel to look at is in the bounds of the image.
 * Used to prevent accessing memory that doesn't belong to the image array.
 * @param img: the image we're checking against
 * @param row: the row index of the pixel to check
 * @param col: the column index of the pixel to check
 * @return: 0 if the pixel is a valid pixel, 1 otherwise
 */
int pixel_in_bounds(const Image *img, int row, int col) {
  if (row < 0 || row >= img->height || col < 0 || col >= img->width)
    return 1;  // indices are out of bounds
  return 0;    // indices are within bounds
}

int init_thread_info(struct thread_info ***thread_infos, Image *image) {
  if ((*thread_infos = malloc(sizeof(struct thread_info *) * THREAD_COUNT)) ==
      NULL) {
    perror("Error while allocating memory for thread_info pointers.");
    return EXIT_FAILURE;
  };

  // for each 'tid', allocate memory for a pointer to a thread_info struct
  for (int i = 0; i < THREAD_COUNT; ++i) {
    if (((*thread_infos)[i] = malloc(sizeof(struct thread_info))) == NULL) {
      perror("Error while allocating memory for thread_info struct.\n");
      // Cleanup on failure
      for (int j = 0; j < i; ++j) {
        free((*thread_infos)[j]);
      }
      free(*thread_infos);
      return EXIT_FAILURE;
    };

    const int width_per_thread = image->width / THREAD_COUNT + 2;
    (*thread_infos)[i]->width = width_per_thread;
    (*thread_infos)[i]->height = image->height;
    // allocate memory for this threads pixel array
    if (((*thread_infos)[i]->thread_pixel_array = create_pixel_array_2d(
             (*thread_infos)[i]->width, (*thread_infos)[i]->height)) == NULL) {
      perror("Error while allocating memory for thread_info pixel array.\n");
      // Cleanup on failure
      for (int j = 0; j < i; ++j) {
        free((*thread_infos)[j]);
      }
      free(*thread_infos);
      return EXIT_FAILURE;
    };

    (*thread_infos)[i]->og_image = image;  // reference to the og image
    // get the image indicies that correspond to this threads window bounds
    if (i == 0)  // first thread (hanging off left edge, i.e., -1)
      (*thread_infos)[i]->start = -1;
    else
      (*thread_infos)[i]->start = (*thread_infos)[i - 1]->end - 1;

    (*thread_infos)[i]->end =
        ((*thread_infos)[i]->start + (*thread_infos)[i]->width) - 1;

    // print the start and end of the window for this thread
    printf("Thread %d: start: %d, end: %d\n", i, (*thread_infos)[i]->start,
           (*thread_infos)[i]->end);
    // copy the image pixels from this threads range into its pixel array
    for (int row = 0; row < image->height; ++row) {
      for (int col = (*thread_infos)[i]->start; col < (*thread_infos)[i]->width;
           ++col) {
        if (pixel_in_bounds(image, row, col) == 0) {
          (*thread_infos)[i]->thread_pixel_array[row][col].r =
              image->pixel_array[row][col].r;
          (*thread_infos)[i]->thread_pixel_array[row][col].g =
              image->pixel_array[row][col].g;
          (*thread_infos)[i]->thread_pixel_array[row][col].b =
              image->pixel_array[row][col].b;
        }
      }
    }
  }
  return 0;
}

void *t_filter(void *data) {
  printf("Thread %d started\n", (int)pthread_self());
  const struct thread_info *thread_info = (struct thread_info *)data;
  switch (thread_info->filter_type) {
    case BOXBLUR:
      image_apply_t_boxblur(data);
      break;
    case CHEESE:
      // image_apply_t_cheese(data);
      break;
    default:
      break;
  }
  pthread_exit(0);
}