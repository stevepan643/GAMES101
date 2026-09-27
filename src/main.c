#include "compute.h"
#include "triangle.h"
#include "window.h"

#include <stdio.h>

float *depth = NULL;
size_t depth_size = 0;
size_t depth_capacity = 0;

color_t *color_buffer = NULL;
size_t color_size = 0;
size_t color_capacity = 0;

uint32_t msaa_level = 4;

static inline mat4f_t get_viewing(vec3f_t eyepos)
{
    mat4f_t viewing = {{
        { 1, 0, 0, -eyepos.x },
        { 0, 1, 0, -eyepos.y },
        { 0, 0, 1, -eyepos.z },
        { 0, 0, 0, 1 }
    }};
    return viewing;
}

static inline mat4f_t get_perspective(float fov_y, float aspect, float n, float f)
{
    float t = n * tanf(fov_y * 0.5f);
    float r = t * aspect;

    mat4f_t projection = {{{n / r, 0, 0, 0},
                           {0, n / t, 0, 0},
                           {0, 0, -(f + n) / (f - n), -2.0f * f * n / (f - n)},
                           {0, 0, -1.0f, 0}}};

    return projection;
}

static inline vec4f_t perspective_divide(vec4f_t v)
{
    vec4f_t r = {v.x / v.w, v.y / v.w, v.z / v.w, v.w};
    return r;
}

static inline vec4f_t ndc_to_screen(vec4f_t ndc, uint32_t w, uint32_t h)
{
    vec4f_t s = {(ndc.x + 1.0f) * 0.5f * (float)w, (1.0f - ndc.y) * 0.5f * (float)h, ndc.z, ndc.w};
    return s;
}

static inline void barycentric(triangle_t *tri, float x, float y, float *a, float *b, float *c)
{
    float x1 = tri->v1.x, y1 = tri->v1.y;
    float x2 = tri->v2.x, y2 = tri->v2.y;
    float x3 = tri->v3.x, y3 = tri->v3.y;

    float area = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);

    if (area == 0.0f)
    {
        *a = *b = *c = 0.0f;
        return;
    }
    float inv = 1.0f / area;

    *a = ((x2 - x) * (y3 - y) - (y2 - y) * (x3 - x)) * inv;
    *b = ((x3 - x) * (y1 - y) - (y3 - y) * (x1 - x)) * inv;
    *c = 1.0f - *a - *b;
}

static const float sample_offsets_4x[4][2] = {
    {0.375f, 0.125f},
    {0.875f, 0.375f},
    {0.125f, 0.625f},
    {0.625f, 0.875f},
};

static void rasterization(triangle_t *tri, uint32_t w, uint32_t h)
{
    float min_xf = fminf(fminf(tri->v1.x, tri->v2.x), tri->v3.x);
    float max_xf = fmaxf(fmaxf(tri->v1.x, tri->v2.x), tri->v3.x);
    float min_yf = fminf(fminf(tri->v1.y, tri->v2.y), tri->v3.y);
    float max_yf = fmaxf(fmaxf(tri->v1.y, tri->v2.y), tri->v3.y);

    int min_x = (int)floorf(min_xf);
    int max_x = (int)ceilf(max_xf);
    int min_y = (int)floorf(min_yf);
    int max_y = (int)ceilf(max_yf);

    if (min_x < 0)
        min_x = 0;
    if (min_y < 0)
        min_y = 0;
    if (max_x >= (int)w)
        max_x = (int)w - 1;
    if (max_y >= (int)h)
        max_y = (int)h - 1;

    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            for (uint32_t s = 0; s < msaa_level; ++s)
            {
                float sx = (float)(x) + sample_offsets_4x[s % 4][0];
                float sy = (float)(y) + sample_offsets_4x[s % 4][1];

                float a, b, c;
                barycentric(tri, sx, sy, &a, &b, &c);
                if (a < 0.0f || b < 0.0f || c < 0.0f)
                    continue;

                float z = a * tri->v1.z + b * tri->v2.z + c * tri->v3.z;

                size_t idx = ((size_t)y * w + (size_t)x) * msaa_level + s;

                float iw = a * tri->v1.w + b * tri->v2.w + c * tri->v3.w;
                float ca = a * tri->v1.w / iw;
                float cb = b * tri->v2.w / iw;
                float cc = c * tri->v3.w / iw;

                if (z < depth[idx])
                {
                    depth[idx] = z;

                    color_buffer[idx] =
                        COLOR_RGB((uint8_t)(tri->c1.r * ca + tri->c2.r * cb + tri->c3.r * cc),
                                  (uint8_t)(tri->c1.g * ca + tri->c2.g * cb + tri->c3.g * cc),
                                  (uint8_t)(tri->c1.b * ca + tri->c2.b * cb + tri->c3.b * cc));
                }
            }
        }
    }
}

static void msaa_resolve(framebuffer_t fb, uint32_t stride, uint32_t bpp, uint32_t w, uint32_t h)
{
    for (uint32_t y = 0; y < h; ++y)
    {
        for (uint32_t x = 0; x < w; ++x)
        {
            uint32_t r = 0, g = 0, b = 0;
            size_t base = ((size_t)y * w + x) * msaa_level;
            for (uint32_t s = 0; s < msaa_level; ++s)
            {
                color_t cs = color_buffer[base + s];
                r += cs.r;
                g += cs.g;
                b += cs.b;
            }
            r /= msaa_level;
            g /= msaa_level;
            b /= msaa_level;
            framebuffer_set_color(fb, stride, bpp, x, y,
                                  COLOR_RGB((uint8_t)r, (uint8_t)g, (uint8_t)b));
        }
    }
}

