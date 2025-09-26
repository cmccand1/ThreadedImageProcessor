#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "headers/BMPHandler.h"
#include "headers/Image.h"
#include "headers/macros.h"

typedef void *(*filter_method)(void *);

/**
 * Structure to hold program options.
 */
typedef struct {
  char input_filename[PATH_MAX]; /**< Input filename buffer */
  char output_filename[PATH_MAX]; /**< Output filename buffer */
  filter_method filter_func; /**< Function pointer to the filter method */
  int rShift; /**< Red color shift value */
  int gShift; /**< Green color shift value */
  int bShift; /**< Blue color shift value */
} ProgramOptions;

/**
 * Display usage information for the program.
 * @param argv Array of command-line arguments.
 */
void display_usage(char **argv);

/**
 * Initialize thread data for image processing.
 * @param data Pointer to the thread data array.
 * @param image Pointer to the image structure.
 * @param rShift Pointer to the red color shift value.
 * @param gShift Pointer to the green color shift value.
 * @param bShift Pointer to the blue color shift value.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
int init_thread_data(ThreadData ***data,
                     const Image *image,
                     const int *rShift,
                     const int *gShift,
                     const int *bShift);

/**
 * Process user arguments and populate program options.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param options Pointer to the ProgramOptions structure.
 */
void process_user_args(int argc, char **argv, ProgramOptions *options);

/**
 * Extract input image data from the input file.
 * @param input_file Pointer to the input file.
 * @param input_pixels Pointer to the 2D array of input pixels.
 * @param BMP Pointer to the BMP header structure.
 * @param DIB Pointer to the DIB header structure.
 */
int extract_input_image_data(char *input_filename,
                             Pixel ***input_pixels,
                             BMPHeader *BMP,
                             DIBHeader *DIB);

/**
 * Write the output file with the filtered image.
 * @param output_file Pointer to the output file.
 * @param image Pointer to the image structure.
 * @param new_pixels Pointer to the 2D array of new pixels.
 * @param BMP BMP header structure.
 * @param DIB DIB header structure.
 */
int write_output_file(char *output_filename,
                      const Image *image,
                      Pixel **new_pixels,
                      BMPHeader BMP,
                      DIBHeader DIB);

/**
 * Perform the specified filter on the input image.
 * @param input_image Pointer to the input image structure.
 * @param filter_func Function pointer to the filter method.
 * @param job_data Pointer to the thread data array.
 * @param tids Array of thread IDs.
 */
void do_filter(const Image *input_image,
               filter_method filter_func,
               ThreadData ***job_data,
               pthread_t tids[THREAD_COUNT]);

/**
 * Write the output pixels to the output pixel array.
 * @param output_pixels Pointer to the 2D array of output pixels.
 * @param job_data Pointer to the thread data array.
 */
void write_output_pixels(Pixel **output_pixels,
                         ThreadData **job_data);

/**
 * Initialize the input image from the input file.
 * @param input_file Pointer to the input file.
 * @param input_image Pointer to the input image structure.
 * @param BMP Pointer to the BMP header structure.
 * @param DIB Pointer to the DIB header structure.
 */
int init_input_image(char *input_filename,
                     Image **input_image,
                     BMPHeader *BMP,
                     DIBHeader *DIB);

/**
 * Perform the filtering process on the input image.
 * @param input_image Pointer to the input image structure.
 * @param job_data Pointer to the thread data array.
 * @param tids Array of thread IDs.
 * @param options Pointer to the ProgramOptions structure.
 */
int perform_filtering(const Image *input_image,
                      ThreadData ***job_data,
                      pthread_t tids[THREAD_COUNT],
                      const ProgramOptions *options);

/**
 * Clean up resources allocated during the program execution.
 * @param input_image Pointer to the input image structure.
 * @param output_image Pointer to the output image structure.
 * @param job_data Pointer to the thread data array.
 */
void cleanup_resources(Image *input_image,
                       Image *output_image,
                       ThreadData **job_data);

