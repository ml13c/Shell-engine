#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "path.h"

extern int should_exit;

int handle_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        const char *path = args[1] ? args[1] : getenv("HOME");
        if (chdir(path) != 0) perror("cd failed");
        return 1;
    }

    if (strcmp(args[0], "exit") == 0) {
        should_exit = 1;
        return 1;
    }

    if (strcmp(args[0], "path") == 0) {
        if (!args[1]) {
            print_path();
        } else if (strcmp(args[1], "+") == 0) {
            if (args[2]) add_path(args[2]);
            else fprintf(stderr, "Usage: path + <dir>\n");
        } else if (strcmp(args[1], "-") == 0) {
            if (args[2]) remove_path(args[2]);
            else fprintf(stderr, "Usage: path - <dir>\n");
        } else {
            fprintf(stderr, "Usage: path [ + | - ] <dir>\n");
        }
        return 1;
    }

    return 0;
}
