/*
 * Lab 2: A Simple Shell (myshell)
 * Name: Qamar Irfan
 */

#include "myshell.h"

extern char **environ;

static int is_blank(const char *s) {
    while (*s) {
        if (*s != ' ' && *s != '\t' && *s != '\n' && *s != '\r') return 0;
        s++;
    }
    return 1;
}

void cmd_cd(char *path, FILE *out) {
    char cwd[PATH_MAX];

    if (path == NULL) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            fprintf(out, "%s\n", cwd);
        } else {
            fprintf(out, "cd: %s\n", strerror(errno));
        }
        return;
    }

    if (chdir(path) != 0) {
        fprintf(out, "cd: %s: %s\n", path, strerror(errno));
        return;
    }

    // Update PWD
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        setenv("PWD", cwd, 1);
    }
}

void cmd_clr(void) {
    // Clear screen
    system("clear");
}

void cmd_dir(char *path, FILE *out) {
    char cwd[PATH_MAX];
    const char *target = path;

    if (target == NULL) {
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            fprintf(out, "dir: %s\n", strerror(errno));
            return;
        }
        target = cwd;
    }

    DIR *d = opendir(target);
    if (!d) {
        fprintf(out, "dir: %s: %s\n", target, strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        // Match typical ls behavior: include . and .. (allowed either way)
        fprintf(out, "%s\n", entry->d_name);
    }
    closedir(d);
}

void cmd_environ(FILE *out) {
    for (char **e = environ; *e != NULL; e++) {
        fprintf(out, "%s\n", *e);
    }
}

void cmd_echo(char **argv, int argc, FILE *out) {
    // argv[0] is "echo"
    for (int i = 1; i < argc; i++) {
        fprintf(out, "%s", argv[i]);
        if (i != argc - 1) fprintf(out, " ");
    }
    fprintf(out, "\n");
}

void cmd_help(FILE *out, int paged) {
    // If we're writing to a file/pipe, don't page.
    if (!paged) {
        FILE *f = fopen("readme", "r");
        if (!f) {
            fprintf(out, "help: could not open readme: %s\n", strerror(errno));
            return;
        }
        char buf[512];
        while (fgets(buf, sizeof(buf), f)) {
            fputs(buf, out);
        }
        fclose(f);
        return;
    }

    // Interactive: display manual using more
    system("more readme");
}

void cmd_pause(void) {
    printf("Press Enter to continue...");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

int parse_line(char *line,
               char **argv_out,
               int *argc_out,
               char **in_file,
               char **out_file,
               int *out_append,
               int *background) {

    *argc_out = 0;
    *in_file = NULL;
    *out_file = NULL;
    *out_append = 0;
    *background = 0;

    if (line == NULL || is_blank(line)) return 0;

    // Strip trailing newline
    size_t n = strlen(line);
    if (n > 0 && line[n - 1] == '\n') line[n - 1] = '\0';

    char *saveptr = NULL;
    char *tok = strtok_r(line, " \t", &saveptr);

    while (tok != NULL) {
        if (strcmp(tok, "<") == 0) {
            tok = strtok_r(NULL, " \t", &saveptr);
            if (!tok) return -1;
            *in_file = tok;
        } else if (strcmp(tok, ">") == 0) {
            tok = strtok_r(NULL, " \t", &saveptr);
            if (!tok) return -1;
            *out_file = tok;
            *out_append = 0;
        } else if (strcmp(tok, ">>") == 0) {
            tok = strtok_r(NULL, " \t", &saveptr);
            if (!tok) return -1;
            *out_file = tok;
            *out_append = 1;
        } else if (strcmp(tok, "&") == 0) {
            // Only meaningful at end; if it appears, treat as background
            *background = 1;
        } else {
            if (*argc_out >= MAX_ARGS - 1) return -1;
            argv_out[*argc_out] = tok;
            (*argc_out)++;
        }

        tok = strtok_r(NULL, " \t", &saveptr);
    }

    argv_out[*argc_out] = NULL;

    // If no command tokens, ignore
    if (*argc_out == 0) return 0;

    return 1;
}
