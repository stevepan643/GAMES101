#include "stb_image.h"

#include "compute.h"
#include "pipeline.h"
#include "triangle.h"
#include "window.h"
#include "texture.h"

#include <stdio.h>

float *depth = NULL;
size_t depth_size = 0;
size_t depth_capacity = 0;

color_t *color_buffer = NULL;
size_t color_size = 0;
size_t color_capacity = 0;

uint32_t msaa_level = 16;

typedef struct
{
    vec3f_t pos;
    vec3f_t color;
    vec2f_t uv;
} vertex_t;

typedef struct
{
    mat4f_t model;
    mat4f_t viewing_projection;
} vuniform_t;

typedef struct
{
    texture_t *texture;
} funiform_t;

typedef struct
{
    vec3f_t color;
    vec2f_t uv;
} varing_t;

int texture_load(texture_t *tex, const char *path)
{
    int w, h, ch;

    stbi_set_flip_vertically_on_load(1);

    unsigned char *data = stbi_load(path, &w, &h, &ch, 4);

    if (!data) {
        fprintf(stderr, "Load failed: %s\n Because: %s\n",
                path, stbi_failure_reason());
        return -1;
    }

    tex->data     = data;
    tex->width    = w;
    tex->height   = h;
    tex->channels = 4;
    return 0;
}
void texture_free(texture_t *tex)
{
    if (tex->data) {
        stbi_image_free(tex->data);
        tex->data = NULL;
    }
    tex->width = tex->height = tex->channels = 0;
}

