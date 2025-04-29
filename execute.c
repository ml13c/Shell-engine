#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#include "execute.h"
#include "builtins.h"
#include "path.h"


extern int should_exit;

char *trim_whitespace(char *str) {
    while (*str == ' ' || *str == '\t') str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == '\n' || *end == ' ' || *end == '\t')) {
        *end-- = '\0';
    }
    return str;
}

// === Command Execution ===
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
    if (strlen(cmd) == 0) exit(0);

    char *token = strtok(cmd, " \t\n");
    while (token && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;

    if (argc == 0) exit(0);

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

void run_single_command(char *cmd) {
    if (strlen(trim_whitespace(cmd)) == 0) return;

    // Parse arguments
    char *args[MAX_ARGS];
    int argc = 0;
    char *infile = NULL, *outfile = NULL;
    int redirect_in = 0, redirect_out = 0;

    char *in_redir = strchr(cmd, '<');
    if (in_redir) {
        *in_redir = '\0';
        infile = trim_whitespace(in_redir + 1);
        redirect_in = 1;
    }

    char *out_redir = strchr(cmd, '>');
    if (out_redir) {
        *out_redir = '\0';
        outfile = trim_whitespace(out_redir + 1);
        redirect_out = 1;
    }

    cmd = trim_whitespace(cmd);
    if (strlen(cmd) == 0) return;

    char *token = strtok(cmd, " \t\n");
    while (token && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;

    if (argc == 0) return;

    // Handle built-in in parent
    if (handle_builtin(args)) return;

    // Fork for external commands only
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        // In child
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        setpgid(0, 0);

        // Redirection
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
    } else {
        int status;
        waitpid(pid, &status, WUNTRACED);
        if (WIFSTOPPED(status)) kill(pid, SIGKILL);
    }
}

void run_piped_commands(char *line) {
    char *commands[3];
    int cmd_count = 0;

    char *token = strtok(line, "|");
    while (token && cmd_count < 3) {
        commands[cmd_count++] = trim_whitespace(token);
        token = strtok(NULL, "|");
    }

    if (cmd_count == 0) return;

    int input_fd = 0;
    int pipes[2];

    for (int i = 0; i < cmd_count; i++) {
        if (i < cmd_count - 1 && pipe(pipes) < 0) {
            perror("pipe failed");
            return;
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

void parse_and_execute(char *line) {
    char *command = strtok(line, ";");
    while (command) {
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

        if (should_exit) break;

        command = strtok(NULL, ";");
    }
}
