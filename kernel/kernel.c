/*
 * ============================================================================
 * KERNEL.C - COFEUOS v2.0
 * ARCHITECTURE: x86 (i686) - VGA 13H MODE
 * ============================================================================
 */

#include "terminus_font.h"

// --- CORE TYPES ---
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// --- CONSTANTS ---
#define VIDEO_MEMORY ((u8*)0xA0000)
#define SCREEN_WIDTH 320
#define MAX_BUFFER 256
#define MAX_FILES 16
#define MAX_PATH 64
#define MAX_USER_DIRS 24

// --- STRUCTURES ---
typedef struct {
    u32 magic, version, headersize, flags, length, charsize, height, width;
} psf2_t;

typedef struct {
    char name[MAX_PATH], content[512];
    int size, active;
} File;

// --- GLOBAL STATE ---
int cursor_x = 5, cursor_y = 5, cmd_ptr = 0, is_nano = 0, is_tscreen = 0, ctrl = 0, shift = 0, f_count = 0;
char cmd_buffer[MAX_BUFFER], user[16] = "os", host[32] = "cofeu", path[MAX_PATH] = "/";
char sudo_password[32];
char nano_file[MAX_PATH], nano_text[512];
int nano_len = 0;
char user_dirs[MAX_USER_DIRS][MAX_PATH];
int user_dir_count = 0;
File fs[MAX_FILES];
void kprint(char* s, u8 col);
char get_ascii(u8 sc);
int find_f(char* n);

// --- HARDWARE I/O ---
void outb(u16 port, u8 data) {
    __asm__ __volatile__("outb %0, %1" : : "a"(data), "Nd"(port));
}

u8 inb(u16 port) {
    u8 res;
    __asm__ __volatile__("inb %1, %0" : "=a"(res) : "Nd"(port));
    return res;
}

// --- STRING UTILS ---
int strlen(char* s) { int i = 0; while (s[i]) i++; return i; }

int strcmp(char* s1, char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(u8*)s1 - *(u8*)s2;
}

int strncmp(char* s1, char* s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return 1;
        if (s1[i] == '\0') return 0;
    }
    return 0;
}

void strcpy(char* d, char* s) { int i = 0; while ((d[i] = s[i]) != '\0') i++; }

void strcopy_n(char* d, char* s, int max) {
    int i = 0;
    if (max <= 0) return;
    while (s[i] && i < max - 1) { d[i] = s[i]; i++; }
    d[i] = '\0';
}

char* skip_spaces(char* s) {
    while (*s == ' ') s++;
    return s;
}

char to_upper_ascii(char c) {
    if (c >= 'a' && c <= 'z') return c - ('a' - 'A');
    return c;
}

int strcmp_ci(char* s1, char* s2) {
    while (*s1 && *s2) {
        char c1 = to_upper_ascii(*s1++);
        char c2 = to_upper_ascii(*s2++);
        if (c1 != c2) return (u8)c1 - (u8)c2;
    }
    return (u8)to_upper_ascii(*s1) - (u8)to_upper_ascii(*s2);
}

int strncmp_ci(char* s1, char* s2, int n) {
    for (int i = 0; i < n; i++) {
        char c1 = to_upper_ascii(s1[i]);
        char c2 = to_upper_ascii(s2[i]);
        if (c1 != c2) return 1;
        if (s1[i] == '\0') return 0;
    }
    return 0;
}

int is_reserved_dir(char* p) {
    if (strcmp(p, "/") == 0) return 1;
    if (strcmp(p, "/bin") == 0) return 1;
    if (strcmp(p, "/dev") == 0) return 1;
    if (strcmp(p, "/etc") == 0) return 1;
    return 0;
}

int dir_exists(char* p) {
    if (is_reserved_dir(p)) return 1;
    for (int i = 0; i < user_dir_count; i++)
        if (strcmp(p, user_dirs[i]) == 0) return 1;
    return 0;
}

int is_valid_component(char* name) {
    int len = strlen(name);
    if (len <= 0 || len >= 16) return 0;
    for (int i = 0; name[i]; i++)
        if (name[i] == '/' || name[i] == ' ' || name[i] == '\t') return 0;
    return 1;
}

