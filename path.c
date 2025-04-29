#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "path.h"

char *path_list[MAX_PATHS];
int path_count = 0;

void init_path() {
    char *env_path = getenv("PATH");
    if (!env_path) return;

    char *copy = strdup(env_path);
    char *token = strtok(copy, ":");
    while (token && path_count < MAX_PATHS) {
        path_list[path_count++] = strdup(token);
        token = strtok(NULL, ":");
    }
    free(copy);
}

void print_path() {
    for (int i = 0; i < path_count; i++) {
        printf("%s", path_list[i]);
        if (i < path_count - 1) printf(":");
    }
    printf("\n");
}

void add_path(const char *new_path) {
    if (path_count < MAX_PATHS && new_path) {
        path_list[path_count++] = strdup(new_path);
    }
}

void remove_path(const char *target) {
    if (!target) return;
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
            path_list[i] = path_list[i + 1];
        }
        path_count--;
    }
}

char *find_executable(char *cmd) {
    static char full_path[512];
    for (int i = 0; i < path_count; i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path_list[i], cmd);
        if (access(full_path, X_OK) == 0) return full_path;
    }
    return NULL;
}
