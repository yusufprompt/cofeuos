/*
 * VIDEO.C - VGA 320x200x256 Video Driver
 */

#include "../include/video.h"
#include "../include/io.h"
#include "../include/types.h"
#include "../include/terminus_font.h"

static u8* video_mem = (u8*)VIDEO_MEMORY;
static psf2_t* font = (psf2_t*)font_psf;
int font_width = 8;
int font_height = 8;
int line_height = 8;

/* VGA 13h mode init */
void video_init(void) {
    if (font) {
        font_width = font->width;
        font_height = font->height;
        line_height = font->height;
    }
    video_clear(0);
}

void video_clear(u8 color) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        video_mem[i] = color;
    }
}

void video_put_pixel(int x, int y, u8 color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        video_mem[y * SCREEN_WIDTH + x] = color;
    }
}

void video_draw_char(char c, int x, int y, u8 color) {
    if (!font) return;

    u8* glyph = (u8*)font_psf + font->headersize + (unsigned int)c * font->charsize;
    int bytes_per_row = (font->width + 7) / 8;

    for (u32 row = 0; row < font->height; row++) {
        for (u32 col = 0; col < font->width; col++) {
            int byte_index = row * bytes_per_row + (col / 8);
            if (byte_index < font->charsize && glyph[byte_index] & (1 << (7 - (col % 8)))) {
                video_put_pixel(x + col, y + row, color);
            }
        }
    }
}

void video_clear_rect(int x, int y, int width, int height) {
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            video_put_pixel(x + dx, y + dy, 0);
        }
    }
}

int video_text_width(const char* str) {
    int width = 0;
    while (*str++) width += font_width;
    return width;
}

void video_print(const char* str, int x, int y, u8 color) {
    int cx = x;
    while (*str) {
        if (*str == '\n') {
            y += LINE_HEIGHT;
            cx = x;
        } else {
            video_draw_char(*str, cx, y, color);
            cx += font_width;
        }
        str++;
        if (cx > SCREEN_WIDTH - font_width) {
            cx = x;
            y += LINE_HEIGHT;
        }
    }
}

void video_scroll(void) {
    /* Scroll up one text line (LINE_HEIGHT pixels) */
    for (int y = LINE_HEIGHT; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            video_mem[(y-LINE_HEIGHT) * SCREEN_WIDTH + x] = video_mem[y * SCREEN_WIDTH + x];
        }
    }
    /* Clear last char row */
    for (int y = SCREEN_HEIGHT - LINE_HEIGHT; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            video_mem[y * SCREEN_WIDTH + x] = 0;
        }
    }
}

