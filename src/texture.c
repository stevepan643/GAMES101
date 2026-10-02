#include "texture.h"

#include "stb_image.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct texture_t
{
    unsigned char *data;

    unsigned char *mip_buffer;
    int *mip_width;
    int *mip_height;
    int *mip_offset;
    int mip_count;

    int width;
    int height;
    int channels;

    wrap_mode_t wrap_mode;
    filter_mode_t filter_mode;
};

static void texture_gen_mipmaps(texture_t *tex)
{
    size_t mipmap_count = 1 + (size_t)floor(log2(fmax((double)tex->width, (double)tex->height)));
    tex->mip_count = (int)mipmap_count;

    tex->mip_width = (int *)malloc(sizeof(int) * mipmap_count);
    tex->mip_height = (int *)malloc(sizeof(int) * mipmap_count);
    tex->mip_offset = (int *)malloc(sizeof(int) * mipmap_count);

    size_t total_bytes = 0;
    for (int i = 0; i < tex->mip_count; ++i)
    {
        int mip_w = (int)fmax(1, tex->width >> i);
        int mip_h = (int)fmax(1, tex->height >> i);
        tex->mip_width[i] = mip_w;
        tex->mip_height[i] = mip_h;
        tex->mip_offset[i] = (int)total_bytes;
        total_bytes += (size_t)mip_w * mip_h * tex->channels;
    }

    tex->mip_buffer = (unsigned char *)malloc(total_bytes);
    if (!tex->mip_buffer)
        return;

    for (int i = 0; i < tex->mip_count; ++i)
    {
        int mip_w = tex->mip_width[i];
        int mip_h = tex->mip_height[i];
        unsigned char *dst_ptr = tex->mip_buffer + tex->mip_offset[i];

        if (i == 0)
        {
            memcpy(dst_ptr, tex->data, (size_t)tex->width * tex->height * tex->channels);
        }
        else
        {
            for (int y = 0; y < mip_h; ++y)
            {
                for (int x = 0; x < mip_w; ++x)
                {
                    int src_x = x * (tex->width / mip_w);
                    int src_y = y * (tex->height / mip_h);
                    int src_index = (src_y * tex->width + src_x) * tex->channels;
                    int dst_index = (y * mip_w + x) * tex->channels;

                    for (int c = 0; c < tex->channels; ++c)
                    {
                        dst_ptr[dst_index + c] = tex->data[src_index + c];
                    }
                }
            }
        }
    }
}

static int texture_load(texture_t *tex, const char *path)
{
    int w, h, ch;
    stbi_set_flip_vertically_on_load(1);

    unsigned char *data = stbi_load(path, &w, &h, &ch, 4);
    if (!data)
    {
        return -1;
    }

    tex->data = data;
    tex->width = w;
    tex->height = h;
    tex->channels = 4;
    return 0;
}

texture_t *texture_create(const char *path, wrap_mode_t wrap, filter_mode_t filter,
                          bool generate_mipmaps)
{
    texture_t *tex = (texture_t *)calloc(1, sizeof(texture_t));
    if (!tex)
        return NULL;

    if (texture_load(tex, path) != 0)
    {
        free(tex);
        return NULL;
    }

    if (generate_mipmaps)
    {
        texture_gen_mipmaps(tex);
    }
    else
    {
        tex->mip_buffer = NULL;
        tex->mip_width = NULL;
        tex->mip_height = NULL;
        tex->mip_offset = NULL;
        tex->mip_count = 0;
    }

    tex->wrap_mode = wrap;
    tex->filter_mode = filter;
    return tex;
}

void texture_free(texture_t *tex)
{
    if (!tex)
        return;

    if (tex->data)
    {
        stbi_image_free(tex->data);
        tex->data = NULL;
    }
    if (tex->mip_buffer)
    {
        free(tex->mip_buffer);
        tex->mip_buffer = NULL;
    }
    if (tex->mip_width)
    {
        free(tex->mip_width);
        tex->mip_width = NULL;
    }
    if (tex->mip_height)
    {
        free(tex->mip_height);
        tex->mip_height = NULL;
    }
    if (tex->mip_offset)
    {
        free(tex->mip_offset);
        tex->mip_offset = NULL;
    }
    free(tex);
}

