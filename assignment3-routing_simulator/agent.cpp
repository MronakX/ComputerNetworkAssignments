#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>


#include <iostream>
#include <map>
#include <vector>
#include <string>

#include "util.h"
#include "common.h"

struct Router routers[MAXROUTER];
int max_router_id = 0;


int dv(struct Command *command, char *buf)
{
    printf("dv called\n");
    int clientfd;
    struct sockaddr_in addr;
    for(int i=0; i<max_router_id; i++)
    {
        int router_id = i;
        int port = routers[router_id].router_port;
        printf("trying to connect router %d, port = %d\n", router_id, port);
        if(port == 0)
            continue;
        if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            printf("create socket failed\n");
            return -1;
        }
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if(inet_pton(AF_INET, routers[router_id].router_ip, &addr.sin_addr) <= 0) 
        {
            printf("address failed\n");
            return -1;
        }
        if(connect(clientfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
        {
            printf("connect failed\n");
            return -1;
        }
        struct Packet packet;
        packet.type = CMD_DV;
        memset(&(packet.buffer), 0, sizeof(packet.buffer));
        memcpy(&(packet.buffer), buf, strlen(buf));
        if(send(clientfd, &packet, sizeof(packet), 0) == -1)
        {
            printf("send error\n");
            continue;
        }
        printf("[INFO]: send dv to router %d\n%", i);
        /*
        fd_set ready_set;
        FD_ZERO(&ready_set);
        FD_SET(clientfd, &ready_set);
        char recv_buffer[MAXLINE];
        printf("blocking on select..waiting for finished dv\n");
        while(1)
        {
            select(clientfd+1, &ready_set, NULL, NULL, NULL);
            if(FD_ISSET(clientfd, &ready_set))
            {
                recv(clientfd, &recv_buffer, MAXLINE, 0);
                break;
            }
        }
        */
        close(clientfd);
    }
    sleep(2);
    return 1;
}

int show(struct Command *command, char *buf)
{
    printf("show called\n");
    int clientfd;
    struct sockaddr_in addr;
    char buffer[4096];
    int router_id = command->router_id_1;
    int port = routers[router_id].router_port;
    printf("router_id = %d, port = %d\n", router_id, port);
    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("create socket failed\n");
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(inet_pton(AF_INET, routers[router_id].router_ip, &addr.sin_addr) <= 0) 
    {
        printf("address failed\n");
        return -1;
    }
    if(connect(clientfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        printf("connect failed\n");
        return -1;
    }
    struct Packet packet;
    packet.type = CMD_SHOW;
    memset(&(packet.buffer), 0, sizeof(packet.buffer));
    memcpy(&(packet.buffer), buf, strlen(buf));
    if(send(clientfd, &packet, sizeof(packet), 0) == -1)
    {
        printf("send error\n");
        return -1;
    }

    ShowCommand show_command;
    memset(&show_command, 0, sizeof(show_command));
    if(recv(clientfd, &(show_command), sizeof(show_command), 0) > 0)
    {
        printf("agent receive show info!\n");
    }
    for(int i=0; i<MAXROUTER; i++)
    {
        if(show_command.cost[i] != -1)
        {
            printf("dest: %d, next: %d, cost: %d\n", show_command.dest[i], show_command.next[i], show_command.cost[i]);
        }
    }
    close(clientfd);
    return 1;
}

int update(struct Command *command, char* buf)
{
    printf("update called\n");
    int clientfd;
    struct sockaddr_in addr;
    char buffer[4096];
    int router_id = command->router_id_1;
    int port = routers[router_id].router_port;

    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("create socket failed\n");
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(inet_pton(AF_INET, routers[router_id].router_ip, &addr.sin_addr) <= 0) 
    {
        printf("address failed\n");
        return -1;
    }
    if(connect(clientfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        printf("connect failed\n");
        return -1;
    }
    struct Packet packet;
    packet.type = CMD_UPDATE;
    memset(&(packet.buffer), 0, sizeof(packet.buffer));
    memcpy(&(packet.buffer), buf, strlen(buf));
    if(send(clientfd, &packet, sizeof(packet), 0) == -1)
    {
        printf("send error\n");
        return -1;
    }
    close(clientfd);

    char recv_info[MAXLINE];
    if(recv(clientfd, recv_info, sizeof(recv_info), 0) > 0)
    {
        printf("agent receive show info:\n %s ",recv_info);
    }
    close(clientfd);
    return 1;
}

int reset(struct Command *command, char *buf)
{
    printf("reset called\n");
    int clientfd;
    struct sockaddr_in addr;
    char buffer[4096];
    int router_id = command->router_id_1;
    int port = routers[router_id].router_port;

    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("create socket failed\n");
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(inet_pton(AF_INET, routers[router_id].router_ip, &addr.sin_addr) <= 0) 
    {
        printf("address failed\n");
        return -1;
    }
    if(connect(clientfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        printf("connect failed\n");
        return -1;
    }

    struct Packet packet;
    packet.type = CMD_RESET;
    memset(&(packet.buffer), 0, sizeof(packet.buffer));
    memcpy(&(packet.buffer), buf, strlen(buf));
    if(send(clientfd, &packet, sizeof(packet), 0) == -1)
    {
        printf("send error\n");
        return -1;
    }
    close(clientfd);

    char recv_info[MAXLINE];
    if(recv(clientfd, recv_info, sizeof(recv_info), 0) > 0)
    {
        printf("agent receive show info:\n %s ",recv_info);
    }
    close(clientfd);
    return 1;
}


int agent()
{
    printf("in agent()\n");
    fd_set read_set, ready_set;
    int maxfd = STDIN_FILENO;
    FD_ZERO(&read_set);
    FD_SET(STDIN_FILENO, &read_set);
    while(1)
    {
        ready_set = read_set;
        printf("blocking on select..\n");
        int ret = select(STDIN_FILENO+1, &ready_set, NULL, NULL, NULL);
        if(FD_ISSET(STDIN_FILENO, &ready_set))
        {
            struct Command command;
            char buf[MAXLINE];
            char cache[MAXLINE];
            memset(buf, 0, sizeof(buf));
            memset(cache, 0, sizeof(cache));

            read_line(STDIN_FILENO, buf, MAXLINE);
            memcpy(cache, buf, sizeof(buf));

            printf("read command from shell, command is %s", buf);
            int tmp = parse_command(buf, &(command));
            if(tmp == -1)
            {
                printf("[info]:required command:\n1. dv\n2. show:id\n3. update:id1, id2, weight\n4. reset id\n");
                continue;
            }
            if(command.type == CMD_DV)
                dv(&command, cache);
            else if(command.type == CMD_SHOW)
                show(&command, cache);
            else if(command.type == CMD_UPDATE)
                update(&command, cache);
            else
                reset(&command, cache);
        }
    }
}

int main(int argc, char **argv)
{
    setvbuf(stdout ,NULL, _IONBF, 0);
    printf("./agent <router location file>\n");
    char *location = argv[1];
    max_router_id = parse_location(location, routers);
    printf("parse finished\n");
    agent();
    return 0;
}