char* path_basename_ptr(char* p) {
    int last = -1;
    for (int i = 0; p[i]; i++) if (p[i] == '/') last = i;
    return (last == -1) ? p : (p + last + 1);
}

void path_parent_of(char* full, char* out) {
    int last = -1;
    if (strcmp(full, "/") == 0) { out[0] = '/'; out[1] = '\0'; return; }
    for (int i = 0; full[i]; i++) if (full[i] == '/') last = i;
    if (last <= 0) { out[0] = '/'; out[1] = '\0'; return; }
    for (int i = 0; i < last; i++) out[i] = full[i];
    out[last] = '\0';
}

void path_pop_last(char* p) {
    int i = strlen(p) - 1;
    if (i <= 0) { p[0] = '/'; p[1] = '\0'; return; }
    while (i > 0 && p[i] != '/') i--;
    if (i == 0) { p[0] = '/'; p[1] = '\0'; }
    else p[i] = '\0';
}

int path_append_component(char* out, char* comp) {
    int olen = strlen(out), clen = strlen(comp);
    if (strcmp(out, "/") == 0) {
        if (1 + clen >= MAX_PATH) return 1;
        strcpy(out + 1, comp);
        return 0;
    }
    if (olen + 1 + clen >= MAX_PATH) return 1;
    out[olen] = '/';
    strcpy(out + olen + 1, comp);
    return 0;
}

int build_abs_path(char* input, char* out) {
    char tok[16];
    int i = 0, tp = 0;
    input = skip_spaces(input);
    if (input[0] == '\0') { strcopy_n(out, path, MAX_PATH); return 0; }
    if (input[0] == '/') { out[0] = '/'; out[1] = '\0'; i = 1; }
    else { strcopy_n(out, path, MAX_PATH); }

    while (1) {
        char c = input[i];
        if (c == '/' || c == '\0') {
            tok[tp] = '\0';
            if (tp > 0) {
                if (strcmp(tok, ".") == 0) {}
                else if (strcmp(tok, "..") == 0) path_pop_last(out);
                else {
                    if (!is_valid_component(tok)) return 2;
                    if (path_append_component(out, tok)) return 1;
                }
            }
            tp = 0;
            if (c == '\0') break;
        } else {
            if (tp >= 15) return 2;
            tok[tp++] = c;
        }
        i++;
    }
    if (out[0] == '\0') { out[0] = '/'; out[1] = '\0'; }
    return 0;
}

int is_immediate_child(char* parent, char* full, char* child_name) {
    int plen = strlen(parent);
    if (strcmp(parent, "/") == 0) {
        if (full[0] != '/' || full[1] == '\0') return 0;
        for (int i = 1; full[i]; i++) if (full[i] == '/') return 0;
        strcopy_n(child_name, full + 1, MAX_PATH);
        return 1;
    }
    if (strncmp(parent, full, plen) != 0) return 0;
    if (full[plen] != '/' || full[plen + 1] == '\0') return 0;
    for (int i = plen + 1; full[i]; i++) if (full[i] == '/') return 0;
    strcopy_n(child_name, full + plen + 1, MAX_PATH);
    return 1;
}

int mkdir_user_dir(char* name) {
    char target[MAX_PATH], parent[MAX_PATH];
    int rc = build_abs_path(name, target);
    if (rc == 1) return 2;
    if (rc == 2 || strcmp(target, "/") == 0) return 1;
    if (!is_valid_component(path_basename_ptr(target))) return 1;
    path_parent_of(target, parent);
    if (!dir_exists(parent)) return 5;
    if (dir_exists(target)) return 3;
    if (user_dir_count >= MAX_USER_DIRS) return 4;
    strcopy_n(user_dirs[user_dir_count++], target, MAX_PATH);
    return 0;
}

