/*
This is a shell engine that can execute commands in a shell-like environment. It has an interactive mode
and batch mode.
    @author: Marcus Lara Sanchez, Lucerro Torres
    CSCE 3600 
*/
// myshell.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LINE 512
#define MAX_ARGS 100

int should_exit = 0; // Flag for exit built-in

// Trim leading/trailing whitespace
char *trim_whitespace(char *str) {
    while (*str == ' ' || *str == '\t') str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && (*end == '\n' || *end == ' ' || *end == '\t')) {
        *end-- = '\0';
    }
    return str;
}

// Execute built-in commands like cd and exit
int handle_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        const char *path = args[1] ? args[1] : getenv("HOME");
        if (chdir(path) != 0) {
            perror("cd failed");
        }
        return 1;
    }

    if (strcmp(args[0], "exit") == 0) {
        should_exit = 1;
        return 1;
    }

    return 0; // Not a built-in
}

// Parse a single command into args and run it
void execute_command(char *command) {
    char *args[MAX_ARGS];
    int argc = 0;

    command = trim_whitespace(command);
    if (strlen(command) == 0) return;

    // Tokenize arguments
    char *token = strtok(command, " \t\n");
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;

    if (argc == 0) return;

    // Check for built-in commands
    if (handle_builtin(args)) return;

    // Fork and execute external command
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        // Child process
        execvp(args[0], args);
        perror("exec failed"); // Only runs if exec fails
        exit(1);
    } else {
        // Parent process
        if (waitpid(pid, NULL, 0) < 0) {
            perror("waitpid failed");
        }
    }
}

// Break a line into commands separated by ;
void parse_and_execute(char *line) {
    char *command = strtok(line, ";");
    while (command != NULL) {
        command = trim_whitespace(command);
        if (strlen(command) > 0) {
            execute_command(command);
        }
        command = strtok(NULL, ";");
    }
}

// Shell loop
void run_shell(FILE *input, int interactive) {
    char line[MAX_LINE];

    while (1) {
        if (interactive) {
            printf("myshell> ");
            fflush(stdout);
        }

        if (fgets(line, MAX_LINE, input) == NULL) break;

        if (strlen(line) >= MAX_LINE - 1) {
            fprintf(stderr, "Warning: input line too long (max 512 chars)\n");
            continue;
        }

        if (!interactive) {
            printf("%s", line); // Echo in batch mode
            fflush(stdout);
        }

        parse_and_execute(line);
        if (should_exit) break;
    }
}

// Main entry
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

    run_shell(input, interactive);

    if (input != stdin) fclose(input);
    return 0;
}
