#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "shell.h"
#include "path.h"

int main(int argc, char *argv[]) {
    FILE *input = stdin;
    int interactive = 1;

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            perror("Batch file open error");
            exit(1);
        }
        interactive = 0;
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [batch_file]\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigint_handler);

    init_path();
    run_shell(input, interactive);

    if (input != stdin) fclose(input);
    return 0;
}
