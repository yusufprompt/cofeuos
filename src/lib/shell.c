/*
 * SHELL.C - cofeuOS Unix-Like Shell (Build Fixed)
 */

#include "../include/shell.h"
#include "../include/video.h"
#include "../include/fs.h"
#include "../include/string.h"
#include "../include/io.h"
#include "../include/types.h"

extern int cursor_x, cursor_y;
extern shell_control g_shell;
extern fs_control_block g_fs;

int main_get_input(char* buffer, int max_len);

/* Prototypes */
static void shell_print(const char* str, u8 color);
static void shell_newline(void);
static int shell_tokenize(const char* cmd, char** args, int max_args);
static int cmd_ls(int argc, char** argv);
static int cmd_cat(int argc, char** argv);
static int cmd_pwd(int argc, char** argv);
static int cmd_cd(int argc, char** argv);
static int cmd_whoami(int argc, char** argv);
static int cmd_uname(int argc, char** argv);
static int cmd_clear(int argc, char** argv);
static int cmd_neofetch(int argc, char** argv);
static int cmd_vim(int argc, char** argv);
static int cmd_ifconfig(int argc, char** argv);
static int cmd_ping(int argc, char** argv);
static int cmd_reboot(int argc, char** argv);
static int cmd_halt(int argc, char** argv);
static int cmd_touch(int argc, char** argv);
static int cmd_mkdir(int argc, char** argv);
static int cmd_rm(int argc, char** argv);
static int cmd_rmdir(int argc, char** argv);
static int cmd_nano(int argc, char** argv);
static int cmd_help(int argc, char** argv);
static int cmd_date(int argc, char** argv);
static int cmd_uptime(int argc, char** argv);
static int cmd_free(int argc, char** argv);
static int cmd_ps(int argc, char** argv);
static int cmd_df(int argc, char** argv);
static int cmd_echo(int argc, char** argv);
static int cmd_env(int argc, char** argv);
static int cmd_rodo(int argc, char** argv);
static int cmd_sudo(int argc, char** argv);
static int cmd_pacman(int argc, char** argv);

int shell_execute(const char* cmd);

static void shell_print(const char* str, u8 color) {
    int x = cursor_x;
    int y = cursor_y;

    for (const char* p = str; *p; p++) {
        if (*p == '\n') {
            x = 5;
            y += LINE_HEIGHT;
            if (y > SCREEN_HEIGHT - LINE_HEIGHT) {
                video_scroll();
                y = SCREEN_HEIGHT - LINE_HEIGHT;
            }
            continue;
        }

        video_draw_char(*p, x, y, color);
        x += CHAR_WIDTH;
        if (x > SCREEN_WIDTH - CHAR_WIDTH) {
            x = 5;
            y += LINE_HEIGHT;
            if (y > SCREEN_HEIGHT - CHAR_HEIGHT) {
                video_scroll();
                y = SCREEN_HEIGHT - CHAR_HEIGHT;
            }
        }
    }

    cursor_x = x;
    cursor_y = y;
}

static void shell_newline(void) {
    cursor_x = 5;
    cursor_y += LINE_HEIGHT;
    if (cursor_y > SCREEN_HEIGHT - LINE_HEIGHT) {
        video_scroll();
        cursor_y = SCREEN_HEIGHT - LINE_HEIGHT;
    }
}

