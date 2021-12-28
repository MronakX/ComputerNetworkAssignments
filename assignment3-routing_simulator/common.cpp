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
#include "common.h"


int parse_location(char *path, struct Router *routers)
{
    int fd;
    if((fd = open(path, O_RDONLY)) < 0)
    {
        printf("open failed! check your path\n");
        exit(0);
    }
    char buf[MAXLINE];
    memset(buf, 0, sizeof(buf));
    char router_num_[MAXLINE];
    read_line(fd, router_num_, MAXLINE);
    memset(buf, 0, sizeof(buf));
    int router_num = atoi((const char*)router_num_);

    int cnt = 0;
    int ret = -1;
    while(read_line(fd, buf, MAXLINE) > 0)
    {
        char line[MAXLINE];
        strcpy(line, buf);
        int len = strlen(line);
        char router_ip[MAXLINE];
        char router_port[MAXLINE];
        char router_id[MAXLINE];
        memset(router_ip, 0, sizeof(router_ip));
        memset(router_port, 0, sizeof(router_port));
        memset(router_id, 0, sizeof(router_id));
        int i, j, k;
        struct Router current;

        for(i=0; i<len; i++)
        {
            if(*(line + i) == ',')
                break;
            router_ip[i] = line[i]; 
        }
        for(j=i+1; j<len; j++)
        {
            if(*(line + j) == ',')
                break;
            router_port[j-i-1] = line[j];
        }
        for(k=j+1; k<len; k++)
        {
            if(line[k] == '\n')
                continue;
            router_id[k-j-1] = line[k];
        }

        strcpy(current.router_ip, router_ip);
        current.router_port = atoi(router_port);
        current.router_id = atoi(router_id);
        ret = std::max(ret, atoi(router_id));
        memcpy(routers + current.router_id, &current, sizeof(struct Router));
        cnt++;
        memset(buf, 0, sizeof(buf));
    }
    return ret + 1;
}

int parse_topology(char *path, struct Topology *topos)
{
    int fd;
    if((fd = open(path, O_RDONLY)) < 0)
    {
        printf("open failed! check your path\n");
        exit(0);
    }
    char buf[MAXLINE];
    memset(buf, 0, sizeof(buf));
    char topo_num_[MAXLINE];
    read_line(fd, topo_num_, MAXLINE);
    memset(buf, 0, sizeof(buf));
    int topo_num = atoi((const char*)topo_num_);

    int cnt = 0;

    while(read_line(fd, buf, MAXLINE) > 0)
    {
        char line[MAXLINE];
        strcpy(line, buf);
        int len = strlen(line);
        char router_id_1[MAXLINE];
        char router_id_2[MAXLINE];
        char weight[MAXLINE];
        memset(router_id_1, 0, sizeof(router_id_1));
        memset(router_id_2, 0, sizeof(router_id_2));
        memset(weight, 0, sizeof(weight));
        int i, j, k;
        struct Topology current;

        for(i=0; i<len; i++)
        {
            if(*(line + i) == ',')
                break;
            router_id_1[i] = line[i]; 
        }
        for(j=i+1; j<len; j++)
        {
            if(*(line + j) == ',')
                break;
            router_id_2[j-i-1] = line[j];
        }
        for(k=j+1; k<len; k++)
        {
            if(line[k] == '\n')
                continue;
            weight[k-j-1] = line[k];
        }

        current.router_id_1 = atoi(router_id_1);
        current.router_id_2 = atoi(router_id_2);
        current.weight = atoi(weight);

        memcpy(topos + cnt, &current, sizeof(struct Topology));
        cnt++;
        memset(buf, 0, sizeof(buf));
    }
    return topo_num;
}

int parse_command(char *line, struct Command *command)
{
    int len = strlen(line);
    char *buf;
    if(line[0] == 'd')//dv
    {
        command->type = CMD_DV;
        return 1;
    }
    else if(line[0] == 's')//show id
    {
        buf = strtok(line, ":");
        command->type = CMD_SHOW;
        buf = strtok(NULL, ":");

        if(buf == NULL)
            return -1;
        command->router_id_1 = atoi(buf);
    }
    else if(line[0] == 'u')//update id1 id2 w
    {
        buf = strtok(line, ":");
        command->type = CMD_UPDATE;
        printf("1: buf = %s\n", buf);

        buf = strtok(NULL, ",");
        if(buf == NULL)
            return -1;
        command->router_id_1 = atoi(buf);
        printf("2: id1 = %s\n", buf);

        buf = strtok(NULL, ",");
        if(buf == NULL)
            return -1;
        command->router_id_2 = atoi(buf);
        printf("3: id2 = %s\n", buf);

        buf = strtok(NULL, ",");
        if(buf == NULL)
            return -1;
        command->weight = atoi(buf);
        printf("4: w = %s\n", buf);
    }
    else if(line[0] == 'r')
    {
        buf = strtok(line, ":");
        command->type = CMD_RESET;
        buf = strtok(line, ":");
        if(buf == NULL)
            return -1;
        command->router_id_1 = atoi(buf);
    }
    else 
        return -1;
    return 1;
}