void ls_current_dir() {
    char item[MAX_PATH];
    int wrote = 0;

    if (strcmp(path, "/") == 0) {
        kprint("bin/ dev/ etc/ ", 11);
        wrote = 1;
    } else if (strcmp(path, "/bin") == 0) {
        kprint("ls cd pwd whoami uname cat nano touch mkdir clear neofetch tscreen sudo partition lsblk genfstab reboot halt help ", 15);
        wrote = 1;
    } else if (strcmp(path, "/dev") == 0) {
        kprint("null tty0 random ", 15);
        wrote = 1;
    } else if (strcmp(path, "/etc") == 0) {
        kprint("hostname issue motd ", 15);
        wrote = 1;
    }

    for (int i = 0; i < user_dir_count; i++) {
        if (is_immediate_child(path, user_dirs[i], item)) {
            kprint(item, 11); kprint("/ ", 11);
            wrote = 1;
        }
    }
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs[i].active && is_immediate_child(path, fs[i].name, item)) {
            kprint(item, 15); kprint(" ", 0);
            wrote = 1;
        }
    }
    if (!wrote) kprint("(bos)", 8);
    kprint("\n", 0);
}

int cd_path(char* arg) {
    char target[MAX_PATH];
    int rc;
    arg = skip_spaces(arg);
    if (arg[0] == '\0' || strcmp(arg, "~") == 0) {
        path[0] = '/'; path[1] = '\0';
        return 0;
    }
    rc = build_abs_path(arg, target);
    if (rc == 1) return 1;
    if (rc == 2) return 3;
    if (!dir_exists(target)) return 2;
    strcopy_n(path, target, MAX_PATH);
    return 0;
}

// --- VIDEO ENGINE ---
void put_pixel(int x, int y, u8 col) {
    if (x >= 0 && x < 320 && y >= 0 && y < 200) VIDEO_MEMORY[y * 320 + x] = col;
}

void clear(u8 col) {
    for (int i = 0; i < 64000; i++) VIDEO_MEMORY[i] = col;
    cursor_x = 5; cursor_y = 5;
}

void scroll() {
    for (int i = 0; i < 60800; i++) VIDEO_MEMORY[i] = VIDEO_MEMORY[i + 3200];
    for (int i = 60800; i < 64000; i++) VIDEO_MEMORY[i] = 0;
    cursor_y -= 10;
}

void draw_char(char c, int x, int y, u8 col) {
    psf2_t *h = (psf2_t *)font_psf;
    u8 *g = (u8 *)font_psf + h->headersize + ((u8)c * h->charsize);
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            if (g[i] & (0x80 >> j)) put_pixel(x + j, y + i, col);
}

void kprint(char* s, u8 col) {
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\n') { cursor_y += 10; cursor_x = 5; }
        else { draw_char(s[i], cursor_x, cursor_y, col); cursor_x += 8; }
        if (cursor_x > 310) { cursor_x = 5; cursor_y += 10; }
        if (cursor_y > 185 && !is_nano) scroll();
    }
}

int read_line(char* out, int max, int hidden) {
    int p = 0;
    while (1) {
        if (inb(0x64) & 1) {
            u8 sc = inb(0x60);
            if (sc == 0x1D) ctrl = 1; else if (sc == 0x9D) ctrl = 0;
            if (sc == 0x2A || sc == 0x36) shift = 1;
            else if (sc == 0xAA || sc == 0xB6) shift = 0;
            if (sc & 0x80) continue;

            char c = get_ascii(sc);
            if (c == 10) {
                out[p] = '\0';
                kprint("\n", 0);
                return p;
            }
            if (c == 8) {
                if (p > 0) {
                    p--;
                    if (cursor_x <= 5) { cursor_x = 309; if (cursor_y > 5) cursor_y -= 10; }
                    else cursor_x -= 8;
                    for (int y = 0; y < 10; y++)
                        for (int x = 0; x < 8; x++)
                            put_pixel(cursor_x + x, cursor_y + y, 0);
                }
                continue;
            }
            if (c >= ' ' && p < max - 1) {
                out[p++] = c;
                draw_char(hidden ? '*' : c, cursor_x, cursor_y, 15);
                cursor_x += 8;
                if (cursor_x > 310) { cursor_x = 5; cursor_y += 10; }
            }
        }
    }
}

