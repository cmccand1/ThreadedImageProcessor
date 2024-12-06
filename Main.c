#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "headers/BMPHandler.h"
#include "headers/Image.h"


typedef void * (*filter_method)(void *);

void display_usage(char **argv);

int init_thread_data(struct thread_data ***thread_data, const Image *image);

void process_user_args(int argc, char **argv, FILE **input_file, FILE **output_file, filter_method*filter_func);

void extract_input_image_data(FILE *input_file, struct Pixel ***input_pixels, struct BMP_Header *BMP, struct DIB_Header *DIB);

void write_output_file(FILE *output_file, const Image *image, struct Pixel **new_pixels, struct BMP_Header BMP, struct DIB_Header DIB);

void do_filter(const Image *input_image, filter_method filter_func, struct thread_data ***job_data, pthread_t tids[THREAD_COUNT]);

void write_output_pixels(struct Pixel **output_pixels, struct thread_data **job_data);

int main(int argc, char *argv[]) {
    // Image headers (input and output)
    struct BMP_Header BMP;
    struct DIB_Header DIB;

    // Input data
    FILE *input_file = NULL;
    struct Pixel **input_pixels;
    Image *input_image;

    // Output data
    FILE *output_file = NULL;
    struct Pixel **output_pixels;
    Image *output_image;

    // Thread data
    filter_method filter_func;
    struct thread_data **job_data;
    pthread_t tids[THREAD_COUNT];


    // init based on user input, terminating if invalid
    process_user_args(argc, argv, &input_file, &output_file, &filter_func);

    // get the data from the input file
    extract_input_image_data(input_file, &input_pixels, &BMP, &DIB);

    // create the image from the pixel array
    input_image = image_create(input_pixels, DIB.image_width_w, DIB.image_height_h);

    // perform the filtering operation. the results will be in the job_data structs
    do_filter(input_image, filter_func, &job_data, tids);

    // Stitch together the pixel array from each thread
    output_pixels = create_pixel_array_2d(DIB.image_width_w, DIB.image_height_h);
    write_output_pixels(output_pixels, job_data);

    // write the output file and clean up
    output_image = image_create(output_pixels, DIB.image_width_w, DIB.image_height_h);
    write_output_file(output_file, output_image, output_pixels, BMP, DIB);

    // clean up
    for (int i = 0; i < THREAD_COUNT; ++i) {
        free_pixel_array_2d(job_data[i]->thread_pixel_array,
                            job_data[i]->height);
        // free the thread_info struct
        free(job_data[i]);
        job_data[i] = NULL;
    }
    free(job_data);
    job_data = NULL;

    image_destroy(&input_image);
    image_destroy(&output_image);
    return 0;
}

void do_filter(const Image *input_image, filter_method filter_func, struct thread_data ***job_data, pthread_t tids[THREAD_COUNT]) {
    if (init_thread_data(job_data, input_image) != EXIT_SUCCESS) {
        perror("Error initializing thread info.");
        exit(EXIT_FAILURE);
    }

    // create the threads, which call filter with error handling
    for (int i = 0; i < THREAD_COUNT; ++i) {
        if (pthread_create(&tids[i], NULL, filter_func, (*job_data)[i]) != EXIT_SUCCESS) {
            perror("Error creating thread.");
            exit(EXIT_FAILURE);
        }
    }

    // wait for all threads to finish, with error handling
    for (int i = 0; i < THREAD_COUNT; ++i) {
        if (pthread_join(tids[i], NULL) != EXIT_SUCCESS) {
            perror("Error joining thread.");
            exit(EXIT_FAILURE);
        }
    }
}

void write_output_pixels(struct Pixel **output_pixels, struct thread_data **job_data) {
    for (int i = 0; i < THREAD_COUNT; ++i) {
        for (int row = 0; row < job_data[i]->height; ++row) {
            for (int col = job_data[i]->start; col <= job_data[i]->end; ++col) {
                output_pixels[row][col] = job_data[i]->thread_pixel_array[row][col - job_data[i]->start];
            }
        }
    }
}

