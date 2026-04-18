/*
 * VIDEO.H - VGA Video Fonksiyonları
 */
#ifndef _VIDEO_H
#define _VIDEO_H

#include "types.h"
#include "terminus_font.h"

#define VIDEO_MEMORY 0xA0000
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

extern int font_width;
extern int font_height;
extern int line_height;
#define CHAR_WIDTH font_width
#define CHAR_HEIGHT font_height
#define LINE_HEIGHT line_height

void video_init(void);
void video_clear(u8 color);
void video_put_pixel(int x, int y, u8 color);
void video_draw_char(char c, int x, int y, u8 color);
void video_clear_rect(int x, int y, int width, int height);
int video_text_width(const char* str);
void video_print(const char* str, int x, int y, u8 color);
void video_scroll(void);

#endif

