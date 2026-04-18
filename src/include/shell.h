/*
 * SHELL.H - cofeuOS Shell API
 */

#ifndef _SHELL_H
#define _SHELL_H

#include "types.h"
#include "fs.h"
#include "string.h"

#define MAX_PATH_LEN 256

typedef struct {
    char user[32];
    char host[32];
    char partition[32];
    char cwd[MAX_PATH_LEN];
} shell_control;

extern shell_control g_shell;
extern int shell_execute(const char* cmd);

#endif