color_t _texture_sample(const unsigned char *data, int width, int height, int channels, float u,
                        float v, wrap_mode_t wrap, filter_mode_t filter)
{
    if (!data || width <= 0 || height <= 0 || channels <= 0)
    {
        return (color_t){0, 0, 0, 255};
    }

    if (wrap == WRAP_REPEAT)
    {
        u = u - floorf(u);
        v = v - floorf(v);
    }
    else if (wrap == WRAP_MIRROR)
    {
        u = fabsf(fmodf(u, 2.0f));
        v = fabsf(fmodf(v, 2.0f));
        if (u > 1.0f)
            u = 2.0f - u;
        if (v > 1.0f)
            v = 2.0f - v;
    }
    else /* WRAP_CLAMP */
    {
        u = fmaxf(0.0f, fminf(1.0f, u));
        v = fmaxf(0.0f, fminf(1.0f, v));
    }

    if (filter == FILTER_NEAREST)
    {
        int x = (int)(u * (float)width) % width;
        int y = (int)(v * (float)height) % height;
        if (x < 0)
            x += width;
        if (y < 0)
            y += height;

        int index = (y * width + x) * channels;
        return (color_t){data[index], data[index + 1], data[index + 2], data[index + 3]};
    }
    else /* FILTER_BILINEAR */
    {
        float x = u * (float)width - 0.5f;
        float y = v * (float)height - 0.5f;

        int x0 = (int)floorf(x);
        int y0 = (int)floorf(y);
        int x1 = x0 + 1;
        int y1 = y0 + 1;

        float sx = x - (float)x0;
        float sy = y - (float)y0;

        color_t c00 = _texture_sample(data, width, height, channels, (float)x0 / (float)width,
                                      (float)y0 / (float)height, wrap, FILTER_NEAREST);
        color_t c10 = _texture_sample(data, width, height, channels, (float)x1 / (float)width,
                                      (float)y0 / (float)height, wrap, FILTER_NEAREST);
        color_t c01 = _texture_sample(data, width, height, channels, (float)x0 / (float)width,
                                      (float)y1 / (float)height, wrap, FILTER_NEAREST);
        color_t c11 = _texture_sample(data, width, height, channels, (float)x1 / (float)width,
                                      (float)y1 / (float)height, wrap, FILTER_NEAREST);

        color_t result;
        result.r = (unsigned char)((1.0f - sx) * ((1.0f - sy) * c00.r + sy * c01.r) +
                                   sx * ((1.0f - sy) * c10.r + sy * c11.r));
        result.g = (unsigned char)((1.0f - sx) * ((1.0f - sy) * c00.g + sy * c01.g) +
                                   sx * ((1.0f - sy) * c10.g + sy * c11.g));
        result.b = (unsigned char)((1.0f - sx) * ((1.0f - sy) * c00.b + sy * c01.b) +
                                   sx * ((1.0f - sy) * c10.b + sy * c11.b));
        result.a = (unsigned char)((1.0f - sx) * ((1.0f - sy) * c00.a + sy * c01.a) +
                                   sx * ((1.0f - sy) * c10.a + sy * c11.a));
        return result;
    }
}

color_t texture_sample(const texture_t *tex, float u, float v, float lod)
{
    if (!tex || !tex->data)
    {
        return (color_t){0, 0, 0, 255};
    }

    if (tex->mip_count > 0 && tex->mip_buffer)
    {
        lod = fmaxf(0.0f, fminf(lod, (float)(tex->mip_count - 1)));

        int lod1 = (int)floorf(lod);
        int lod2 = (int)ceilf(lod);

        if (lod1 == lod2)
        {
            const unsigned char *data_ptr = tex->mip_buffer + tex->mip_offset[lod1];
            return _texture_sample(data_ptr, tex->mip_width[lod1], tex->mip_height[lod1],
                                   tex->channels, u, v, tex->wrap_mode, tex->filter_mode);
        }
        else
        {
            const unsigned char *data_ptr1 = tex->mip_buffer + tex->mip_offset[lod1];
            const unsigned char *data_ptr2 = tex->mip_buffer + tex->mip_offset[lod2];

            color_t c1 = _texture_sample(data_ptr1, tex->mip_width[lod1], tex->mip_height[lod1],
                                         tex->channels, u, v, tex->wrap_mode, tex->filter_mode);
            color_t c2 = _texture_sample(data_ptr2, tex->mip_width[lod2], tex->mip_height[lod2],
                                         tex->channels, u, v, tex->wrap_mode, tex->filter_mode);

            float t = lod - (float)lod1;
            color_t result;
            result.r = (unsigned char)((1.0f - t) * c1.r + t * c2.r);
            result.g = (unsigned char)((1.0f - t) * c1.g + t * c2.g);
            result.b = (unsigned char)((1.0f - t) * c1.b + t * c2.b);
            result.a = (unsigned char)((1.0f - t) * c1.a + t * c2.a);
            return result;
        }
    }

    return _texture_sample(tex->data, tex->width, tex->height, tex->channels, u, v, tex->wrap_mode,
                           tex->filter_mode);
}