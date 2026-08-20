#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <setjmp.h>
#include <png.h>

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>

#include "graphics.h"

static char __attribute__((aligned(16))) list[262144];

struct Vertex {
    float u, v;
    unsigned int color;
    float x, y, z;
};

static int getNextPowerOf2(int n) {
    int p = 1;
    while (p < n) p <<= 1;
    return p;
}

static FILE* abrirArchivo(const char* filename) {
    if (!filename) return NULL;
    FILE* fp = fopen(filename, "rb");
    if (fp) return fp;

    char buf[512];
    snprintf(buf, sizeof(buf), "./%s", filename);
    fp = fopen(buf, "rb");
    if (fp) return fp;

    snprintf(buf, sizeof(buf), "ms0:/PSP/GAME/ArbitroPSP/%s", filename);
    fp = fopen(buf, "rb");
    if (fp) return fp;

    if (strncmp(filename, "assets/", 7) == 0) {
        snprintf(buf, sizeof(buf), "%s", filename + 7);
        fp = fopen(buf, "rb");
        if (fp) return fp;
    }

    return NULL;
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
    sceGuEnable(GU_SCISSOR_TEST);
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
    sceGuClearColor(0xFF1E5B22); // Verde Césped de fondo por defecto
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
}

void endFrame() {
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

Image* loadPNG(const char* filename) {
    FILE* fp = abrirArchivo(filename);
    if (!fp) return NULL;

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) { fclose(fp); return NULL; }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height(png_ptr, info_ptr);

    png_set_expand(png_ptr);
    png_set_strip_16(png_ptr);
    png_set_gray_to_rgb(png_ptr);
    png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);

    png_read_update_info(png_ptr, info_ptr);

    int texWidth = getNextPowerOf2(width);
    int texHeight = getNextPowerOf2(height);

    Image* img = (Image*)malloc(sizeof(Image));
    if (!img) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    img->width = width;
    img->height = height;
    img->textureWidth = texWidth;
    img->textureHeight = texHeight;
    img->data = (unsigned int*)memalign(16, texWidth * texHeight * sizeof(unsigned int));

    if (!img->data) {
        free(img);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for (png_uint_32 y = 0; y < height; y++) {
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

    sceGuEnable(GU_TEXTURE_2D);
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

void drawRect(int x, int y, int w, int h, unsigned int color) {
    sceGuDisable(GU_TEXTURE_2D);
    Vertex* vertices = (Vertex*)sceGuGetMemory(2 * sizeof(Vertex));

    vertices[0].u = 0.0f; vertices[0].v = 0.0f;
    vertices[0].color = color;
    vertices[0].x = (float)x; vertices[0].y = (float)y; vertices[0].z = 0.0f;

    vertices[1].u = 0.0f; vertices[1].v = 0.0f;
    vertices[1].color = color;
    vertices[1].x = (float)(x + w); vertices[1].y = (float)(y + h); vertices[1].z = 0.0f;

    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, NULL, vertices);
    sceGuEnable(GU_TEXTURE_2D);
}