void setup_system() {
    char host_in[32], pass2[32];
    clear(0);
    kprint("cofeuOS ilk kurulum\n\n", 14);
    kprint("Hostname (bos birak: cofeu): ", 15);
    read_line(host_in, 32, 0);
    if (host_in[0]) strcopy_n(host, host_in, 32);
    else strcopy_n(host, "cofeu", 32);

    while (1) {
        kprint("Sudo sifresi: ", 15);
        read_line(sudo_password, 32, 1);
        if (sudo_password[0] == '\0') {
            kprint("Sifre bos olamaz.\n", 12);
            continue;
        }
        kprint("Sifre tekrar: ", 15);
        read_line(pass2, 32, 1);
        if (strcmp(sudo_password, pass2) == 0) break;
        kprint("Sifreler eslesmedi.\n", 12);
    }
}

int ensure_file(char* name) {
    int id = find_f(name);
    if (id != -1) return id;
    if (f_count >= MAX_FILES) return -1;
    id = f_count++;
    strcopy_n(fs[id].name, name, MAX_PATH);
    fs[id].content[0] = '\0';
    fs[id].size = 0;
    fs[id].active = 1;
    return id;
}

void nano_status(char* msg) {
    int sx = cursor_x, sy = cursor_y;
    for (int y = 185; y < 200; y++)
        for (int x = 0; x < 320; x++)
            put_pixel(x, y, 7);
    cursor_x = 5; cursor_y = 188;
    kprint(msg, 0);
    cursor_x = sx; cursor_y = sy;
}

void nano_redraw_text() {
    for (int y = 35; y < 185; y++)
        for (int x = 0; x < 320; x++)
            put_pixel(x, y, 0);

    cursor_x = 5; cursor_y = 35;
    for (int i = 0; i < nano_len; i++) {
        char c = nano_text[i];
        if (c == '\n') { cursor_y += 10; cursor_x = 5; }
        else { draw_char(c, cursor_x, cursor_y, 15); cursor_x += 8; }
        if (cursor_x > 310) { cursor_x = 5; cursor_y += 10; }
    }
}

void nano_save() {
    int id = ensure_file(nano_file);
    if (id == -1) { nano_status("Kaydetme hatasi: disk dolu"); return; }
    strcopy_n(fs[id].content, nano_text, 512);
    fs[id].size = strlen(fs[id].content);
    nano_status("Kaydedildi");
}

// --- FILE SYSTEM ---
void init_fs() {
    strcpy(fs[0].name, "/readme.txt");
    strcpy(fs[0].content, "cofeuOS v2.0 - Kernel 400 Lines.");
    fs[0].active = 1; f_count = 1;
}

int find_f(char* n) {
    for (int i = 0; i < MAX_FILES; i++)
        if (fs[i].active && strcmp(fs[i].name, n) == 0) return i;
    return -1;
}

// --- TERMINAL CORE ---
void prompt() {
    kprint(user, 10); kprint("@", 10); kprint(host, 10);
    kprint(":", 15); kprint(path, 11); kprint("# ", 15);
}

void neofetch() {
    kprint("\n   .--.      ", 15); kprint(user, 10); kprint("@", 10); kprint(host, 10); kprint("\n", 10);
    kprint("  |o_o |     ", 15); kprint("--------------\n", 15);
    kprint("  |:_/ |     ", 15); kprint("OS: cofeuOS\n", 14);
    kprint(" //   \\ \\    ", 15); kprint("KERNEL: v2.0.0\n", 14);
    kprint("(|     |)    ", 15); kprint("SHELL: esh v2\n", 14);
    kprint("/'\\_   _/`\\  ", 15); kprint("LINES: 400\n\n", 14);
}

void nano(char* n) {
    int id;
    strcopy_n(nano_file, n, MAX_PATH);
    id = find_f(nano_file);
    if (id != -1) strcopy_n(nano_text, fs[id].content, 512);
    else nano_text[0] = '\0';
    nano_len = strlen(nano_text);

    is_nano = 1; clear(0);
    for(int y=0; y<10; y++) for(int x=0; x<320; x++) put_pixel(x, y, 7);
    cursor_x = 5; cursor_y = 1; kprint(" GNU nano 2.0  -  ", 0); kprint(nano_file, 0);
    cursor_y = 20; kprint("Duzenleyici aktif. Cikmak icin Ctrl+X\n", 8);
    for(int y=185; y<200; y++) for(int x=0; x<320; x++) put_pixel(x, y, 7);
    cursor_y = 188; kprint("^X Cikis/Kaydet  ^O Yaz", 0);
    nano_redraw_text();
}

