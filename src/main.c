#include "bitmap.h"
#include <stdint.h>
#include <stdio.h>

#define WIDTH 1280
#define HEIGHT 720

struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

int main() {
    // Initialize frame and fill it with black
    uint8_t frame[WIDTH * HEIGHT][3];
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        frame[i][0] = 0;
        frame[i][1] = 0;
        frame[i][2] = 0;
    }

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        int frame_x = i % WIDTH;
        int frame_y = i / WIDTH;
        int x = frame_x - WIDTH / 2;
        int y = frame_y - HEIGHT / 2;

        struct color result = {0, 0, 0}; // TODO: trace_ray(...);
        frame[i][0] = result.r;
        frame[i][1] = result.g;
        frame[i][2] = result.b;
    }

    // Write rendered frame to an image
    write_bitmap("output.bmp", WIDTH, HEIGHT, frame);
    return 0;
}
