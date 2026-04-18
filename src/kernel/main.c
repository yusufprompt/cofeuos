/*
 * MAIN.C - cofeuOS Kernel Entry Point
 */

#include "../include/types.h"
#include "../include/io.h"
#include "../include/memory.h"
#include "../include/fs.h"
#include "../include/shell.h"
#include "../include/video.h"
#include "../include/string.h"

shell_control g_shell;
fs_control_block g_fs;
memory_arena g_mem_arena;
int cursor_x = 5;
int cursor_y = 30;

static u8 shift_pressed = 0;

static const char key_normal[59] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    9, 'q','w','e','r','t','y','u','i','o','p','[',']',10,
    0,'a','s','d','f','g','h','j','k','l',';','\'', '`', 0,
    '\\','z','x','c','v','b','n','m',',','.','/', 0,42,0,' '
};

static const char key_shift[59] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    9, 'Q','W','E','R','T','Y','U','I','O','P','{','}',10,
    0,'A','S','D','F','G','H','J','K','L',':','"','~', 0,
    '|','Z','X','C','V','B','N','M','<','>','?', 0,42,0,' '
};

static char read_key(void) {
    while (1) {
        if (inb(0x64) & 1) {
            u8 sc = inb(0x60);
            if (sc == 0x2A || sc == 0x36) {
                shift_pressed = 1;
                continue;
            }
            if (sc == 0xAA || sc == 0xB6) {
                shift_pressed = 0;
                continue;
            }
            if (sc == 0x0E) {
                return '\b';
            }
            if (sc < 59 && !(sc & 0x80)) {
                char c = shift_pressed ? key_shift[sc] : key_normal[sc];
                if (c) return c;
            }
        }
    }
}

static void next_line(void) {
    cursor_x = 5;
    cursor_y += LINE_HEIGHT;
    if (cursor_y > SCREEN_HEIGHT - LINE_HEIGHT) {
        video_scroll();
        cursor_y = SCREEN_HEIGHT - LINE_HEIGHT;
    }
}

static void erase_at(int x, int y) {
    if (x >= 0 && y >= 0) {
        video_clear_rect(x, y, font_width, font_height);
    }
}

static int get_login_input(char* buffer, int max_len, int start_x, int y, int masked) {
    int x = start_x;
    int len = 0;

    while (1) {
        char ch = read_key();
        if (ch == '\n') {
            buffer[len] = '\0';
            return len;
        }
        if (ch == '\b') {
            if (len > 0) {
                len--;
                x -= font_width;
                if (x < start_x) {
                    x = start_x;
                }
                erase_at(x, y);
            }
            continue;
        }
        if (ch >= ' ' && len < max_len - 1) {
            buffer[len++] = ch;
            video_draw_char(masked ? '*' : ch, x, y, 15);
            x += font_width;
        }
    }
}

static void show_login(void) {
    char username[32];
    char password[32];

    while (1) {
        video_clear(0);

        video_print("cofeuOS Login", 5, 20, 14);
        video_print("====================", 5, 20 + font_height + 2, 7);

        int login_y = 20 + font_height + 2 + font_height + 5;
        video_print("Login: ", 5, login_y, 15);
        int login_x = 5 + 7 * font_width;

        int ulen = get_login_input(username, sizeof(username), login_x, login_y, 0);
        if (ulen == 0) {
            video_print("Username required", 5, login_y + font_height + 5, 12);
            continue;
        }

        int pass_y = login_y + font_height + 5;
        video_print("Password: ", 5, pass_y, 15);
        int pass_x = 5 + 10 * font_width;
        get_login_input(password, sizeof(password), pass_x, pass_y, 1);

        strcpy(g_shell.user, username);
        cursor_x = 5;
        cursor_y = pass_y + font_height + 5;
        return;
    }
}

int main_get_input(char* buffer, int max_len) {
    int pos = 0;

    while (1) {
        char ch = read_key();
        if (ch == '\n') {
            buffer[pos] = '\0';
            next_line();
            return pos;
        }
        if (ch == '\b') {
            if (pos > 0) {
                pos--;
                if (cursor_x > 5) {
                    cursor_x -= font_width;
                } else {
                    cursor_x = 5;
                }
                erase_at(cursor_x, cursor_y);
            }
            continue;
        }
        if (ch >= ' ' && pos < max_len - 1) {
            if (cursor_x >= SCREEN_WIDTH - font_width) {
                next_line();
            }
            buffer[pos++] = ch;
            video_draw_char(ch, cursor_x, cursor_y, 15);
            cursor_x += font_width;
        }
    }
}

static void print_shell_prompt(void) {
    int x = 5;
    video_print(g_shell.user, x, cursor_y, 10);
    x += video_text_width(g_shell.user);

    video_print("@", x, cursor_y, 10);
    x += font_width;

    video_print(g_shell.host, x, cursor_y, 14);
    x += video_text_width(g_shell.host);

    video_print(" [", x, cursor_y, 11);
    x += video_text_width(" [");

    video_print(g_shell.partition, x, cursor_y, 11);
    x += video_text_width(g_shell.partition);

    video_print("] ", x, cursor_y, 11);
    x += video_text_width("] ");

    video_print(g_shell.cwd, x, cursor_y, 12);
    x += video_text_width(g_shell.cwd);

    video_print(" # ", x, cursor_y, 15);
    x += video_text_width(" # ");
    cursor_x = x;
}

void kernel_main(void) {
    video_init();
    video_clear(0);

    video_print("cofeuOS v3.0 - Boot", 5, 5, 14);
    video_print("====================", 5, 5 + font_height + 2, 7);

    void* mem_base = (void*)0x100000;
    mem_init(&g_mem_arena, mem_base, 16 * 1024 * 1024);
    fs_init(&g_fs);

    strcpy(g_shell.host, "cofeu");
    strcpy(g_shell.partition, "/dev/sda1");
    strcpy(g_shell.cwd, "/");

    cursor_x = 5;
    cursor_y = 5 + font_height + 2 + font_height + 8;

    show_login();

    char cmd[512];
    while (1) {
        next_line();
        print_shell_prompt();
        int len = main_get_input(cmd, sizeof(cmd));
        if (len > 0) {
            shell_execute(cmd);
        }
    }
}
