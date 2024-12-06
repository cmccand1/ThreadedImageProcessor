# Multi-Threaded Image Filter Processor

## Overview

This program is a multi-threaded image processing tool designed to apply various filters to BMP images. It uses POSIX threads to parallelize the processing of image data for efficiency. The program supports multiple filter types, including grayscale, color shift, box blur, and a "Swiss cheese" effect.

## Features

- **Multi-threaded Processing**: Divides the image into sections processed independently by multiple threads.
- **Supported Filters**:
  - Grayscale (`-f g`)
  - Color Shift (`-f s` with optional `-r`, `-g`, `-b` for red, green, and blue shifts)
  - Box Blur (`-f b`)
  - Swiss Cheese Effect (`-f c`)
- **BMP File Support**: Reads and writes uncompressed BMP image files.
- **Modular Design**: Cleanly structured code for ease of maintenance and extension.

## Usage

### Build Instructions

1. Ensure you have a C compiler (e.g., `gcc`) and `make` installed on your system.
2. Clone this repository:
   ```bash
   git clone <repository-url>
   cd <repository-directory>
   ```

## Run Instructions
The program takes the following arguments:

```bash
./image_processor -i <input_file> -o <output_file> -f <filter> [-r <red_shift>] [-g <green_shift>] [-b <blue_shift>]
```
-	`-i`: Input BMP file.
-	`-o`: Output BMP file.
-	`-f`: Filter type (b, g, s, or c).
-	`-r`, `-g`, `-b`: Optional red, green, and blue shift values for the color shift filter (`-f` s).

## Examples

Apply Grayscale Filter
```bash
./image_processor -i input.bmp -o output.bmp -f g
```
![Screenshot 2024-12-06 at 12 57 25 AM](https://github.com/user-attachments/assets/f98ebf9e-15ba-4fde-b7ed-df15db1dba56)

Apply Color Shift Filter
```bash
./image_processor -i input.bmp -o output.bmp -f s -r 255 -g 0 -b 100
```
![Screenshot 2024-12-06 at 12 59 51 AM](https://github.com/user-attachments/assets/2d4f90ab-51a5-4c9e-9789-d5c3d81f9ee7)

Apply Box Blur Filter
```bash
./image_processor -i input.bmp -o output.bmp -f b
```
![Screenshot 2024-12-06 at 12 55 01 AM](https://github.com/user-attachments/assets/88b12599-516b-4e7f-ac2a-8aef69f1afe1)
Apply Swiss Cheese Filter
```bash
./image_processor -i input.bmp -o output.bmp -f c
```

## How It Works

1. **Command-Line Parsing**:
   - The program reads user-provided arguments using `getopt`.
   - File paths, filter type, and optional RGB shift values are stored in a `ProgramOptions` structure.

2. **Image Reading**:
   - BMP file headers (`BMP_Header` and `DIB_Header`) are parsed to retrieve image metadata.
   - Pixel data is loaded into a dynamically allocated 2D array of `struct Pixel`.

3. **Multi-Threaded Processing**:
   - The image is divided into vertical sections, each assigned to a thread.
   - Threads process their respective sections using the selected filter.

4. **Filter Application**:
   - **Grayscale**: Converts each pixel to grayscale by calculating a weighted average of the RGB components.
   - **Color Shift**: Adjusts RGB values based on user-specified shifts for red, green, and blue channels.
   - **Box Blur**: Averages the RGB values of neighboring pixels within a kernel window to produce a blur effect.
   - **Swiss Cheese**: Randomly applies black circular holes across the image to simulate a “cheese-like” appearance.

5. **Image Writing**:
   - The program combines the results from all threads.
   - The processed pixel data is written back to a new BMP file, preserving the original file’s metadata.
