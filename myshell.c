/*
 * SOFE3950U - Operating Systems
 * Lab 2: A Simple Shell (myshell)
 *
 * Name: Qamar Irfan
 * Student #: (fill-in)
 * Group: (fill-in)
 */

#include "myshell.h"

static void set_shell_env(const char *argv0) {
    char fullpath[PATH_MAX];

    // Best-effort to resolve the executable path
    if (realpath(argv0, fullpath) == NULL) {
        // Fallback: use argv0 as-is
        strncpy(fullpath, argv0, sizeof(fullpath) - 1);
        fullpath[sizeof(fullpath) - 1] = '\0';
    }

    setenv("shell", fullpath, 1);
}

static FILE *open_redirect_out(const char *out_file, int append) {
    if (!out_file) return NULL;
    return fopen(out_file, append ? "a" : "w");
}

static int is_internal(const char *cmd) {
    return (strcmp(cmd, "cd") == 0) ||
           (strcmp(cmd, "clr") == 0) ||
           (strcmp(cmd, "dir") == 0) ||
           (strcmp(cmd, "environ") == 0) ||
           (strcmp(cmd, "echo") == 0) ||
           (strcmp(cmd, "help") == 0) ||
           (strcmp(cmd, "pause") == 0) ||
           (strcmp(cmd, "quit") == 0);
}

static int run_internal(char **argv, int argc, FILE *out, int paged) {
    const char *cmd = argv[0];

    if (strcmp(cmd, "cd") == 0) {
        cmd_cd(argc > 1 ? argv[1] : NULL, out);
        return 0;
    }
    if (strcmp(cmd, "clr") == 0) {
        (void)out; // not used
        cmd_clr();
        return 0;
    }
    if (strcmp(cmd, "dir") == 0) {
        cmd_dir(argc > 1 ? argv[1] : NULL, out);
        return 0;
    }
    if (strcmp(cmd, "environ") == 0) {
        cmd_environ(out);
        return 0;
    }
    if (strcmp(cmd, "echo") == 0) {
        cmd_echo(argv, argc, out);
        return 0;
    }
    if (strcmp(cmd, "help") == 0) {
        cmd_help(out, paged);
        return 0;
    }
    if (strcmp(cmd, "pause") == 0) {
        (void)out;
        cmd_pause();
        return 0;
    }
    if (strcmp(cmd, "quit") == 0) {
        return 1; // signal to exit
    }

    return 0;
}

static void run_external(char **argv, char *in_file, char *out_file, int out_append, int background) {
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) {
        // Child
        // Set child-only env var "parent" to shell path
        const char *shell_path = getenv("shell");
        if (shell_path) setenv("parent", shell_path, 1);

        if (in_file) {
            FILE *f = fopen(in_file, "r");
            if (!f) {
                fprintf(stderr, "< %s: %s\n", in_file, strerror(errno));
                _exit(1);
            }
            dup2(fileno(f), STDIN_FILENO);
            fclose(f);
        }

        if (out_file) {
            FILE *f = fopen(out_file, out_append ? "a" : "w");
            if (!f) {
                fprintf(stderr, "> %s: %s\n", out_file, strerror(errno));
                _exit(1);
            }
            dup2(fileno(f), STDOUT_FILENO);
            fclose(f);
        }

        execvp(argv[0], argv);
        fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }

    // Parent
    if (!background) {
        int status;
        (void)waitpid(pid, &status, 0);
    }
}

int main(int argc, char *argv[]) {
    set_shell_env(argv[0]);

    // Ensure PWD is initialized
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        setenv("PWD", cwd, 1);
    }

    FILE *in = stdin;
    int batch_mode = 0;

    if (argc == 2) {
        in = fopen(argv[1], "r");
        if (!in) {
            fprintf(stderr, "Could not open batch file '%s': %s\n", argv[1], strerror(errno));
            return 1;
        }
        batch_mode = 1;
    } else if (argc > 2) {
        fprintf(stderr, "Usage: myshell [batchfile]\n");
        return 1;
    }

    char line[MAX_LINE];

    while (1) {
        if (!batch_mode) {
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s$ ", cwd);
            } else {
                printf("$ ");
            }
            fflush(stdout);
        }

        if (!fgets(line, sizeof(line), in)) {
            break; // EOF
        }

        // Parse
        char *argv_parsed[MAX_ARGS];
        int argc_parsed = 0;
        char *in_file = NULL;
        char *out_file = NULL;
        int out_append = 0;
        int background = 0;

        int ok = parse_line(line, argv_parsed, &argc_parsed, &in_file, &out_file, &out_append, &background);
        if (ok == 0) continue; // blank
        if (ok < 0) {
            fprintf(stderr, "Invalid command syntax\n");
            continue;
        }

        // Internal commands: support stdout redirection (dir, environ, echo, help)
        if (is_internal(argv_parsed[0])) {
            FILE *out = stdout;
            FILE *redir = NULL;
            int paged = 1;

            if (out_file) {
                redir = open_redirect_out(out_file, out_append);
                if (!redir) {
                    fprintf(stderr, "%s: %s\n", out_file, strerror(errno));
                    continue;
                }
                out = redir;
                paged = 0; // don't use more when redirecting
            }

            // Internal commands do not need input redirection or background
            if (in_file) {
                fprintf(stderr, "Input redirection is not supported for internal commands\n");
            }
            if (background) {
                fprintf(stderr, "Background execution is not supported for internal commands\n");
            }

            int should_exit = run_internal(argv_parsed, argc_parsed, out, paged);

            if (redir) fclose(redir);
            if (should_exit) break;

        } else {
            // External commands
            run_external(argv_parsed, in_file, out_file, out_append, background);
        }
    }

    if (batch_mode && in) fclose(in);
    return 0;
}