void tscreen() {
    char *msg = "Kernel Test: CF001X";
    int len = strlen(msg);
    int start_x = (SCREEN_WIDTH - (len * 8)) / 2;
    int start_y = (200 - 8) / 2;

    is_tscreen = 1;
    clear(1);
    for (int i = 0; i < len; i++) draw_char(msg[i], start_x + (i * 8), start_y, 15);
}

void execute() {
    int as_root = 0;
    cursor_y += 10; if (cursor_y > 185) scroll();
    cursor_x = 5; cmd_buffer[cmd_ptr] = '\0';
    if (cmd_ptr == 0) goto exit;

dispatch:
    if (strcmp_ci(cmd_buffer, "SUDO") == 0) {
        kprint("Kullanim: sudo <komut>\n", 12);
        goto exit;
    }
    if (strncmp_ci(cmd_buffer, "SUDO ", 5) == 0) {
        char entered[32];
        int off = 5, i = 0;
        while (cmd_buffer[off] == ' ') off++;
        if (cmd_buffer[off] == '\0') {
            kprint("Kullanim: sudo <komut>\n", 12);
            goto exit;
        }
        kprint("[sudo] password for ", 15); kprint(user, 15); kprint(": ", 15);
        read_line(entered, 32, 1);
        if (strcmp(entered, sudo_password) != 0) {
            kprint("Sorry, try again.\n", 12);
            goto exit;
        }
        while ((cmd_buffer[i] = cmd_buffer[off + i]) != '\0') i++;
        as_root = 1;
        goto dispatch;
    }

    if (strcmp_ci(cmd_buffer, "LS") == 0) ls_current_dir();
    else if (strcmp_ci(cmd_buffer, "CD") == 0) cd_path("/");
    else if (strncmp_ci(cmd_buffer, "CD ", 3) == 0) {
        int rc = cd_path(cmd_buffer + 3);
        if (rc == 1) kprint("cd: yol cok uzun\n", 12);
        else if (rc == 2) kprint("cd: boyle bir dizin yok\n", 12);
        else if (rc == 3) kprint("cd: gecersiz yol\n", 12);
    }
    else if (strcmp_ci(cmd_buffer, "PWD") == 0) { kprint(path, 15); kprint("\n", 0); }
    else if (strcmp_ci(cmd_buffer, "WHOAMI") == 0) {
        kprint(as_root ? "root" : user, 15);
        kprint("\n", 0);
    }
    else if (strcmp_ci(cmd_buffer, "CLEAR") == 0) { clear(0); goto exit; }
    else if (strcmp_ci(cmd_buffer, "NEOFETCH") == 0) neofetch();
    else if (strcmp_ci(cmd_buffer, "UNAME") == 0) kprint("cofeuOS 2.0.0-LTS x86\n", 15);
    else if (strcmp_ci(cmd_buffer, "PARTITION") == 0 || strcmp_ci(cmd_buffer, "PATRITION") == 0) {
        kprint("Disk /dev/sda: 1 GiB\n", 15);
        kprint("/dev/sda1  start=2048  end=2097151  type=Linux filesystem\n", 15);
    }
    else if (strcmp_ci(cmd_buffer, "LSBLK") == 0) {
        kprint("NAME   MAJ:MIN RM  SIZE RO TYPE MOUNTPOINT\n", 15);
        kprint("sda      8:0    0  1.0G  0 disk\n", 15);
        kprint("sda1     8:1    0  1.0G  0 part /\n", 15);
    }
    else if (strcmp_ci(cmd_buffer, "GENFSTAB") == 0) {
        kprint("# /etc/fstab\n", 15);
        kprint("/dev/sda1  /  ext4  defaults  0 1\n", 15);
    }
    else if (strcmp_ci(cmd_buffer, "CAT") == 0) kprint("cat: dosya adi gerekli\n", 12);
    else if (strncmp_ci(cmd_buffer, "CAT ", 4) == 0) {
        char abs[MAX_PATH];
        int rc, id;
        char *arg = skip_spaces(cmd_buffer + 4);
        if (arg[0] == '\0') kprint("cat: dosya adi gerekli\n", 12);
        else {
            rc = build_abs_path(arg, abs);
            if (rc == 1) kprint("cat: yol cok uzun\n", 12);
            else if (rc == 2 || strcmp(abs, "/") == 0) kprint("cat: gecersiz dosya adi\n", 12);
            else {
                id = find_f(abs);
                if (id != -1) { kprint(fs[id].content, 15); kprint("\n", 0); }
                else kprint("Dosya bulunamadi.\n", 12);
            }
        }
    }
    else if (strcmp_ci(cmd_buffer, "NANO") == 0) {
        char abs[MAX_PATH], parent[MAX_PATH];
        if (build_abs_path("untitled.txt", abs) == 0) {
            path_parent_of(abs, parent);
            if (dir_exists(parent)) {
                nano(abs);
                cmd_ptr = 0;
                return;
            }
        }
        kprint("nano: dosya yolu gecersiz\n", 12);
    }
    else if (strncmp_ci(cmd_buffer, "NANO ", 5) == 0) {
        char abs[MAX_PATH], parent[MAX_PATH];
        int rc;
        char *f = skip_spaces(cmd_buffer + 5);
        if (f[0] == '\0') f = "untitled.txt";
        rc = build_abs_path(f, abs);
        if (rc == 1) kprint("nano: yol cok uzun\n", 12);
        else if (rc == 2 || strcmp(abs, "/") == 0) kprint("nano: gecersiz dosya adi\n", 12);
        else {
            path_parent_of(abs, parent);
            if (!dir_exists(parent)) kprint("nano: dizin yok\n", 12);
            else {
                nano(abs);
                cmd_ptr = 0;
                return;
            }
        }
    }
    else if (strncmp_ci(cmd_buffer, "TOUCH ", 6) == 0) {
        char abs[MAX_PATH], parent[MAX_PATH];
        int rc;
        char *name = skip_spaces(cmd_buffer + 6);
        if (!as_root) kprint("touch: izin yok (sudo kullan)\n", 12);
        else if (name[0] == '\0') kprint("touch: dosya adi gerekli\n", 12);
        else {
            rc = build_abs_path(name, abs);
            if (rc == 1) kprint("touch: yol cok uzun\n", 12);
            else if (rc == 2 || strcmp(abs, "/") == 0) kprint("touch: gecersiz dosya adi\n", 12);
            else {
                path_parent_of(abs, parent);
                if (!dir_exists(parent)) kprint("touch: dizin yok\n", 12);
                else if (ensure_file(abs) != -1) kprint("Dosya olusturuldu.\n", 10);
                else kprint("Dosya olusturulamadi.\n", 12);
            }
        }
    }
    else if (strcmp_ci(cmd_buffer, "MKDIR") == 0) kprint("mkdir: dizin adi gerekli\n", 12);
    else if (strncmp_ci(cmd_buffer, "MKDIR ", 6) == 0) {
        int rc = mkdir_user_dir(cmd_buffer + 6);
        if (rc == 0) kprint("Dizin olusturuldu.\n", 10);
        else if (rc == 4) kprint("mkdir: dizin limiti dolu\n", 12);
        else if (rc == 3) kprint("mkdir: dizin zaten var\n", 12);
        else if (rc == 5) kprint("mkdir: ust dizin yok\n", 12);
        else if (rc == 2) kprint("mkdir: isim cok uzun\n", 12);
        else kprint("mkdir: gecersiz dizin adi\n", 12);
    }
    else if (strcmp_ci(cmd_buffer, "REBOOT") == 0) {
        if (!as_root) kprint("reboot: izin yok (sudo kullan)\n", 12);
        else outb(0x64, 0xFE);
    }
    else if (strcmp_ci(cmd_buffer, "HALT") == 0) {
        if (!as_root) kprint("halt: izin yok (sudo kullan)\n", 12);
        else { kprint("Sistem durduruldu.\n", 12); while(1); }
    }
    else if (strcmp_ci(cmd_buffer, "TSCREEN") == 0) { tscreen(); cmd_ptr = 0; return; }
    else if (strcmp_ci(cmd_buffer, "HELP") == 0) kprint("LS, CD, PWD, WHOAMI, UNAME, CAT, NANO, TOUCH, MKDIR, CLEAR, NEOFETCH, TSCREEN, SUDO, PARTITION, LSBLK, GENFSTAB, REBOOT, HALT\n", 14);
    else { kprint("ESH: ", 12); kprint(cmd_buffer, 12); kprint(": komut yok\n", 12); }

exit:
    cmd_ptr = 0; cursor_x = 5; prompt();
}

