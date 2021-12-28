#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include <vector>

#include "data_structure.h"

ssize_t read_line(int fd, char *buffer, size_t n);

int parse_command(char *line, struct Command *command);