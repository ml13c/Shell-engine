#ifndef EXECUTE_H
#define EXECUTE_H

void run_single_command(char *cmd);
void run_piped_commands(char *line);
void parse_and_execute(char *line);
char *trim_whitespace(char *str);

#endif
