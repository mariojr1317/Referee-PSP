#include "graphics.h"
#include <pspkernel.h>
#include <display.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

static char __attribute__((aligned(16))) list[262144];

typedef struct {
    float u, v;
    unsigned int color;
    float x, y, z;
} Vertex;

static int getNextPowerOf2(int n) {
    int p = 1;
    while (p < n) p <<= 1;
    return p;
}

void initGraphics() {
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x00088000, 512);
    sceGuDepthBuffer((void*)0x00110000, 512);

    sceGuOffset(2048 - (480 / 2), 2048 - (272 / 2));
    sceGuViewport(2048, 2048, 480, 272);
    sceGuDepthRange(65535, 0);

    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void startFrame() {
    sceGuStart(GU_DIRECT, list);
    sceGuClearColor(0xFF111111);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
}

void endFrame() {
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

Image* loadPNG(const char* filename) {
    png_structp png_ptr;
    png_infop info_ptr;
    FILE* fp = fopen(filename, "rb");
    if (!fp) return NULL;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) { fclose(fp); return NULL; }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) { png_destroy_read_struct(&png_ptr, NULL, NULL); fclose(fp); return NULL; }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    int width = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    if (bit_depth == 16) png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY) png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);

    png_read_update_info(png_ptr, info_ptr);

    int texWidth = getNextPowerOf2(width);
    int texHeight = getNextPowerOf2(height);

    Image* img = (Image*)malloc(sizeof(Image));
    img->width = width;
    img->height = height;
    img->textureWidth = texWidth;
    img->textureHeight = texHeight;
    img->data = (unsigned int*)memalign(16, texWidth * texHeight * sizeof(unsigned int));

    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*)(img->data + y * texWidth);
    }

    png_read_image(png_ptr, row_pointers);
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    sceKernelDcacheWritebackInvalidateAll();
    return img;
}

void freeImage(Image* img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
}

void drawImage(Image* img, int x, int y, int w, int h) {
    if (!img || !img->data) return;

    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexImage(0, img->textureWidth, img->textureHeight, img->textureWidth, img->data);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);

    Vertex* vertices = (Vertex*)sceGuGetMemory(2 * sizeof(Vertex));

    float u2 = (float)img->width / (float)img->textureWidth;
    float v2 = (float)img->height / (float)img->textureHeight;

    vertices[0].u = 0.0f; vertices[0].v = 0.0f;
    vertices[0].color = 0xFFFFFFFF;
    vertices[0].x = (float)x; vertices[0].y = (float)y; vertices[0].z = 0.0f;

    vertices[1].u = u2; vertices[1].v = v2;
    vertices[1].color = 0xFFFFFFFF;
    vertices[1].x = (float)(x + w); vertices[1].y = (float)(y + h); vertices[1].z = 0.0f;

    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, NULL, vertices);
}
