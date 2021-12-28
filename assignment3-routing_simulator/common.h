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

#include "util.h"

int parse_location(char *path, struct Router *routers);

int parse_topology(char *path, struct Topology *topos);

int parse_command(char *line, struct Command *command);