static int shell_tokenize(const char* cmd, char** args, int max_args) {
    char buf[512];
    strcpy(buf, cmd);
    int i = 0;
    char* token = strtok(buf, " \t\n");
    while (token && i < max_args - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    return i;
}

int shell_execute(const char* cmd) {
    char* args[16];
    int argc = shell_tokenize(cmd, args, 16);
    if (!argc) return 0;

    if (strcmp(args[0], "help") == 0) return cmd_help(argc, args);
    if (strcmp(args[0], "ls") == 0) return cmd_ls(argc, args);
    if (strcmp(args[0], "cat") == 0) return cmd_cat(argc, args);
    if (strcmp(args[0], "pwd") == 0) return cmd_pwd(argc, args);
    if (strcmp(args[0], "cd") == 0) return cmd_cd(argc, args);
    if (strcmp(args[0], "whoami") == 0) return cmd_whoami(argc, args);
    if (strcmp(args[0], "uname") == 0) return cmd_uname(argc, args);
    if (strcmp(args[0], "clear") == 0) return cmd_clear(argc, args);
    if (strcmp(args[0], "neofetch") == 0) return cmd_neofetch(argc, args);
    if (strcmp(args[0], "reboot") == 0) return cmd_reboot(argc, args);
    if (strcmp(args[0], "halt") == 0) return cmd_halt(argc, args);
    if (strcmp(args[0], "touch") == 0) return cmd_touch(argc, args);
    if (strcmp(args[0], "mkdir") == 0) return cmd_mkdir(argc, args);
    if (strcmp(args[0], "rm") == 0) return cmd_rm(argc, args);
    if (strcmp(args[0], "rmdir") == 0) return cmd_rmdir(argc, args);
    if (strcmp(args[0], "nano") == 0) return cmd_nano(argc, args);
    if (strcmp(args[0], "vim") == 0) return cmd_vim(argc, args);
    if (strcmp(args[0], "rodo") == 0) return cmd_rodo(argc, args);
    if (strcmp(args[0], "sudo") == 0) return cmd_sudo(argc, args);
    if (strcmp(args[0], "pacman") == 0) return cmd_pacman(argc, args);
    if (strcmp(args[0], "ifconfig") == 0) return cmd_ifconfig(argc, args);
    if (strcmp(args[0], "ping") == 0) return cmd_ping(argc, args);
    if (strcmp(args[0], "date") == 0) return cmd_date(argc, args);
    if (strcmp(args[0], "uptime") == 0) return cmd_uptime(argc, args);
    if (strcmp(args[0], "free") == 0) return cmd_free(argc, args);
    if (strcmp(args[0], "ps") == 0) return cmd_ps(argc, args);
    if (strcmp(args[0], "df") == 0) return cmd_df(argc, args);
    if (strcmp(args[0], "echo") == 0) return cmd_echo(argc, args);
    if (strcmp(args[0], "env") == 0) return cmd_env(argc, args);
    
    shell_print("cofeuOS: '", 12);
    shell_print(args[0], 12);
    shell_print("': unknown command", 12);
    shell_newline();
    return -1;
}

static int cmd_help(int argc, char** argv) {
    shell_print("cofeuOS Unix Shell Commands:\n", 14);
    shell_newline();
    shell_print("File: ls cat pwd cd touch mkdir rm rmdir\n", 15);
    shell_newline();
    shell_print("System: whoami uname clear date uptime free ps df echo env\n", 15);
    shell_newline();
    shell_print("Other: neofetch nano vim rodo reboot halt\n", 15);
    shell_newline();
    shell_print("Package: pacman\n", 15);
    shell_newline();
    shell_print("Network: ifconfig ping\n", 15);
    shell_newline();
    return 0;
}

static int cmd_ls(int argc, char** argv) {
    char buf[1024];
    const char* p = argc > 1 ? argv[1] : g_shell.cwd;
    int sz = fs_list_dir(&g_fs, p, buf, 1024);
    if (sz < 0) {
        shell_print("ls: no such dir: ", 12);
        shell_print(p, 12);
        shell_newline();
        return -1;
    }
    shell_print(buf[0] ? buf : "(empty)", 11);
    shell_newline();
    return 0;
}

static int cmd_cat(int argc, char** argv) {
    if (argc < 2) {
        shell_print("cat: missing file", 12);
        shell_newline();
        return -1;
    }
    char res[256], buf[4096];
    fs_resolve_path(g_shell.cwd, argv[1], res);
    int sz = fs_read_file(&g_fs, res, buf, 4096);
    if (sz < 0) {
        shell_print("cat: ", 12);
        shell_print(argv[1], 12);
        shell_print(": no such file", 12);
        shell_newline();
        return -1;
    }
    buf[sz] = 0;
    shell_print(buf, 15);
    shell_newline();
    return 0;
}

static int cmd_pwd(int argc, char** argv) {
    shell_print(g_shell.cwd, 11);
    shell_newline();
    return 0;
}

static int cmd_cd(int argc, char** argv) {
    const char* p = argc > 1 ? argv[1] : "/";
    char res[256];
    fs_resolve_path(g_shell.cwd, p, res);
    if (!fs_dir_exists(&g_fs, res)) {
        shell_print("cd: no such directory: ", 12);
        shell_print(p, 12);
        shell_newline();
        return -1;
    }
    strcpy(g_shell.cwd, res);
    return 0;
}

static int cmd_whoami(int argc, char** argv) {
    shell_print(g_shell.user, 10);
    shell_newline();
    return 0;
}

static int cmd_uname(int argc, char** argv) {
    shell_print("cofeuOS v3.0 x86_32", 14);
    shell_newline();
    return 0;
}

static int cmd_clear(int argc, char** argv) {
    video_clear(0);
    cursor_x = 5;
    cursor_y = 30;
    return 0;
}

static int cmd_neofetch(int argc, char** argv) {
    shell_print("    .-\"-.     ", 15); shell_newline();
    shell_print("   / ..  \\    ", 15); shell_print("OS: cofeuOS v3.0", 11); shell_newline();
    shell_print("  | (  )  |   ", 15); shell_print("Kernel: x86_32", 11); shell_newline();
    shell_print("   \\ ..  /    ", 15); shell_print("Shell: Cofeu Shell", 11); shell_newline();
    shell_print("    `---'      ", 15); shell_print("Host: cofeu", 11); shell_newline();
    shell_print("Resolution: 320x200", 11); shell_newline();
    shell_print("Disk: /dev/sda1", 11); shell_newline();
    shell_print("Memory: 16MB", 11); shell_newline();
    shell_print("Uptime: 0 days", 11); shell_newline();
    shell_print("Network: no NIC driver installed", 12); shell_newline();
    shell_print("Note: Ethernet stack requires kernel driver support.", 12); shell_newline();
    return 0;
}

static int cmd_text_editor(int argc, char** argv, const char* editor_name) {
    shell_print("========================================", 14);
    shell_newline();
    shell_print(editor_name, 14);
    shell_print(" editor - :w = save, :q = quit, :wq = save+quit", 14);
    shell_newline();

    char path[MAX_PATH_LEN] = "";
    if (argc > 1) {
        fs_resolve_path(g_shell.cwd, argv[1], path);
    }
    shell_print("File: ", 7);
    shell_print(path[0] ? path : "<no file>", 15);
    shell_newline();
    shell_print("========================================", 14);
    shell_newline();

    char buffer[FILE_CONTENT_SIZE];
    int len = 0;
    if (path[0]) {
        int sz = fs_read_file(&g_fs, path, buffer, sizeof(buffer) - 1);
        if (sz >= 0) {
            buffer[sz] = '\0';
            len = sz;
            shell_print("Loaded file content. Append lines and save with :wq.", 15);
            shell_newline();
        } else {
            buffer[0] = '\0';
            shell_print("New file will be created. Use :wq to save.", 15);
            shell_newline();
        }
    } else {
        buffer[0] = '\0';
        shell_print("No file specified. Use :q to exit.", 15);
        shell_newline();
    }

    char line[80];
    while (1) {
        cursor_x = 5;
        video_print("> ", 7, cursor_y, 7);
        cursor_x = 25;
        main_get_input(line, 80);

        if (line[0] == ':' && strcmp(line, ":q") == 0) {
            break;
        }
        if (line[0] == ':' && strcmp(line, ":wq") == 0) {
            if (path[0]) {
                fs_write_file(&g_fs, path, buffer, len);
                shell_print("Saved file.", 10);
                shell_newline();
            } else {
                shell_print("No filename given. Use nano <file> or vim <file>.", 12);
                shell_newline();
            }
            break;
        }
        if (line[0] == ':' && strcmp(line, ":w") == 0) {
            if (path[0]) {
                fs_write_file(&g_fs, path, buffer, len);
                shell_print("Saved file.", 10);
                shell_newline();
            } else {
                shell_print("No filename given. Use nano <file> or vim <file>.", 12);
                shell_newline();
            }
            continue;
        }

        int line_len = strlen(line);
        if (len + line_len + 1 < (int)sizeof(buffer)) {
            memcpy(buffer + len, line, line_len);
            len += line_len;
            buffer[len++] = '\n';
            buffer[len] = '\0';
            shell_print(line, 15);
            shell_newline();
        } else {
            shell_print("Editor buffer full.", 12);
            shell_newline();
            break;
        }
    }

    shell_print(editor_name, 14);
    shell_print(" exited.", 14);
    shell_newline();
    return 0;
}

static int cmd_vim(int argc, char** argv) {
    return cmd_text_editor(argc, argv, "vim");
}

static int cmd_reboot(int argc, char** argv) {
    shell_print("Rebooting cofeuOS...", 14);
    shell_newline();
    outb(0x64, 0xFE);
    while(1);
    return 0;
}

static int cmd_halt(int argc, char** argv) {
    shell_print("cofeuOS halted.", 12);
    shell_newline();
    while(1);
    return 0;
}

static int cmd_ifconfig(int argc, char** argv) {
    shell_print("eth0: no hardware driver installed", 12);
    shell_newline();
    shell_print("IP: 0.0.0.0", 12);
    shell_newline();
    shell_print("Netmask: 255.255.255.0", 12);
    shell_newline();
    shell_print("Gateway: 0.0.0.0", 12);
    shell_newline();
    shell_print("Note: Ethernet support requires NIC driver and TCP/IP stack.", 12);
    shell_newline();
    return 0;
}

static int cmd_ping(int argc, char** argv) {
    if (argc < 2) {
        shell_print("ping: hostname required", 12);
        shell_newline();
        return -1;
    }
    shell_print("ping: network stack not implemented yet", 12);
    shell_newline();
    return -1;
}

static int cmd_touch(int argc, char** argv) {
    if (argc < 2) {
        shell_print("touch: filename required", 12);
        shell_newline();
        return -1;
    }
    char res[256];
    fs_resolve_path(g_shell.cwd, argv[1], res);
    fs_create_file(&g_fs, res, "", 0);
    shell_print("Created file: ", 10);
    shell_print(argv[1], 10);
    shell_newline();
    return 0;
}

static int cmd_mkdir(int argc, char** argv) {
    if (argc < 2) {
        shell_print("mkdir: dirname required", 12);
        shell_newline();
        return -1;
    }
    char res[256];
    fs_resolve_path(g_shell.cwd, argv[1], res);
    fs_create_dir(&g_fs, res);
    shell_print("Created directory: ", 10);
    shell_print(argv[1], 10);
    shell_newline();
    return 0;
}

static int cmd_rm(int argc, char** argv) {
    if (argc < 2) {
        shell_print("rm: filename required", 12);
        shell_newline();
        return -1;
    }
    char res[256];
    fs_resolve_path(g_shell.cwd, argv[1], res);
    fs_delete_file(&g_fs, res);
    shell_print("Removed: ", 10);
    shell_print(argv[1], 10);
    shell_newline();
    return 0;
}

static int cmd_rodo(int argc, char** argv) {
    if (argc < 2) {
        shell_print("rodo: command required", 12);
        shell_newline();
        return -1;
    }

    shell_print("rodo: running as root", 10);
    shell_newline();

    char command[512];
    int pos = 0;
    for (int i = 1; i < argc; i++) {
        int len = strlen(argv[i]);
        if (pos + len + 2 >= (int)sizeof(command)) break;
        strcpy(&command[pos], argv[i]);
        pos += len;
        if (i < argc - 1) {
            command[pos++] = ' ';
        }
    }
    command[pos] = '\0';
    return shell_execute(command);
}

static int cmd_sudo(int argc, char** argv) {
    return cmd_rodo(argc, argv);
}

static int cmd_pacman(int argc, char** argv) {
    if (argc < 2) {
        shell_print("pacman: usage: pacman -S <pkg> | -Ss <pkg> | -Sy | -Syu", 12);
        shell_newline();
        return -1;
    }

    if (strcmp(argv[1], "-S") == 0) {
        if (argc < 3) {
            shell_print("pacman: package name required", 12);
            shell_newline();
            return -1;
        }
        shell_print("installing package: ", 10);
        shell_print(argv[2], 10);
        shell_newline();
        return 0;
    }

    if (strcmp(argv[1], "-Ss") == 0) {
        if (argc < 3) {
            shell_print("pacman: package search required", 12);
            shell_newline();
            return -1;
        }
        shell_print("search results for: ", 11);
        shell_print(argv[2], 11);
        shell_newline();
        shell_print("community/" , 10); shell_print(argv[2], 10); shell_newline();
        return 0;
    }

    if (strcmp(argv[1], "-Sy") == 0) {
        shell_print("syncing package databases... done", 10);
        shell_newline();
        return 0;
    }

    if (strcmp(argv[1], "-Syu") == 0) {
        shell_print("synchronizing package databases and upgrading... done", 10);
        shell_newline();
        return 0;
    }

    shell_print("pacman: unsupported operation", 12);
    shell_newline();
    return -1;
}

static int cmd_rmdir(int argc, char** argv) {
    if (argc < 2) {
        shell_print("rmdir: dirname required", 12);
        shell_newline();
        return -1;
    }
    char res[256];
    fs_resolve_path(g_shell.cwd, argv[1], res);
    fs_delete_dir(&g_fs, res);
    shell_print("Removed directory: ", 10);
    shell_print(argv[1], 10);
    shell_newline();
    return 0;
}

static int cmd_nano(int argc, char** argv) {
    return cmd_text_editor(argc, argv, "nano");
}

static int cmd_date(int argc, char** argv) { shell_print("Thu Jan 1 00:00:00 2025", 14); shell_newline(); return 0; }
static int cmd_uptime(int argc, char** argv) { shell_print("uptime 0 days", 14); shell_newline(); return 0; }
static int cmd_free(int argc, char** argv) { shell_print("free: 16MB total", 14); shell_newline(); return 0; }
static int cmd_ps(int argc, char** argv) { shell_print("PID 1 shell", 15); shell_newline(); return 0; }
static int cmd_df(int argc, char** argv) { shell_print("/dev/sda1 16MB 6% used", 15); shell_newline(); return 0; }
static int cmd_echo(int argc, char** argv) { for (int i = 1; i < argc; i++) shell_print(argv[i], 15); shell_newline(); return 0; }
static int cmd_env(int argc, char** argv) { shell_print("USER=root PATH=/bin", 15); shell_newline(); return 0; }

