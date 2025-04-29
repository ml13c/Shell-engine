#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LINE 512
#define MAX_ARGS 100
#define MAX_PATHS 100

int should_exit = 0;
char *path_list[MAX_PATHS];
int path_count = 0;

// === Signal Handler (Parent ignores Ctrl+C) ===
void sigint_handler(int signo) {
    printf("\n");
}

// === Path Management ===
void init_path() {
    char *env_path = getenv("PATH");
    if (!env_path) return;

    char *token = strtok(env_path, ":");
    while (token != NULL && path_count < MAX_PATHS) {
        path_list[path_count++] = strdup(token);
        token = strtok(NULL, ":");
    }
}

void print_path() {
    for (int i = 0; i < path_count; i++) {
        printf("%s", path_list[i]);
        if (i < path_count - 1) printf(":");
    }
    printf("\n");
}

void add_path(const char *new_path) {
    if (path_count < MAX_PATHS) {
        path_list[path_count++] = strdup(new_path);
    }
}

void remove_path(const char *target) {
    int found = -1;
    for (int i = 0; i < path_count; i++) {
        if (strcmp(path_list[i], target) == 0) {
            found = i;
            free(path_list[i]);
            break;
        }
    }
    if (found != -1) {
        for (int i = found; i < path_count - 1; i++) {
            path_list[i] = path_list[i+1];
        }
        path_count--;
    }
}

// === Utility ===
char *trim_whitespace(char *str) {
    while (*str == ' ' || *str == '\t') str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == '\n' || *end == ' ' || *end == '\t')) {
        *end-- = '\0';
    }
    return str;
}

char *find_executable(char *cmd) {
    static char full_path[MAX_LINE];
    for (int i = 0; i < path_count; i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path_list[i], cmd);
        if (access(full_path, X_OK) == 0) {
            return full_path;
        }
    }
    return NULL;
}

// === Built-in Command Handling ===
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
        if (args[1] == NULL) {
            print_path();
        } else if (args[1][0] == '+') {
            add_path(args[2]);
        } else if (args[1][0] == '-') {
            remove_path(args[2]);
        } else {
            fprintf(stderr, "Invalid path command\n");
        }
        return 1;
    }
    return 0;
}

// === Command Execution (No Fork Here) ===
void execute_command_direct(char *cmd) {
    char *args[MAX_ARGS];
    int argc = 0;
    int redirect_in = 0, redirect_out = 0;
    char *infile = NULL, *outfile = NULL;

    char *in_redir = strchr(cmd, '<');
    char *out_redir = strchr(cmd, '>');
    if (in_redir) {
        *in_redir = '\0';
        infile = trim_whitespace(in_redir + 1);
        redirect_in = 1;
    }
    if (out_redir) {
        *out_redir = '\0';
        outfile = trim_whitespace(out_redir + 1);
        redirect_out = 1;
    }

    cmd = trim_whitespace(cmd);
    if (strlen(cmd) == 0) exit(1);

    char *token = strtok(cmd, " \t\n");
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;

    if (argc == 0) exit(1);

    if (handle_builtin(args)) exit(0);

    if (redirect_in) {
        int fd = open(infile, O_RDONLY);
        if (fd < 0) {
            perror("input redirection failed");
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (redirect_out) {
        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("output redirection failed");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    char *exec_path = find_executable(args[0]);
    if (exec_path) {
        execv(exec_path, args);
        perror("execv failed");
    } else {
        fprintf(stderr, "command not found: %s\n", args[0]);
    }
    exit(1);
}

// === Run Single Command (With Fork) ===
void run_single_command(char *cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);  // Default Ctrl+C behavior in child
        signal(SIGTSTP, SIG_DFL); // Default Ctrl+Z behavior in child
        setpgid(0, 0);
        execute_command_direct(cmd);
    } else {
        int status;
        waitpid(pid, &status, WUNTRACED);
        if(WIFSTOPPED(status)) {
                kill(pid, SIGKILL);
        }
        freopen("/dev/tty", "r", stdin);
        }
}

// === Piped Commands ===
void run_piped_commands(char *line) {
    char *commands[3];
    int cmd_count = 0;

    char *token = strtok(line, "|");
    while (token != NULL && cmd_count < 3) {
        commands[cmd_count++] = trim_whitespace(token);
        token = strtok(NULL, "|");
    }

    int pipes[2];
    int input_fd = 0;

    for (int i = 0; i < cmd_count; i++) {
        if (i < cmd_count - 1) {
            if (pipe(pipes) < 0) {
                perror("pipe failed");
                return;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            return;
        }

        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            setpgid(0, 0);

            if (input_fd != 0) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            if (i < cmd_count - 1) {
                dup2(pipes[1], STDOUT_FILENO);
                close(pipes[0]);
                close(pipes[1]);
            }

            execute_command_direct(commands[i]);
        } else {
            if (input_fd != 0) close(input_fd);
            if (i < cmd_count - 1) {
                close(pipes[1]);
                input_fd = pipes[0];
            }
            waitpid(pid, NULL, 0);
        }
    }
}

// === Parsing Commands (Semicolons) ===
void parse_and_execute(char *line) {
    char *command = strtok(line, ";");
    while (command != NULL) {
        command = trim_whitespace(command);
        if (strlen(command) == 0) {
            command = strtok(NULL, ";");
            continue;
        }

        if (strchr(command, '|')) {
            run_piped_commands(command);
        } else {
            run_single_command(command);
        }

        command = strtok(NULL, ";");
    }
}

// === Main Shell Loop ===
void run_shell(FILE *input, int interactive) {
    char line[MAX_LINE];

    while (1) {
        if (interactive) {
            printf("myshell> ");
            fflush(stdout);
        }

        if (fgets(line, MAX_LINE, input) == NULL) break;

        if (strlen(line) >= MAX_LINE - 1) {
            fprintf(stderr, "Warning: input line too long\n");
            continue;
        }

        if (!interactive) {
            printf("%s", line);
            fflush(stdout);
        }

        parse_and_execute(line);
        if (should_exit) break;
    }
}

// === Main ===
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