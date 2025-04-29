#ifndef PATH_H
#define PATH_H

#define MAX_PATHS 100

extern char *path_list[MAX_PATHS];
extern int path_count;

void init_path();
void print_path();
void add_path(const char *new_path);
void remove_path(const char *target);
char *find_executable(char *cmd);

#endif
