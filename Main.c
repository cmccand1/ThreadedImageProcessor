#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "headers/BMPHandler.h"
#include "headers/Image.h"

typedef void *(*filter_method)(void *);

/**
 * Structure to hold program options.
 */
typedef struct {
  FILE *input_file;          /**< Input file pointer */
  FILE *output_file;         /**< Output file pointer */
  filter_method filter_func; /**< Function pointer to the filter method */
  int rShift;                /**< Red color shift value */
  int gShift;                /**< Green color shift value */
  int bShift;                /**< Blue color shift value */
} ProgramOptions;

/**
 * Display usage information for the program.
 * @param argv Array of command-line arguments.
 */
void display_usage(char **argv);

/**
 * Initialize thread data for image processing.
 * @param thread_data Pointer to the thread data array.
 * @param image Pointer to the image structure.
 * @param rShift Pointer to the red color shift value.
 * @param gShift Pointer to the green color shift value.
 * @param bShift Pointer to the blue color shift value.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
int init_thread_data(struct thread_data ***thread_data, const Image *image,
                     const int *rShift, const int *gShift, const int *bShift);

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
void extract_input_image_data(FILE *input_file, struct Pixel ***input_pixels,
                              struct BMP_Header *BMP, struct DIB_Header *DIB);

/**
 * Write the output file with the filtered image.
 * @param output_file Pointer to the output file.
 * @param image Pointer to the image structure.
 * @param new_pixels Pointer to the 2D array of new pixels.
 * @param BMP BMP header structure.
 * @param DIB DIB header structure.
 */
void write_output_file(FILE *output_file, const Image *image,
                       struct Pixel **new_pixels, struct BMP_Header BMP,
                       struct DIB_Header DIB);

/**
 * Perform the specified filter on the input image.
 * @param input_image Pointer to the input image structure.
 * @param filter_func Function pointer to the filter method.
 * @param job_data Pointer to the thread data array.
 * @param tids Array of thread IDs.
 */
void do_filter(const Image *input_image, filter_method filter_func,
               struct thread_data ***job_data, pthread_t tids[THREAD_COUNT]);

/**
 * Write the output pixels to the output pixel array.
 * @param output_pixels Pointer to the 2D array of output pixels.
 * @param job_data Pointer to the thread data array.
 */
void write_output_pixels(struct Pixel **output_pixels,
                         struct thread_data **job_data);

/**
 * Initialize the input image from the input file.
 * @param input_file Pointer to the input file.
 * @param input_image Pointer to the input image structure.
 * @param BMP Pointer to the BMP header structure.
 * @param DIB Pointer to the DIB header structure.
 */
void initialize_input_image(FILE *input_file, Image **input_image,
                            struct BMP_Header *BMP, struct DIB_Header *DIB);

/**
 * Perform the filtering process on the input image.
 * @param input_image Pointer to the input image structure.
 * @param job_data Pointer to the thread data array.
 * @param tids Array of thread IDs.
 * @param options Pointer to the ProgramOptions structure.
 */
void perform_filtering(const Image *input_image, struct thread_data ***job_data,
                       pthread_t tids[THREAD_COUNT],
                       const ProgramOptions *options);

/**
 * Clean up resources allocated during the program execution.
 * @param input_image Pointer to the input image structure.
 * @param output_image Pointer to the output image structure.
 * @param job_data Pointer to the thread data array.
 */
void cleanup_resources(Image *input_image, Image *output_image,
                       struct thread_data **job_data);

/**
 * Write the output image to the output file.
 * @param output_file Pointer to the output file.
 * @param input_image Pointer to the input image structure.
 * @param job_data Pointer to the thread data array.
 * @param output_image Pointer to the output image structure.
 * @param BMP Pointer to the BMP header structure.
 * @param DIB Pointer to the DIB header structure.
 */
void write_output(FILE *output_file, const Image *input_image,
                  struct thread_data **job_data, Image **output_image,
                  const struct BMP_Header *BMP, const struct DIB_Header *DIB);

