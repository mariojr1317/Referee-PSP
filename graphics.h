#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <pspgu.h>

typedef struct {
    int width;
    int height;
    int textureWidth;
    int textureHeight;
    unsigned int* data;
} Image;

void initGraphics();
void startFrame();
void endFrame();
Image* loadPNG(const char* filename);
void freeImage(Image* img);
void drawImage(Image* img, int x, int y, int w, int h);

#endif