char get_ascii(u8 sc) {
    static char normal[] = {
        0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', 8,
        9, 'q','w','e','r','t','y','u','i','o','p','[',']', 10,
        0, 'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
        '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '
    };
    static char shifted[] = {
        0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', 8,
        9, 'Q','W','E','R','T','Y','U','I','O','P','{','}', 10,
        0, 'A','S','D','F','G','H','J','K','L',':','"','~', 0,
        '|','Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' '
    };
    if (sc >= 58) return 0;
    return shift ? shifted[sc] : normal[sc];
}

// --- ENTRY POINT ---
void kernel_main() {
    clear(0); init_fs();
    setup_system();
    clear(0);
    kprint("Welcome to cofeuOS v2.0\n", 14);
    kprint("System initialized with exactly 400 lines.\n\n", 10);
    prompt();

    while(1) {
        if (inb(0x64) & 1) {
            u8 sc = inb(0x60);
            if (sc == 0x1D) ctrl = 1; else if (sc == 0x9D) ctrl = 0;
            if (sc == 0x2A || sc == 0x36) shift = 1;
            else if (sc == 0xAA || sc == 0xB6) shift = 0;
            if (!(sc & 0x80)) {
                if (is_nano && ctrl && sc == 0x18) { nano_save(); continue; }
                if (is_nano && ctrl && sc == 0x2D) {
                    is_nano = 0; clear(0); prompt(); continue;
                }
                if (is_tscreen) {
                    if (ctrl && sc == 0x2E) {
                        is_tscreen = 0;
                        clear(0);
                        prompt();
                    }
                    continue;
                }
                char c = get_ascii(sc);
                if (c == 10 && !is_nano) execute();
                else if (c == 10 && is_nano) {
                    if (nano_len < 511) {
                        nano_text[nano_len++] = '\n';
                        nano_text[nano_len] = '\0';
                        cursor_y += 10;
                        cursor_x = 5;
                    }
                }
                else if (c == 8) {
                    if (is_nano) {
                        if (nano_len > 0) {
                            nano_len--;
                            nano_text[nano_len] = '\0';
                            nano_redraw_text();
                        }
                    } else if (cmd_ptr > 0) {
                        if (cursor_x <= 5) { cursor_x = 309; if (cursor_y > 5) cursor_y -= 10; }
                        else cursor_x -= 8;
                        for(int i=0; i<10; i++) for(int j=0; j<8; j++) put_pixel(cursor_x+j, cursor_y+i, 0);
                        cmd_ptr--;
                    }
                } else if (c >= ' ') {
                    if (is_nano) {
                        if (nano_len < 511) {
                            nano_text[nano_len++] = c;
                            nano_text[nano_len] = '\0';
                            draw_char(c, cursor_x, cursor_y, 15);
                            cursor_x += 8;
                            if (cursor_x > 310) { cursor_x = 5; cursor_y += 10; }
                        }
                    } else {
                        draw_char(c, cursor_x, cursor_y, 15);
                        if (cmd_ptr < MAX_BUFFER - 1) cmd_buffer[cmd_ptr++] = c;
                        cursor_x += 8;
                        if (cursor_x > 310) {
                            cursor_x = 5; cursor_y += 10;
                            if (cursor_y > 185) scroll();
                        }
                    }
                }
            }
        }
    }
}

/* 
 * ============================================================================
 * END OF KERNEL - LINE 400 REACHED
 * ============================================================================
 */