int main(int argc, char *argv[]) {
  // Define program options
  ProgramOptions options = {0};

  // Parse user arguments
  process_user_args(argc, argv, &options);

  // Load input image
  struct BMP_Header BMP;
  struct DIB_Header DIB;
  Image *input_image = NULL;
  initialize_input_image(options.input_file, &input_image, &BMP, &DIB);

  // Perform filtering
  struct thread_data **job_data = NULL;
  pthread_t tids[THREAD_COUNT];
  perform_filtering(input_image, &job_data, tids, &options);

  // Write output
  Image *output_image = NULL;
  write_output(options.output_file, input_image, job_data, &output_image, &BMP,
               &DIB);

  // Clean up resources
  cleanup_resources(input_image, output_image, job_data);

  return EXIT_SUCCESS;
}

void cleanup_resources(Image *input_image, Image *output_image,
                       struct thread_data **job_data) {
  // Free input and output images
  image_destroy(&input_image);
  image_destroy(&output_image);

  // Free thread data
  for (int i = 0; i < THREAD_COUNT; ++i) {
    free_pixel_array_2d(job_data[i]->thread_pixel_array, job_data[i]->height);
    free(job_data[i]);
  }
  free(job_data);
}

void write_output(FILE *output_file, const Image *input_image,
                  struct thread_data **job_data, Image **output_image,
                  const struct BMP_Header *BMP, const struct DIB_Header *DIB) {
  // Create output pixel array
  struct Pixel **output_pixels =
      create_pixel_array_2d(input_image->width, input_image->height);
  write_output_pixels(output_pixels, job_data);

  // Create the output image
  *output_image =
      image_create(output_pixels, input_image->width, input_image->height);

  // Write the output file
  write_output_file(output_file, *output_image, output_pixels, *BMP, *DIB);
}

void perform_filtering(const Image *input_image, struct thread_data ***job_data,
                       pthread_t tids[THREAD_COUNT],
                       const ProgramOptions *options) {
  // Initialize thread data
  if (init_thread_data(job_data, input_image, &options->rShift,
                       &options->gShift, &options->bShift) != EXIT_SUCCESS) {
    perror("Error initializing thread info.");
    exit(EXIT_FAILURE);
  }

  // Perform filtering
  for (int i = 0; i < THREAD_COUNT; ++i) {
    if (pthread_create(&tids[i], NULL, options->filter_func, (*job_data)[i]) !=
        EXIT_SUCCESS) {
      perror("Error creating thread.");
      exit(EXIT_FAILURE);
    }
  }

  // Wait for threads to finish
  for (int i = 0; i < THREAD_COUNT; ++i) {
    if (pthread_join(tids[i], NULL) != EXIT_SUCCESS) {
      perror("Error joining thread.");
      exit(EXIT_FAILURE);
    }
  }
}

void initialize_input_image(FILE *input_file, Image **input_image,
                            struct BMP_Header *BMP, struct DIB_Header *DIB) {
  struct Pixel **pixels;

  // Extract input pixels
  extract_input_image_data(input_file, &pixels, BMP, DIB);

  // Create input image
  *input_image = image_create(pixels, DIB->image_width_w, DIB->image_height_h);
}

void write_output_pixels(struct Pixel **output_pixels,
                         struct thread_data **job_data) {
  // Copy thread pixel arrays to output pixel array
  for (int i = 0; i < THREAD_COUNT; ++i) {
    for (int row = 0; row < job_data[i]->height; ++row) {
      for (int col = job_data[i]->start; col <= job_data[i]->end; ++col) {
        output_pixels[row][col] =
            job_data[i]->thread_pixel_array[row][col - job_data[i]->start];
      }
    }
  }
}

