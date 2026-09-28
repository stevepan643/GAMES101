#include "window.h"

#include <SDL3/SDL.h>

struct window_t
{
    SDL_Window *sdl_window;
    SDL_Renderer *sdl_renderer;
    SDL_Texture *sdl_texture;
    SDL_Surface *sdl_surface;
    framebuffer_t framebuffer;

    framebuffer_format_t format;
    uint32_t width;
    uint32_t height;
    char *title;
    window_flag_t flags;
};

static void build_framebuffer(window_t *window)
{
    if (window->sdl_surface)
    {
        SDL_DestroySurface(window->sdl_surface);
        window->sdl_surface = NULL;
    }

    SDL_PixelFormat format;
    if (window->format == PIXEL_FORMAT_RGBA8888)
    {
        format = SDL_PIXELFORMAT_ABGR8888;
    }
    else
    {
        format = SDL_PIXELFORMAT_BGR24;
    }

    window->sdl_surface = SDL_CreateSurface((int)window->width, (int)window->height, format);

    if (!window->sdl_surface)
    {
        printf("SDL_CreateSurface failed: %s\n", SDL_GetError());
        window->framebuffer = NULL;
        return;
    }

    window->framebuffer = (framebuffer_t)window->sdl_surface->pixels;
}

static void rebuild_surface_and_texture(window_t *window)
{
    build_framebuffer(window);

    if (window->sdl_texture)
    {
        SDL_DestroyTexture(window->sdl_texture);
    }
    window->sdl_texture =
        SDL_CreateTexture(window->sdl_renderer,
                          (window->format == PIXEL_FORMAT_RGBA8888) ? SDL_PIXELFORMAT_RGBA8888
                                                                    : SDL_PIXELFORMAT_RGB24,
                          SDL_TEXTUREACCESS_STREAMING, (int)window->width, (int)window->height);
    if (!window->sdl_texture)
    {
        printf("SDL_CreateTexture failed: %s\n", SDL_GetError());
    }
}

static void apply_size(window_t *window, uint32_t width, uint32_t height)
{
    window->width = width;
    window->height = height;
    rebuild_surface_and_texture(window);
}

window_t *window_create(framebuffer_format_t format)
{
    window_t *window = calloc(1, sizeof(window_t));
    if (!window)
        return NULL;

    window->format = format;
    window->width = 100;
    window->height = 100;
    window->title = strdup("Window");
    window->flags = WINDOW_FLAG_NONE;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        free(window);
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return NULL;
    }

    window->sdl_window =
        SDL_CreateWindow(window->title, (int)window->width, (int)window->height, SDL_WINDOW_HIDDEN);

    if (!window->sdl_window)
    {
        SDL_Quit();
        free(window);
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return NULL;
    }

    window->sdl_renderer = SDL_CreateRenderer(window->sdl_window, NULL);
    if (!window->sdl_renderer)
    {
        SDL_DestroyWindow(window->sdl_window);
        SDL_Quit();
        free(window);
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return NULL;
    }

    window->sdl_texture = SDL_CreateTexture(
        window->sdl_renderer,
        (format == PIXEL_FORMAT_RGBA8888) ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_XRGB8888,
        SDL_TEXTUREACCESS_STREAMING, (int)window->width, (int)window->height);
    if (!window->sdl_texture)
    {
        SDL_DestroyRenderer(window->sdl_renderer);
        SDL_DestroyWindow(window->sdl_window);
        SDL_Quit();
        free(window);
        printf("SDL_CreateTexture failed: %s\n", SDL_GetError());
        return NULL;
    }

    build_framebuffer(window);

    return window;
}

void window_set_size(window_t *window, uint32_t width, uint32_t height)
{
    SDL_SetWindowSize(window->sdl_window, (int)width, (int)height);
    apply_size(window, width, height);
}
void window_set_title(window_t *window, const char *title)
{
    free(window->title);
    window->title = strdup(title);
    if (!window->title)
    {
        printf("Failed to allocate memory for window title\n");
        return;
    }

    SDL_SetWindowTitle(window->sdl_window, window->title);
}
void window_set_flags(window_t *window, window_flag_t flags)
{
    window->flags = flags;

    if (flags & WINDOW_FLAG_FULLSCREEN)
        SDL_SetWindowFullscreen(window->sdl_window, SDL_WINDOW_FULLSCREEN);
    else
        SDL_SetWindowFullscreen(window->sdl_window, 0);

    if (flags & WINDOW_FLAG_RESIZABLE && flags & WINDOW_FLAG_UNRESIZABLE)
    {
        printf("Warning: Both WINDOW_FLAG_RESIZABLE and WINDOW_FLAG_UNRESIZABLE are set. "
               "Unresizable will take precedence.\n");
        return;
    }
    if (flags & WINDOW_FLAG_RESIZABLE)
        SDL_SetWindowResizable(window->sdl_window, true);
    else if (flags & WINDOW_FLAG_UNRESIZABLE)
        SDL_SetWindowResizable(window->sdl_window, false);
}
void window_set_visible(window_t *window, bool visible)
{
    if (visible)
        SDL_ShowWindow(window->sdl_window);
    else
        SDL_HideWindow(window->sdl_window);
}

uint32_t window_get_width(window_t *window)
{
    return window->width;
}
uint32_t window_get_height(window_t *window)
{
    return window->height;
}
const char *window_get_title(window_t *window)
{
    return window->title;
}

framebuffer_t window_get_framebuffer(window_t *window)
{
    return window->framebuffer;
}
uint32_t window_get_framebuffer_bytes_per_pixel(window_t *window)
{
    return (window->format == PIXEL_FORMAT_RGBA8888) ? 4 : 3;
}
uint32_t window_get_framebuffer_stride(window_t *window)
{
    if (!window->sdl_surface)
        return 0;
    return (uint32_t)window->sdl_surface->pitch;
}
framebuffer_format_t window_get_framebuffer_format(window_t *window)
{
    return window->format;
}

window_event_t window_poll_event(window_t *window)
{
    SDL_Event event;
    if (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                return WINDOW_EVENT_CLOSE;
            case SDL_EVENT_WINDOW_RESIZED:
                apply_size(window, (uint32_t)event.window.data1, (uint32_t)event.window.data2);
                return WINDOW_EVENT_RESIZE;
            default:
                return WINDOW_EVENT_NONE;
        }
    }
    return WINDOW_EVENT_NONE;
}
void window_swap_framebuffer(window_t *window)
{
    SDL_UpdateTexture(window->sdl_texture, NULL, window->sdl_surface->pixels,
                      window->sdl_surface->pitch);

    SDL_RenderTexture(window->sdl_renderer, window->sdl_texture, NULL, NULL);

    SDL_RenderPresent(window->sdl_renderer);
}

void window_destroy(window_t *window)
{
    if (!window)
        return;

    if (window->sdl_surface)
        SDL_DestroySurface(window->sdl_surface);
    if (window->sdl_texture)
        SDL_DestroyTexture(window->sdl_texture);
    if (window->sdl_renderer)
        SDL_DestroyRenderer(window->sdl_renderer);
    if (window->sdl_window)
        SDL_DestroyWindow(window->sdl_window);

    SDL_Quit();

    free(window->title);
    free(window);
}

uint64_t window_get_time(void)
{
    return SDL_GetTicks();
}
