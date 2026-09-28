#ifndef RASTERIZER_WINDOW_H
#define RASTERIZER_WINDOW_H

#include "color.h"
#include "common.h"
#include "framebuffer.h"

typedef struct window_t window_t;

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

window_t *window_create(framebuffer_format_t format);

void window_set_size(window_t *window, uint32_t width, uint32_t height);
void window_set_title(window_t *window, const char *title);
void window_set_flags(window_t *window, window_flag_t flags);
void window_set_visible(window_t *window, bool visible);

uint32_t window_get_width(window_t *window);
uint32_t window_get_height(window_t *window);
const char *window_get_title(window_t *window);

framebuffer_t window_get_framebuffer(window_t *window);
uint32_t window_get_framebuffer_bytes_per_pixel(window_t *window);
uint32_t window_get_framebuffer_stride(window_t *window);
framebuffer_format_t window_get_framebuffer_format(window_t *window);

window_event_t window_poll_event(window_t *window);
void window_swap_framebuffer(window_t *window);

void window_destroy(window_t *window);

uint64_t window_get_time(void);

#endif /* RASTERIZER_WINDOW_H */