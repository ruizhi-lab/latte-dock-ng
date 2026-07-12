/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later

    Small helper that captures a single frame from a PipeWire screencast
    node and writes it as a PNG file.

    Usage: latte-pw-capture <node-id> <output-path>
*/

#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/debug/types.h>
#include <spa/param/props.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <png.h>

static struct pw_thread_loop *loop = NULL;
static struct pw_stream *stream = NULL;
static struct spa_video_info format;
static int done = 0;
static int has_frame = 0;

static uint8_t *frame_data = NULL;
static int frame_width = 0;
static int frame_height = 0;
static int frame_stride = 0;
static const char *output_path = NULL;

static void on_process(void *data)
{
    struct pw_buffer *b;
    struct spa_buffer *buf;

    if (has_frame)
        return;

    b = pw_stream_dequeue_buffer(stream);
    if (!b) {
        pw_log_warn("no buffer");
        return;
    }

    buf = b->buffer;
    if (buf->n_datas > 0 && buf->datas[0].chunk->size > 0) {
        struct spa_data *d = &buf->datas[0];
        int size = d->chunk->size;
        int stride = d->chunk->stride ? d->chunk->stride
                    : (format.info.raw.size.width * 4);

        frame_data = (uint8_t *)malloc(size);
        if (frame_data) {
            memcpy(frame_data, d->data, size);
            frame_width = format.info.raw.size.width;
            frame_height = format.info.raw.size.height;
            frame_stride = stride;
            has_frame = 1;
        }
    }

    pw_stream_queue_buffer(stream, b);
    done = 1;
    pw_thread_loop_signal(loop, FALSE);
}

static int write_png(const char *path, const uint8_t *data,
                     int width, int height, int stride)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) return -1;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) { fclose(fp); return -1; }

    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_write_struct(&png, NULL); fclose(fp); return -1; }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return -1;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    // PipeWire typically gives BGRA; convert to RGBA for PNG
    png_bytep row = (png_bytep)malloc(stride);
    for (int y = 0; y < height; y++) {
        const uint8_t *src = data + y * stride;
        for (int x = 0; x < width; x++) {
            row[x * 4 + 0] = src[x * 4 + 2]; // R
            row[x * 4 + 1] = src[x * 4 + 1]; // G
            row[x * 4 + 2] = src[x * 4 + 0]; // B
            row[x * 4 + 3] = src[x * 4 + 3]; // A
        }
        png_write_row(png, row);
    }
    free(row);

    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return 0;
}

static void on_param_changed(void *data, uint32_t id,
                              const struct spa_pod *param)
{
    if (param == NULL || id != SPA_PARAM_Format)
        return;

    spa_format_video_raw_parse(param, &format.info.raw);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .param_changed = on_param_changed,
    .process = on_process,
};

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <node-id> <output-path>\n", argv[0]);
        return 1;
    }

    uint32_t node_id = (uint32_t)strtoul(argv[1], NULL, 10);
    output_path = argv[2];

    signal(SIGPIPE, SIG_IGN);

    pw_init(&argc, &argv);

    loop = pw_thread_loop_new("latte-pw-capture", NULL);
    if (!loop) {
        fprintf(stderr, "pw_thread_loop_new failed\n");
        return 1;
    }

    struct pw_context *context = pw_context_new(
        pw_thread_loop_get_loop(loop), NULL, 0);
    if (!context) {
        fprintf(stderr, "pw_context_new failed\n");
        return 1;
    }

    struct pw_core *core = pw_context_connect(context, NULL, 0);
    if (!core) {
        fprintf(stderr, "pw_context_connect failed\n");
        return 1;
    }

    stream = pw_stream_new(core, "latte-capture",
                           pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
                                             PW_KEY_MEDIA_CATEGORY, "Capture",
                                             PW_KEY_MEDIA_ROLE, "Screen",
                                             NULL));
    if (!stream) {
        fprintf(stderr, "pw_stream_new failed\n");
        return 1;
    }

    pw_stream_add_listener(stream, &stream_events,
                           &stream_events, sizeof(stream_events));

    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    const struct spa_pod *params[1];
    params[0] = (const struct spa_pod *)spa_pod_builder_add_object(&b,
        SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
        SPA_FORMAT_mediaType,       SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype,    SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
        SPA_FORMAT_VIDEO_format,    SPA_POD_CHOICE_ENUM_Id(4,
            SPA_VIDEO_FORMAT_BGRA,
            SPA_VIDEO_FORMAT_RGBA,
            SPA_VIDEO_FORMAT_BGRx,
            SPA_VIDEO_FORMAT_RGBx),
        SPA_FORMAT_VIDEO_size,      SPA_POD_CHOICE_RANGE_Rectangle(
            &SPA_RECTANGLE(640, 480),
            &SPA_RECTANGLE(1, 1),
            &SPA_RECTANGLE(4096, 4096)),
        0);

    if (pw_stream_connect(stream, PW_DIRECTION_INPUT, node_id,
                          (enum pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                                                  PW_STREAM_FLAG_DONT_RECONNECT),
                          params, 1) < 0) {
        fprintf(stderr, "pw_stream_connect failed\n");
        return 1;
    }

    pw_thread_loop_start(loop);

    // Wait for one frame (max 5 seconds)
    pw_thread_loop_lock(loop);
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 5;

    while (!done) {
        if (pw_thread_loop_timed_wait(loop, &timeout) != 0)
            break;
    }
    pw_thread_loop_unlock(loop);

    pw_thread_loop_stop(loop);

    if (has_frame && frame_data) {
        fprintf(stderr, "Got frame %dx%d stride=%d\n",
                frame_width, frame_height, frame_stride);
        write_png(output_path, frame_data, frame_width, frame_height,
                  frame_stride);
        free(frame_data);
    } else {
        fprintf(stderr, "No frame captured\n");
    }

    pw_stream_destroy(stream);
    pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_thread_loop_destroy(loop);
    pw_deinit();

    return has_frame ? 0 : 1;
}
