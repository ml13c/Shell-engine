#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "execute.h"
#include "shell.h"

int should_exit = 0;

void sigint_handler(int signo) {
    printf("\n");
    fflush(stdout);
}

void run_shell(FILE *input, int interactive) {
    char line[512];
    while (!should_exit) {
        if (interactive) {
            printf("myshell> ");
            fflush(stdout);
        }

        if (!fgets(line, sizeof(line), input)) break;

        if (strlen(line) >= sizeof(line) - 1) {
            fprintf(stderr, "Warning: input line too long\n");
            continue;
        }

        if (!interactive) {
            printf("%s", line);
            fflush(stdout);
        }

        parse_and_execute(line);
    }
}
