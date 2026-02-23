/*
 * SOFE3950U - Operating Systems
 * Lab 2
 * Name: Qamar Irfan

 */

#ifndef MYSHELL_H
#define MYSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

// Max sizes
#define MAX_LINE 1024
#define MAX_ARGS 64

// Internal command helpers
void cmd_cd(char *path, FILE *out);
void cmd_clr(void);
void cmd_dir(char *path, FILE *out);
void cmd_environ(FILE *out);
void cmd_echo(char **argv, int argc, FILE *out);
void cmd_help(FILE *out, int paged);
void cmd_pause(void);

// Parsing helpers
int parse_line(char *line,
               char **argv_out,
               int *argc_out,
               char **in_file,
               char **out_file,
               int *out_append,
               int *background);

#endif
