#ifndef RASTERIZER_FRAMEBUFFER_H
#define RASTERIZER_FRAMEBUFFER_H

#include "color.h"
#include "common.h"

typedef uint8_t *framebuffer_t;

static inline void framebuffer_clear(framebuffer_t fb, uint32_t stride, uint32_t bpp, uint32_t w,
                                     uint32_t h, color_t c)
{
    size_t row_bytes = (size_t)w * bpp;

    uint8_t *row0 = fb;
    for (uint32_t x = 0; x < w; ++x)
    {
        uint8_t *px = row0 + (size_t)x * bpp;
        px[0] = c.r;
        px[1] = c.g;
        px[2] = c.b;
        if (bpp == 4)
            px[3] = c.a;
    }

    for (uint32_t y = 1; y < h; ++y)
    {
        memcpy(fb + (size_t)y * stride, fb + (size_t)(y - 1) * stride, row_bytes);
    }
}
static inline void framebuffer_set_color(framebuffer_t fb, uint32_t stride, uint32_t bpp,
                                         uint32_t x, uint32_t y, color_t c)
{
    uint8_t *pixel = fb + (size_t)y * stride + (size_t)x * bpp;
    pixel[0] = c.r;
    pixel[1] = c.g;
    pixel[2] = c.b;
    if (bpp == 4)
        pixel[3] = c.a;
}

#endif /* RASTERIZER_FRAMEBUFFER_H */