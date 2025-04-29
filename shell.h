#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>

void sigint_handler(int signo);
void run_shell(FILE *input, int interactive);

#endif
