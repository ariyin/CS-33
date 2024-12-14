/* 
 *  Name: Jenna Wang
 *  UID: 806023162
 */

#include <stdlib.h>
#include <omp.h>
#include <stdio.h>

#include "utils.h"
#include "parallel.h"

/*
 *  PHASE 1: compute the mean pixel value
 *  This code is buggy! Find the bug and speed it up.
 *  Points: 20
 *  1024x1024 baseline: 3
 *  2048x2048 baseline: 12
 *  4096x4096 baseline: 28
 */
void mean_pixel_parallel(const uint8_t img[][NUM_CHANNELS], int num_rows, int num_cols, double mean[NUM_CHANNELS]) {
    int pixel;
    int num_pixels = num_rows * num_cols;
    double red = 0.0, green = 0.0, blue = 0.0;

    // to get red, green, blue, read rgb from every pixel
    #pragma omp parallel for reduction(+:red, green, blue)
    for(pixel = 0; pixel < num_pixels; pixel++)
    {
        red += img[pixel][0];
        green += img[pixel][1];
        blue += img[pixel][2];
    }

    // divide sum of red, green, blue by number of pixels
    mean[0] = red / num_pixels;
    mean[1] = green / num_pixels;
    mean[2] = blue / num_pixels;
}

/*
 *  PHASE 2: convert image to grayscale and record the max grayscale value along with the number of times it appears
 *  This code is NOT buggy, just sequential. Speed it up.
 *  Points: 40
 *  1024x1024 baseline: 2
 *  2048x2048 baseline: 7
 *  4096x4096 baseline: 26
 */
void grayscale_parallel(const uint8_t img[][NUM_CHANNELS], int num_rows, int num_cols, uint32_t grayscale_img[][NUM_CHANNELS], uint8_t *max_gray, uint32_t *max_count) {
    // grey = add red, green, blue then / 3
    int pixel;
    *max_gray = 0, *max_count = 0;
    int num_pixels = num_rows * num_cols;
    int count[256] = {0};

    #pragma omp parallel for reduction(+:count) private(pixel)
    for(pixel = 0; pixel < num_pixels; pixel++)
    {
        int grey = (int)((img[pixel][0] + img[pixel][1] + img[pixel][2]) / 3);

        grayscale_img[pixel][0] = grey;
        grayscale_img[pixel][1] = grey;
        grayscale_img[pixel][2] = grey;

        count[grey] += 3;
    }

    for (int i = 255; i >= 0; i--) 
    {
        // printf("count for %i: %i\n", i, count[i]);
        if (count[i] > 0) {
            *max_count = count[i];
            *max_gray = i;
            break;
        }
    }
}

/*
 *  PHASE 3: perform convolution on image
 *  This code is NOT buggy, just sequential. Speed it up.
 *  Points: 40
 *
 *  3x3 kernel
 *  1024x1024 baseline: 6
 *  2048x2048 baseline: 14
 *  4096x4096 baseline: 21
 *
 *  5x5 kernel
 *  1024x1024 baseline: 11
 *  2048x2048 baseline: 17
 *  4096x4096 baseline: 23
 */
void convolution_parallel(const uint8_t padded_img[][NUM_CHANNELS], int num_rows, int num_cols, const uint32_t kernel[], int kernel_size, uint32_t convolved_img[][NUM_CHANNELS]) {
    int row, col, kernel_row, kernel_col, kernel_norm = 0, pixel, temp1, temp2, temp3, temp4, temp5, acc0 =0, acc1, acc2;
    int conv_dim = num_rows - kernel_size + 1;
    int kernel_block = kernel_size * kernel_size;

    #pragma omp parallel for reduction(+:kernel_norm)
    for (int i = 0; i < kernel_block; i++) {
        kernel_norm += kernel[i];
    }

    #pragma omp parallel for private(col, row, kernel_col, kernel_row, temp1, temp2, temp3, temp4, temp5, acc0, acc1, acc2)
    for (row = 0; row < conv_dim; row++) {
        temp5 = row * conv_dim;
        for (col = 0; col < conv_dim; col++) {
            pixel = temp5 + col;
            acc0 = 0, acc1 = 0, acc2 = 0;
            for (kernel_row = 0; kernel_row < kernel_size; kernel_row++) {
                temp1 = (row + kernel_row) * num_cols + col;
                temp2 = kernel_row * kernel_size;
                for (kernel_col = 0; kernel_col < kernel_size; kernel_col++) {
                    temp3 = temp1 + kernel_col;
                    temp4 = kernel[temp2 + kernel_col];
                    acc0 += padded_img[temp3][0] * temp4;
                    acc1 += padded_img[temp3][1] * temp4;
                    acc2 += padded_img[temp3][2] * temp4;
                }
            }
            convolved_img[pixel][0] = acc0 / kernel_norm;
            convolved_img[pixel][1] = acc1 / kernel_norm;
            convolved_img[pixel][2] = acc2 / kernel_norm;
        }
    }
}