/**
 * Write the output image to the output file.
 * @param output_file Pointer to the output file.
 * @param input_image Pointer to the input image structure.
 * @param job_data Pointer to the thread data array.
 * @param output_image Pointer to the output image structure.
 * @param BMP Pointer to the BMP header structure.
 * @param DIB Pointer to the DIB header structure.
 */
int write_output(char *output_file,
                 const Image *input_image,
                 ThreadData **job_data,
                 Image **output_image,
                 const BMPHeader *BMP,
                 const DIBHeader *DIB);

int main(int argc, char *argv[]) {
  // Define program options
  ProgramOptions options = {0};
  BMPHeader BMP;
  DIBHeader DIB;
  FILE *input_file = nullptr;
  FILE *output_file = nullptr;
  Image *input_image = nullptr;
  Image *output_image = nullptr;
  ThreadData **job_data = nullptr;
  pthread_t tids[THREAD_COUNT];

  // Parse user arguments
  process_user_args(argc, argv, &options);

  // Initialize input image
  if ((init_input_image(options.input_filename,
                        &input_image,
                        &BMP,
                        &DIB)) != EXIT_SUCCESS) {
    perror("Error initializing input image.");
    goto cleanup;
  }

  // Perform filtering
  if ((perform_filtering(input_image, &job_data, tids, &options)) !=
      EXIT_SUCCESS) {
    perror("Error occurred during filtering.");
    goto cleanup;
  }

  // Write output
  if ((write_output(options.output_filename,
                    input_image,
                    job_data,
                    &output_image,
                    &BMP,
                    &DIB)) != EXIT_SUCCESS) {
    perror("Error writing output image.");
    goto cleanup;
  }
  return EXIT_SUCCESS;

cleanup:
  if (input_file) fclose(input_file);
  if (output_file) fclose(output_file);
  if (input_image) image_destroy(&input_image);
  if (output_image) image_destroy(&output_image);
  // Free thread data
  if (job_data) {
    for (int i = 0; i < THREAD_COUNT; ++i) {
      free_pixel_array_2d(job_data[i]->thread_pixel_array,
                          (size_t) job_data[i]->height);
      FREE(job_data[i]);
    }
    FREE(job_data);
  }

  return EXIT_FAILURE;
}

void cleanup_resources(Image *input_image,
                       Image *output_image,
                       ThreadData **job_data) {
  // Free input and output images
  image_destroy(&input_image);
  image_destroy(&output_image);

  // Free thread data
  for (int i = 0; i < THREAD_COUNT; ++i) {
    free_pixel_array_2d(job_data[i]->thread_pixel_array,
                        (size_t) job_data[i]->height);
    FREE(job_data[i]);
  }
  FREE(job_data);
}