static inline mat4f_t get_viewing(vec3f_t eyepos)
{
    mat4f_t viewing = {
        {{1, 0, 0, -eyepos.x}, {0, 1, 0, -eyepos.y}, {0, 0, 1, -eyepos.z}, {0, 0, 0, 1}}};
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

void vs(const vertex_input_t *input, void *out, vec4f_t *out_position, void *uniform)
{
    vec3f_t position = program_location_get3f(input, 0);
    vec3f_t color = program_location_get3f(input, 1);

    vuniform_t *uni = (vuniform_t *)uniform;
    *out_position = mat4f_mul_vec4f(
        uni->viewing_projection,
        mat4f_mul_vec4f(uni->model, (vec4f_t){position.x, position.y, position.z, 1})
    );
    varing_t *v = (varing_t *)out;
    v->color = color;
    v->uv = program_location_get2f(input, 2);
}

void fs(vec2f_t screen_pos, float fdepth, const void *in,
        color_t *out_color, void *uniform)
{
    (void)screen_pos;
    (void)fdepth;
    funiform_t *funi = (funiform_t *)uniform;
    varing_t *v = (varing_t *)in;

    color_t c = texture_sample(funi->texture, v->uv.x, v->uv.y,
                               WRAP_CLAMP, FILTER_BILINEAR);

    float a = c.a / 255.0f;

    float tr = c.r / 255.0f;
    float tg = c.g / 255.0f;
    float tb = c.b / 255.0f;

    float rr = tr * a + v->color.x * (1.0f - a);
    float rg = tg * a + v->color.y * (1.0f - a);
    float rb = tb * a + v->color.z * (1.0f - a);

    out_color->r = (unsigned char)(rr * 255.0f + 0.5f);
    out_color->g = (unsigned char)(rg * 255.0f + 0.5f);
    out_color->b = (unsigned char)(rb * 255.0f + 0.5f);
    out_color->a = 255;
}

void init(window_t **window_out)
{
    window_t *window = window_create(PIXEL_FORMAT_RGB888);
    if (!window)
        return;

    window_set_size(window, 400, 400);
    window_set_title(window, "GAMES101");
    window_set_visible(window, true);
    window_set_flags(window, WINDOW_FLAG_RESIZABLE);

    depth_capacity = (size_t)400 * 400 * msaa_level;
    depth_size = depth_capacity;
    depth = malloc(sizeof(float) * depth_capacity);
    if (!depth)
        goto fail;

    color_capacity = depth_capacity;
    color_size = depth_capacity;
    color_buffer = malloc(sizeof(color_t) * color_capacity);
    if (!color_buffer)
        goto fail;

    *window_out = window;
    return;

fail:
    free(depth);        depth = NULL;
    free(color_buffer); color_buffer = NULL;
    window_destroy(window);
}

void framebuffer_resize(window_t *window)
{
    uint32_t w = window_get_width(window);
    uint32_t h = window_get_height(window);

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

int main(void)
{
    window_t *window = NULL;
    init(&window);

    vertex_attr_t attr[] = {
        {0, VERT_ATTR_FLOAT3, 0},
        {1, VERT_ATTR_FLOAT3, sizeof(float) * 3},
        {2, VERT_ATTR_FLOAT2, sizeof(float) * 6}
    };

    vertex_t vertices[] = {
        {{ 0.0f,  1.8f, -2.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f}},
        {{-1.8f, -1.5f, -2.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.8f, -1.5f, -2.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    };

    uint32_t indices[] = {
        0, 1, 2,
    };

    texture_t texture;
    texture_load(&texture, "resource/texture.png");

    funiform_t funi = (funiform_t){ .texture = &texture };

    float n = 0.01f, f = 100.0f;
    float fov_y = 60.0f * ((float)M_PI / 180.0f);
    float aspect = (float)400 / (float)400;

    mat4f_t projection = get_perspective(fov_y, aspect, n, f);
    vec3f_t eyepos = {0, 0, 5.0f};
    mat4f_t viewing = get_viewing(eyepos);

    program_t *program = create_program();
    program_set_vertex_shader(program, vs);
    program_set_fragment_shader(program, fs);
    program_link(program, sizeof(varing_t));

    render_target_t target = {
        .color_buffer = color_buffer,
        .sample_count = msaa_level,
        .z_buffer = depth
    };

    uint64_t last_time = window_get_time();
    uint64_t last_fps_update = last_time;
    int frame_count = 0;
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
                target.w = w;
                target.h = h;
                aspect = (float)w / (float)h;
                projection = get_perspective(fov_y, aspect, n, f);
                framebuffer_resize(window);
                target.color_buffer = color_buffer;
                target.z_buffer = depth;
            }
        }

        framebuffer_t fb = window_get_framebuffer(window);
        uint32_t stride = window_get_framebuffer_stride(window);
        uint32_t bpp = window_get_framebuffer_bytes_per_pixel(window);
        uint32_t w = window_get_width(window);
        uint32_t h = window_get_height(window);

        for (size_t i = 0; i < depth_size; ++i)
            depth[i] = INFINITY;
        for (size_t i = 0; i < color_size; ++i)
            color_buffer[i] = COLOR_RGB(0xAF, 0xAF, 0xAF);
        
        uint64_t t_ms = window_get_time();
        float t = (float)t_ms / 1000.0f;
        float angle = t * 1.0f;

        mat4f_t model = mat4f_rotate_y(angle);

        vuniform_t vunif = {
            .model = model,
            .viewing_projection = mat4f_mul(projection, viewing)
        };
        program_draw(program, target, attr, 3, sizeof(vertex_t),
                     vertices, 3,
                     indices, 3,
                     (void *)&vunif, (void *)&funi);

        msaa_resolve(fb, stride, bpp, w, h);

        window_swap_framebuffer(window);

        frame_count++;
        uint64_t now = window_get_time();

        if (now - last_fps_update >= 500)
        {
            double elapsed = (double)(now - last_fps_update) / 1000.0;
            double fps = frame_count / elapsed;

            char title[128];
            snprintf(title, sizeof(title), "GAMES101 (FPS: %.1f)", fps);
            window_set_title(window, title);

            frame_count = 0;
            last_fps_update = now;
        }
    }

done:
    free(depth);
    free(color_buffer);
    texture_free(&texture);
    window_destroy(window);
    return 0;
}