int init_thread_data(struct thread_data ***thread_data, const Image *image,
                     const int *rShift, const int *gShift, const int *bShift) {
  // Allocate memory for thread_data pointers
  if ((*thread_data = malloc(sizeof(struct thread_data *) * THREAD_COUNT)) ==
      NULL) {
    perror("Error while allocating memory for thread_info pointers.");
    return EXIT_FAILURE;
  }

  // Calculate width distribution among threads
  const int width_per_thread = image->width / THREAD_COUNT;
  const int remaining_width = image->width % THREAD_COUNT;

  // Log for debugging
  printf("Thread count: %d\n", THREAD_COUNT);
  printf("Width per thread: %d, remainder: %d\n", width_per_thread,
         remaining_width);

  for (int i = 0; i < THREAD_COUNT; ++i) {
    // Allocate individual thread_data structure
    if (((*thread_data)[i] = malloc(sizeof(struct thread_data))) == NULL) {
      perror("Error while allocating memory for thread_info struct.");
      return EXIT_FAILURE;
    }

    // Set basic thread properties
    (*thread_data)[i]->height = image->height;
    (*thread_data)[i]->og_image = image;
    (*thread_data)[i]->rShift = *rShift;
    (*thread_data)[i]->gShift = *gShift;
    (*thread_data)[i]->bShift = *bShift;

    // Calculate thread's section boundaries
    if (i == 0) {
      (*thread_data)[i]->start = 0;
    } else {
      (*thread_data)[i]->start = (*thread_data)[i - 1]->end + 1;
    }

    if (i == THREAD_COUNT - 1) {
      // Last thread gets any remaining columns
      (*thread_data)[i]->end = image->width - 1;
    } else {
      (*thread_data)[i]->end = (*thread_data)[i]->start + width_per_thread - 1;
    }

    // Calculate thread's width
    (*thread_data)[i]->width =
        (*thread_data)[i]->end - (*thread_data)[i]->start + 1;

    // Allocate memory for thread pixel array
    if (((*thread_data)[i]->thread_pixel_array = create_pixel_array_2d(
             (*thread_data)[i]->width, (*thread_data)[i]->height)) == NULL) {
      perror("Error while allocating memory for thread_info pixel array.");
      return EXIT_FAILURE;
    }

    // Log for debugging
    printf("Thread %-3d: start: %-3d, end: %-3d, width: %-3d\n", i,
           (*thread_data)[i]->start, (*thread_data)[i]->end,
           (*thread_data)[i]->width);
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
        if ((options->input_file = fopen(optarg, "rb")) == NULL) {
          perror("Input file could not be opened.");
          exit(EXIT_FAILURE);
        }
        break;
      case 'o':
        if ((options->output_file = fopen(optarg, "wb")) == NULL) {
          perror("Output file could not be opened.");
          exit(EXIT_FAILURE);
        }
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

void extract_input_image_data(FILE *input_file, struct Pixel ***input_pixels,
                              struct BMP_Header *BMP, struct DIB_Header *DIB) {
  // read headers from input file
  readBMPHeader(input_file, BMP);
  readDIBHeader(input_file, DIB);

  // allocate memory for input pixel array
  *input_pixels =
      create_pixel_array_2d(DIB->image_width_w, DIB->image_height_h);

  // read pixels from input file
  readPixelsBMP(input_file, *input_pixels, DIB->image_width_w,
                DIB->image_height_h);
  // close input file
  fclose(input_file);
}

/**
 * Write the output file with the filtered image.
 * @param output_file the output file to write to
 * @param image the image to write to the output file
 * @param new_pixels the new pixels to write to the output file
 * @param BMP the BMP header for the output image
 * @param DIB the DIB header for the output image
 */
void write_output_file(FILE *output_file, const Image *image,
                       struct Pixel **new_pixels, const struct BMP_Header BMP,
                       const struct DIB_Header DIB) {
  // write headers to output file
  writeBMPHeader(output_file, &BMP);
  writeDIBHeader(output_file, &DIB);

  // write pixels to output file
  writePixelsBMP(output_file, new_pixels, image_get_width(image),
                 image_get_height(image));

  // close output file
  fclose(output_file);
}

void display_usage(char **argv) {
  fprintf(stderr, "Usage: %s -i <input file> -o <output file> -f <filter>\n",
          argv[0]);
}