int main(void)
{
    window_t *window = window_create(PIXEL_FORMAT_RGB888);
    if (!window)
        return 1;

    window_set_size(window, 800, 600);
    window_set_title(window, "GAMES101");
    window_set_visible(window, true);
    window_set_flags(window, WINDOW_FLAG_RESIZABLE);
    // window_set_flags(window, WINDOW_FLAG_UNRESIZABLE);

    triangle_t *tris[2];

    triangle_t tri1 = triangle_create3f(
        (vec3f_t){  2.0f, 0.0f, -2.0f },
        (vec3f_t){  0.0f, 2.0f, -2.0f },
        (vec3f_t){ -2.0f, 0.0f, -2.0f },
        COLOR_RGB(0xFF, 0x00, 0x00), 
        COLOR_RGB(0x00, 0xFF, 0x00),
        COLOR_RGB(0x00, 0x00, 0xFF)
    );

    tris[0] = &tri1;

    triangle_t tri2 = triangle_create3f(
        (vec3f_t){  4.0f, -1.0f, -5.0f },
        (vec3f_t){  3.0f,  1.5f, -5.0f },
        (vec3f_t){ -0.5f,  0.5f, -5.0f },
        COLOR_RGB(0xFF, 0x00, 0xFF), 
        COLOR_RGB(0x00, 0xFF, 0x00),
        COLOR_RGB(0xFF, 0x00, 0xFF)
    );

    tris[1] = &tri2;

    size_t tri_count = 2;

    float n = 0.01f, f = 100.0f;
    float fov_y = 45.0f * ((float)M_PI / 180.0f);
    float aspect = (float)800 / (float)600;

    mat4f_t projection = get_perspective(fov_y, aspect, n, f);

    vec3f_t eyepos = { 0, 0, 5.0f };
    mat4f_t viewing = get_viewing(eyepos);

    depth_capacity = (size_t)800 * 600 * msaa_level;
    depth_size = depth_capacity;
    depth = malloc(sizeof(float) * depth_capacity);
    if (!depth)
        goto done;

    color_capacity = depth_capacity;
    color_size = depth_capacity;
    color_buffer = malloc(sizeof(color_t) * color_capacity);
    if (!color_buffer)
        goto done;

    while (1)
    {
        window_event_t ev;
        while ((ev = window_poll_event(window)) != WINDOW_EVENT_NONE)
        {
            if (ev == WINDOW_EVENT_CLOSE)
                goto done;
            if (ev == WINDOW_EVENT_RESIZE)
            {
                uint32_t w = window_get_width(window);
                uint32_t h = window_get_height(window);
                aspect = (float)w / (float)h;
                projection = get_perspective(fov_y, aspect, n, f);

                size_t new_size = (size_t)w * h * msaa_level;

                /* Z-buffer */
                if (new_size > depth_capacity)
                {
                    size_t new_cap = depth_capacity + depth_capacity / 2;
                    if (new_cap < new_size)
                        new_cap = new_size;

                    float *tmp = realloc(depth, new_cap * sizeof(float));
                    if (!tmp)
                    { /* TODO: Handle error */
                    }
                    else
                    {
                        depth = tmp;
                        depth_capacity = new_cap;
                    }
                }
                depth_size = new_size;

                /* Color buffer */
                if (new_size > color_capacity)
                {
                    size_t new_cap = color_capacity + color_capacity / 2;
                    if (new_cap < new_size)
                        new_cap = new_size;

                    color_t *tmp = realloc(color_buffer, new_cap * sizeof(color_t));
                    if (!tmp)
                    { /* TODO: Handle error */
                    }
                    else
                    {
                        color_buffer = tmp;
                        color_capacity = new_cap;
                    }
                }
                color_size = new_size;
            }
        }

        framebuffer_t fb = window_get_framebuffer(window);
        uint32_t stride = window_get_framebuffer_stride(window);
        uint32_t bpp = window_get_framebuffer_bytes_per_pixel(window);
        uint32_t w = window_get_width(window);
        uint32_t h = window_get_height(window);

        framebuffer_clear(fb, stride, bpp, w, h, COLOR_RGB(0xFF, 0xFF, 0xFF));
        for (size_t i = 0; i < depth_size; ++i)
            depth[i] = INFINITY;
        for (size_t i = 0; i < color_size; ++i)
            color_buffer[i] = COLOR_RGB(0xFF, 0xFF, 0xFF);

        for (size_t i = 0; i < tri_count; ++i)
        {
            mat4f_t mvp = mat4f_mul(projection, viewing);

            vec4f_t c1 = mat4f_mul_vec4f(mvp, tris[i]->v1);
            vec4f_t c2 = mat4f_mul_vec4f(mvp, tris[i]->v2);
            vec4f_t c3 = mat4f_mul_vec4f(mvp, tris[i]->v3);

            vec4f_t s1 = ndc_to_screen(perspective_divide(c1), w, h);
            vec4f_t s2 = ndc_to_screen(perspective_divide(c2), w, h);
            vec4f_t s3 = ndc_to_screen(perspective_divide(c3), w, h);

            triangle_t tri =
                triangle_create4f(s1, s2, s3, tris[i]->c1, tris[i]->c2, tris[i]->c3);

            rasterization(&tri, w, h);
        }

        msaa_resolve(fb, stride, bpp, w, h);

        window_swap_framebuffer(window);
    }

done:
    free(depth);
    free(color_buffer);
    window_destroy(window);
    return 0;
}