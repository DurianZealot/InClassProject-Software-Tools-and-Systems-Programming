#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"


/*
 * Read in the location of the pixel array, the image width, and the image 
 * height in the given bitmap file.
 */
void read_bitmap_metadata(FILE *image, int *pixel_array_offset, int *width, int *height) {
    // Pointer to the location of the pixel array
    fseek(image, 10, SEEK_SET);
    // Read 4 bytes at the starting location of the pixel array, store the data into pixel_array_offset
    fread(pixel_array_offset, 4, 1, image);

    fseek(image, 18, SEEK_SET);
    fread(width, 4, 1, image);

    fseek(image, 22, SEEK_SET);
    fread(height, 4, 1, image);
}

/*
 * Read in pixel array by following these instructions:
 *
 * 1. First, allocate space for m `struct pixel *` values, where m is the 
 *    height of the image.  Each pointer will eventually point to one row of 
 *    pixel data.
 * 2. For each pointer you just allocated, initialize it to point to 
 *    heap-allocated space for an entire row of pixel data.
 * 3. Use the given file and pixel_array_offset to initialize the actual 
 *    struct pixel values. Assume that `sizeof(struct pixel) == 3`, which is 
 *    consistent with the bitmap file format.
 *    NOTE: We've tested this assumption on the Teaching Lab machines, but 
 *    if you're trying to work on your own computer, we strongly recommend 
 *    checking this assumption!
 * 4. Return the address of the first `struct pixel *` you initialized.
 */
struct pixel **read_pixel_array(FILE *image, int pixel_array_offset, int width, int height) {
    struct pixel **m_pixels = malloc(sizeof(struct pixel *)*height);
    struct pixel *first_pixel;

    // If the allocation fails
    if (m_pixels == NULL){
        return NULL;
    }
    
    fseek(image, pixel_array_offset, SEEK_SET);

    for(int i = 0; i < height; i++){
        first_pixel = malloc(sizeof(struct pixel) * width);

        if (first_pixel == NULL){
            int prev_row = i - 1;
            while(prev_row >= 0){
                free(m_pixels[prev_row]);
                prev_row -= 1;
            }
        }

        for(int j = 0; j < width; j++){
            fread(&(first_pixel[j].blue), 1, 1, image);
            fread(&(first_pixel[j].green), 1, 1, image);
            fread(&(first_pixel[j].red), 1, 1, image);
        }

        m_pixels[i] = first_pixel;
    }
    
    return m_pixels;
}


/*
 * Print the blue, green, and red colour values of a pixel.
 * You don't need to change this function.
 */
void print_pixel(struct pixel p) {
    printf("(%u, %u, %u)\n", p.blue, p.green, p.red);
}