int write_output(char *output_file,
                 const Image *input_image,
                 ThreadData **job_data,
                 Image **output_image,
                 const BMPHeader *BMP,
                 const DIBHeader *DIB) {
  // Create output pixel array
  Pixel **output_pixels =
      create_pixel_array_2d((size_t) input_image->width,
                            (size_t) input_image->height);
  write_output_pixels(output_pixels, job_data);

  // Create the output image
  *output_image = image_create(output_pixels,
                               input_image->width,
                               input_image->height);
  if (!*output_image) {
    perror("Error creating output image.");
    return EXIT_FAILURE;
  }

  // Write the output file
  if ((write_output_file(output_file, *output_image, output_pixels, *BMP, *DIB))
      != EXIT_SUCCESS) {
    perror("Error writing output file.");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int perform_filtering(const Image *input_image,
                      ThreadData ***job_data,
                      pthread_t tids[THREAD_COUNT],
                      const ProgramOptions *options) {
  // Initialize thread data
  if (init_thread_data(job_data,
                       input_image,
                       &options->rShift,
                       &options->gShift,
                       &options->bShift) != EXIT_SUCCESS) {
    perror("Error initializing thread info.");
    return EXIT_FAILURE;
  }

  // Perform filtering
  for (int i = 0; i < THREAD_COUNT; ++i) {
    if (pthread_create(&tids[i],
                       nullptr,
                       options->filter_func,
                       (*job_data)[i]) !=
        EXIT_SUCCESS) {
      perror("Error creating thread.");
      return EXIT_FAILURE;
    }
  }

  // Wait for threads to finish
  for (int i = 0; i < THREAD_COUNT; ++i) {
    if (pthread_join(tids[i], nullptr) != EXIT_SUCCESS) {
      perror("Error joining thread.");
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int init_input_image(char *input_filename,
                     Image **input_image,
                     BMPHeader *BMP,
                     DIBHeader *DIB) {
  Pixel **pixels = nullptr;

  // Extract input pixels
  if (extract_input_image_data(input_filename, &pixels, BMP, DIB) !=
      EXIT_SUCCESS) {
    perror("Error extracting input image data.");
    return EXIT_FAILURE;
  }

  // Create input image
  *input_image = image_create(pixels, DIB->image_width_w, DIB->image_height_h);
  if (!*input_image) {
    perror("Error creating input image.");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

void write_output_pixels(Pixel **output_pixels,
                         ThreadData **job_data) {
  // Copy thread pixel arrays to output pixel array
  for (size_t i = 0; i < THREAD_COUNT; ++i) {
    for (size_t row = 0; row < job_data[i]->height; ++row) {
      for (size_t col = job_data[i]->start; col <= job_data[i]->end; ++col) {
        output_pixels[row][col] =
            job_data[i]->thread_pixel_array[row][col - job_data[i]->start];
      }
    }
  }
}

int init_thread_data(ThreadData ***data,
                     const Image *image,
                     const int32_t *rShift,
                     const int32_t *gShift,
                     const int32_t *bShift) {
  // Allocate memory for thread_data pointers
  if ((*data = malloc(sizeof(ThreadData *) * THREAD_COUNT)) ==
      nullptr) {
    perror("Error while allocating memory for thread_info pointers.");
    return EXIT_FAILURE;
  }

  // Calculate width distribution among threads
  const int32_t width_per_thread = image->width / THREAD_COUNT;
  const int32_t remaining_width = image->width % THREAD_COUNT;

  // Log for debugging
  printf("Thread count: %d\n", THREAD_COUNT);
  printf("Width per thread: %d, remainder: %d\n",
         width_per_thread,
         remaining_width);

  for (size_t i = 0; i < THREAD_COUNT; ++i) {
    // Allocate individual thread_data structure
    if (((*data)[i] = malloc(sizeof(ThreadData))) == nullptr) {
      perror("Error while allocating memory for thread_info struct.");
      return EXIT_FAILURE;
    }

    // Set basic thread properties
    (*data)[i]->height = (size_t) image->height;
    (*data)[i]->og_image = image;
    (*data)[i]->rShift = *rShift;
    (*data)[i]->gShift = *gShift;
    (*data)[i]->bShift = *bShift;

    // Calculate thread's section boundaries
    if (i == 0) {
      (*data)[i]->start = 0;
    } else {
      (*data)[i]->start = (*data)[i - 1]->end + 1;
    }

    if (i == THREAD_COUNT - 1) {
      // Last thread gets any remaining columns
      (*data)[i]->end = (size_t) image->width - 1;
    } else {
      (*data)[i]->end = (*data)[i]->start + (size_t)
                        width_per_thread - 1;
    }

    // Calculate thread's width
    (*data)[i]->width =
        (*data)[i]->end - (*data)[i]->start + 1;

    // Allocate memory for thread pixel array
    if (((*data)[i]->thread_pixel_array = create_pixel_array_2d(
           (*data)[i]->width,
           (*data)[i]->height)) == nullptr) {
      perror("Error while allocating memory for thread_info pixel array.");
      return EXIT_FAILURE;
    }

    // Log for debugging
    printf("Thread %-3zu: start: %-3zu, end: %-3zu, width: %-3zu\n",
           i,
           (*data)[i]->start,
           (*data)[i]->end,
           (*data)[i]->width);
  }
  return EXIT_SUCCESS;
}

void process_user_args(int argc, char **argv, ProgramOptions *options) {
  int opt;
  while ((opt = getopt(argc, argv, "i:o:f:r:g:b:")) != -1) {
    // if (argc != 6 + 1) {
    //   fprintf(stderr, "Expected 6 arguments, got %d instead.\n", argc - 1);
    //   display_usage(argv);
    //   exit(EXIT_FAILURE);
    // }
    switch (opt) {
      case 'i':
        snprintf(options->input_filename,
                 sizeof(options->input_filename),
                 "%s",
                 optarg);
        break;
      case 'o':
        snprintf(options->output_filename,
                 sizeof(options->output_filename),
                 "%s",
                 optarg);
        break;
      case 'f':
        switch (optarg[0]) {
          case 'b':
            options->filter_func = image_apply_t_boxblur;
            break;
          case 'c':
            options->filter_func = image_apply_t_cheese;
            break;
          case 'g':
            options->filter_func = image_apply_t_bw;
            break;
          case 's':
            options->filter_func = image_apply_t_colorshift;
            break;
          default:
            fprintf(stderr, "Invalid filter type: %s\n", optarg);
            display_usage(argv);
            exit(EXIT_FAILURE);
        }
        break;
      case 'r':
        options->rShift = atoi(optarg);
        break;
      case 'g':
        options->gShift = atoi(optarg);
        break;
      case 'b':
        options->bShift = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Invalid option: %c\n", opt);
        display_usage(argv);
        exit(EXIT_FAILURE);
    }
  }
}

int extract_input_image_data(char *input_filename,
                             Pixel ***input_pixels,
                             BMPHeader *BMP,
                             DIBHeader *DIB) {
  FILE *input_file = nullptr;

  input_file = fopen(input_filename, "rb");
  if (!input_file) {
    perror("Input file could not be opened.");
    return EXIT_FAILURE;
  }

  // read headers from input file
  readBMPHeader(input_file, BMP);
  readDIBHeader(input_file, DIB);

  // allocate memory for input pixel array
  *input_pixels = create_pixel_array_2d((size_t) DIB->image_width_w,
                                        (size_t) DIB->image_height_h);
  if (!*input_pixels) {
    perror("Error creating pixel array.");
    return EXIT_FAILURE;
  }

  // read pixels from input file
  readPixels(input_file,
             *input_pixels,
             (size_t) DIB->image_width_w,
             (size_t) DIB->image_height_h);

  // close input file
  if (input_file) fclose(input_file);
  return EXIT_SUCCESS;
}

/**
 * Write the output file with the filtered image.
 * @param output_file the output file to write to
 * @param image the image to write to the output file
 * @param new_pixels the new pixels to write to the output file
 * @param BMP the BMP header for the output image
 * @param DIB the DIB header for the output image
 */
int write_output_file(char *output_filename,
                      const Image *image,
                      Pixel **new_pixels,
                      const BMPHeader BMP,
                      const DIBHeader DIB) {
  FILE *output_file = nullptr;

  if ((output_file = fopen(output_filename, "wb")) == nullptr) {
    perror("Output file could not be opened.");
    return EXIT_FAILURE;
  }

  // write headers to output file
  writeBMPHeader(output_file, &BMP);
  writeDIBHeader(output_file, &DIB);

  // write pixels to output file
  writePixels(output_file,
              (const Pixel * const *) new_pixels,
              (size_t) image_get_width(image),
              (size_t) image_get_height(image));

  // close output file
  if (output_file) fclose(output_file);
  return EXIT_SUCCESS;
}

void display_usage(char **argv) {
  fprintf(stderr,
          "Usage: %s -i <input file> -o <output file> -f <filter>\n",
          argv[0]);
}
