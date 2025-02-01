#include "bitmap.h"
#include "portable_endian.h"
#include <stdio.h>

void write_bitmap(char *filename, unsigned int imgwidth, unsigned int imgheight,
                  uint8_t *pixels) {
    FILE *fptr = fopen(filename, "wb");
    size_t row_padding = (4 - (imgwidth * 3) % 4) % 4;

    // file header
    uint32_t file_size =
        htole32(14 + 40 + (3 * imgwidth + row_padding) * imgheight);
    uint32_t reserved = htole32(0);
    uint32_t data_offset = htole32(54);
    fputs("BM", fptr); // signature
    fwrite(&file_size, 4, 1, fptr);
    fwrite(&reserved, 4, 1, fptr);
    fwrite(&data_offset, 4, 1, fptr);

    // info header
    uint32_t size = htole32(40);
    uint32_t width = htole32(imgwidth);
    uint32_t height = htole32(imgheight);
    uint16_t planes = htole16(1);
    uint16_t bit_per_pixel = htole16(24);
    uint32_t compression = htole32(0);
    uint32_t image_size = htole32((3 * imgwidth + row_padding) * imgheight);
    uint32_t x_per_meter = htole32(0);
    uint32_t y_per_meter = htole32(0);
    uint32_t colors_used = htole32(0);
    uint32_t important_colors = htole32(0);
    fwrite(&size, 4, 1, fptr);
    fwrite(&width, 4, 1, fptr);
    fwrite(&height, 4, 1, fptr);
    fwrite(&planes, 2, 1, fptr);
    fwrite(&bit_per_pixel, 2, 1, fptr);
    fwrite(&compression, 4, 1, fptr);
    fwrite(&image_size, 4, 1, fptr);
    fwrite(&x_per_meter, 4, 1, fptr);
    fwrite(&y_per_meter, 4, 1, fptr);
    fwrite(&colors_used, 4, 1, fptr);
    fwrite(&important_colors, 4, 1, fptr);

    // pixel data
    for (int y = imgheight - 1; y >= 0; y--) {
        for (int x = 0; x < imgwidth; x++) {
            int idx = y * imgwidth + x;

            // Write in BGR format
            fputc(pixels[3 * idx + 2], fptr);
            fputc(pixels[3 * idx + 1], fptr);
            fputc(pixels[3 * idx + 0], fptr);
        }

        // apply padding
        for (int i = 0; i < row_padding; i++)
            fputc(0, fptr);
    }

    fclose(fptr);
}
