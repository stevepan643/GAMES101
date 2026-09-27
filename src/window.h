#ifndef RASTERIZATION_WINDOW_H
#define RASTERIZATION_WINDOW_H

#include "common.h"
#include "color.h"

typedef struct window_t window_t;
typedef uint8_t* framebuffer_t;

typedef enum
{
    WINDOW_FLAG_NONE = 0,
    WINDOW_FLAG_FULLSCREEN = BIT(0),
    WINDOW_FLAG_RESIZABLE = BIT(1),
    WINDOW_FLAG_UNRESIZABLE = BIT(2),
} window_flag_t;

typedef enum
{
    WINDOW_EVENT_NONE,
    WINDOW_EVENT_CLOSE,
    WINDOW_EVENT_RESIZE,
} window_event_t;

typedef enum
{
    PIXEL_FORMAT_RGBA8888,
    PIXEL_FORMAT_RGB888,
} framebuffer_format_t; 

window_t   *window_create(framebuffer_format_t format);

void        window_set_size(window_t *window, uint32_t width, uint32_t height);
void        window_set_title(window_t *window, const char *title);
void        window_set_flags(window_t *window, window_flag_t flags);
void        window_set_visible(window_t *window, bool visible);

uint32_t    window_get_width(window_t *window);
uint32_t    window_get_height(window_t *window);
const char *window_get_title(window_t *window);

framebuffer_t   window_get_framebuffer(window_t *window);
uint32_t        window_get_framebuffer_bytes_per_pixel(window_t *window);
uint32_t        window_get_framebuffer_stride(window_t *window);
framebuffer_format_t
                window_get_framebuffer_format(window_t *window);

window_event_t  window_poll_event(window_t *window);
void            window_swap_framebuffer(window_t *window);

void            window_destroy(window_t *window);

static inline void framebuffer_clear(framebuffer_t fb, uint32_t stride, uint32_t bpp,
                            uint32_t w, uint32_t h, color_t c)
{
    size_t row_bytes = (size_t)w * bpp;

    uint8_t *row0 = fb;
    for (uint32_t x = 0; x < w; ++x) {
        uint8_t *px = row0 + (size_t)x * bpp;
        px[0] = c.r; px[1] = c.g; px[2] = c.b;
        if (bpp == 4) px[3] = c.a;
    }

    for (uint32_t y = 1; y < h; ++y) {
        memcpy(fb + (size_t)y * stride, fb + (size_t)(y - 1) * stride, row_bytes);
    }
}
static inline void framebuffer_set_color(framebuffer_t fb, uint32_t stride,
                                         uint32_t bpp, uint32_t x, uint32_t y,
                                         color_t c)
{
    uint8_t *pixel = fb + (size_t)y * stride + (size_t)x * bpp;
    pixel[0] = c.r;
    pixel[1] = c.g;
    pixel[2] = c.b;
    if (bpp == 4) pixel[3] = c.a;
}

#endif  /* RASTERIZATION_WINDOW_H */