int init_thread_data(struct thread_data ***thread_data, const Image *image) {
    // Allocate array of thread_data pointers
    if ((*thread_data = malloc(sizeof(struct thread_data *) * THREAD_COUNT)) == NULL) {
        perror("Error while allocating memory for thread_info pointers.");
        return EXIT_FAILURE;
    }

    // Calculate width distribution among threads
    const int width_per_thread = image->width / THREAD_COUNT;
    const int remaining_width = image->width % THREAD_COUNT;

    printf("Thread count: %d\n", THREAD_COUNT);
    printf("Width per thread: %d, remainder: %d\n", width_per_thread, remaining_width);

    for (int i = 0; i < THREAD_COUNT; ++i) {
        // Allocate individual thread_data structure
        if (((*thread_data)[i] = malloc(sizeof(struct thread_data))) == NULL) {
            perror("Error while allocating memory for thread_info struct.");
            return EXIT_FAILURE;
        }

        // Set basic thread properties
        (*thread_data)[i]->height = image->height;
        (*thread_data)[i]->og_image = image;

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

        // Calculate actual width for this thread's section
        (*thread_data)[i]->width = (*thread_data)[i]->end - (*thread_data)[i]->start + 1;

        // Allocate thread's pixel array with correct dimensions
        if (((*thread_data)[i]->thread_pixel_array = create_pixel_array_2d(
                 (*thread_data)[i]->width, (*thread_data)[i]->height)) == NULL) {
            perror("Error while allocating memory for thread_info pixel array.");
            return EXIT_FAILURE;
        }

        printf("Thread %d: start: %d, end: %d, width: %d\n",
               i, (*thread_data)[i]->start, (*thread_data)[i]->end, (*thread_data)[i]->width);
    }
    return EXIT_SUCCESS;
}

void process_user_args(int argc, char **argv, FILE **input_file, FILE **output_file, filter_method*filter_func) {
    int opt;
    optarg = NULL;
    while ((opt = getopt(argc, argv, "i:o:f:")) != -1) {
        if (argc != 6 + 1) {
            fprintf(stderr, "Expected 6 arguments, got %d instead.\n", argc - 1);
            display_usage(argv);
            exit(EXIT_FAILURE);
        }
        switch (opt) {
            case 'i':
                // open the input file to read binary
                if ((*input_file = fopen(optarg, "rb")) == NULL) {
                    perror("Input file could not be opened.");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'o':
                // create an output file to write binary
                if ((*output_file = fopen(optarg, "wb")) == NULL) {
                    perror("Output file could not be opened.");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'f':
                // set which filtering procedure to use
                switch (optarg[0]) {
                    case 'b':
                        *filter_func = image_apply_t_boxblur;
                        break;
                    case 'c':
                        *filter_func = image_apply_t_cheese;
                        break;
                    default:
                        fprintf(stderr, "Invalid filter type: %s\n", optarg);
                        display_usage(argv);
                        exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Invalid option: %c\n", opt);
                display_usage(argv);
                exit(EXIT_FAILURE);
        }
    }
}

void extract_input_image_data(FILE *input_file, struct Pixel ***input_pixels, struct BMP_Header *BMP, struct DIB_Header *DIB) {
    // read BMP header and DIB header from input file
    readBMPHeader(input_file, BMP);
    readDIBHeader(input_file, DIB);

    *input_pixels = create_pixel_array_2d(DIB->image_width_w, DIB->image_height_h);

    // readPixels into pixel array
    readPixelsBMP(input_file, *input_pixels, DIB->image_width_w, DIB->image_height_h);
    fclose(input_file);
}

void write_output_file(FILE *output_file, const Image *image, struct Pixel **new_pixels, const struct BMP_Header BMP,
                       const struct DIB_Header DIB) {
    // write new headers and pixels to output file
    writeBMPHeader(output_file, &BMP);
    writeDIBHeader(output_file, &DIB);
    writePixelsBMP(output_file, new_pixels, image_get_width(image),
                   image_get_height(image));

    fclose(output_file);
}

void display_usage(char **argv) {
    fprintf(stderr,
            "Usage: %s -i <input file> -o <output file> -f <filter>\n",
            argv[0]);
}