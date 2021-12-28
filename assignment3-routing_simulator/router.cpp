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

#include "common.h"
#include "util.h"

struct Router routers[MAXROUTER];
struct Topology topos[MAXTOPO];
struct Node nodes[MAXROUTER];

#define MAXDISTANCE 10000

int max_router_id = 0, topo_num = 0;

#define backlog 100

void printinfo(int router_id)
{
    for(int i=0; i<MAXROUTER; i++)
    {
        nodes[router_id].routing_table[i].router_id = i;
        nodes[router_id].routing_table[i].distance = -1;
        nodes[router_id].routing_table[i].next_hop = -1;
        if(router_id == i)
        {
            nodes[router_id].routing_table[i].next_hop = i;
            nodes[router_id].routing_table[i].distance = 0;
        }
        printf("nodes[%d].routing_table[%d].router_id = %d\n", router_id, i, nodes[router_id].routing_table[i].router_id);
        printf("nodes[%d].routing_table[%d].next_hop = %d\n", router_id, i, nodes[router_id].routing_table[i].next_hop);
        printf("nodes[%d].routing_table[%d].distance = %d\n\n", router_id, i, nodes[router_id].routing_table[i].distance);
    }
}

void initialize_distance(int router_id)
{
    nodes[router_id].router_id = router_id;
    for(int i=0; i<MAXROUTER; i++)
    {
        nodes[router_id].cost[i] = -1;
        nodes[router_id].cost[router_id] = 0;
    }
    for(int i=0; i<topo_num; i++)
    {
        struct Topology topo = *(topos + i);
        int router_id_1 = topo.router_id_1;
        int router_id_2 = topo.router_id_2;
        if(router_id_1 != router_id)
            continue;
    
        nodes[router_id].cost[router_id_2] = topo.weight;
        nodes[router_id].neighbors.push_back(router_id_2);
    }

    for(int i=0; i<MAXROUTER; i++)
    {
        for(int j=0; j<MAXROUTER; j++)
        {
            nodes[router_id].distance[i][j] = -1;
            if(i == j)
                nodes[router_id].distance[i][i] = 0;
        }
    }
    for(int i=0; i<MAXROUTER; i++)
    {
        nodes[router_id].routing_table[i].router_id = i;
        nodes[router_id].routing_table[i].distance = -1;
        nodes[router_id].routing_table[i].next_hop = -1;
        if(router_id == i)
        {
            nodes[router_id].routing_table[i].next_hop = i;
            nodes[router_id].routing_table[i].distance = 0;
        }
        //printinfo(router_id);
    }
}

/* @return 
 * when some distance changed, return 1
 * when nothing changed, return -1;
 */
int calculate_routing_table(int router_id)
{
    printf("[INFO]: routing table changing..\n");
    struct Node* node = &(nodes[router_id]);
    int flag = -1;
    for(int i=0; i<max_router_id; i++)//goal
    {
        flag = -1;
        int min_distance = MAXDISTANCE;

        //int next_hop = node->routing_table[i].next_hop;
        int next_hop = -1;
        if(i == router_id) 
            continue;
        //printf("we got %d neighbrors right now\n", node->neighbors.size());
        for(int j=0; j < node->neighbors.size(); j++)
        {
            int current_neighbor = node->neighbors[j];// hashed
            printf("[DEBUG]: current neighbor = %d, destination is %d\n", current_neighbor, i);
            printf("[DEBUG]: cost = %d, distance = %d\n", node->cost[current_neighbor], node->distance[current_neighbor][i]);
            if(node->distance[current_neighbor][i] != -1 && node->cost[current_neighbor] != -1)
            {
                //printf("start: %d to mid %d: %d; mid %d to end %d: %d\n", router_id, current_neighbor, node->cost[current_neighbor], 
                //                                                        current_neighbor, i, node->distance[current_neighbor][i]);
                if(min_distance >= node->cost[current_neighbor] + node->distance[current_neighbor][i])
                {
                    if(min_distance == node->cost[current_neighbor] + node->distance[current_neighbor][i])
                        if(next_hop == -1)
                            next_hop = current_neighbor;
                        else
                            next_hop = std::min(current_neighbor, next_hop);
                    else
                        next_hop = current_neighbor;
                    printf("[info]: some path changed: begin: %d, end: %d, cost: %d, next_hop: %d\n", router_id, i, node->cost[current_neighbor] + node->distance[current_neighbor][i], next_hop);
                    flag = 1;
                }
                min_distance = std::min(min_distance, node->cost[current_neighbor] + node->distance[current_neighbor][i]);
            }
        }

        node->routing_table[i].next_hop = next_hop;
        if(min_distance > 1000)
        {
            node->routing_table[i].distance = -1; 
            /*std::vector<int>::iterator it = std::find(node->neighbors.begin(), node->neighbors.end(), next_hop);
            if(it != node->neighbors.end())
                node->neighbors.erase(it);*/
            //node->cost[i] = -1;
        }
        else if(flag == 1)
        {
            /*std::vector<int>::iterator it = std::find(node->neighbors.begin(), node->neighbors.end(), i);
            if(node->cost[i] != -1)
                node->cost[i] = min_distance;
            if(it == node->neighbors.end())
                node->neighbors.push_back(i);*/
            node->routing_table[i].distance = min_distance;
        }
        else if(min_distance == MAXDISTANCE)
        {
            node->routing_table[i].distance = -1;
            //node->cost[i] = -1;
        }
        printf("[INFO]: routing table has literally update! start = %d, end = %d, cost = %d, next_hop = %d\n", 
                router_id, i, node->routing_table[i].distance, node->routing_table[i].next_hop);
    }
    node->routing_table[router_id].router_id = router_id;
    node->routing_table[router_id].next_hop = router_id;
    node->routing_table[router_id].distance = 0;
    return flag;
}

int dv_from_agent(int router_id, int fd)
{
    printf("[info]: received DV from agent\n");
    
    struct Node old_table;
    memset(&old_table, 0, sizeof(old_table));
    for(int i=0; i<MAXROUTER; i++)
    {
        old_table.routing_table[i].distance = nodes[router_id].routing_table[i].distance;
    } 
    calculate_routing_table(router_id);
    int ret = 1;
    for(int i=0; i<MAXROUTER; i++)
    {
        if(old_table.routing_table[i].distance != nodes[router_id].routing_table[i].distance)
        {
            ret = 1;
            break;
        }
    }   
    printf("[info]: has changed: %d\n", ret);
    struct Packet packet;
    memset(&(packet), 0, sizeof(packet));
    packet.type = CMD_PROP;
    for(int i=0; i<MAXROUTER; i++)
    {
        packet.distance[i] = nodes[router_id].routing_table[i].distance;
    }
    packet.router_id = router_id;

    if(ret == -1)
    {
        return -1;
    }

    //printf("neighbor_size = %d\n", nodes[router_id].neighbors.size());

    for(int i=0; i<nodes[router_id].neighbors.size(); i++)
    {
        int neighbor_id = nodes[router_id].neighbors[i];
        int neighbor_router_id = routers[neighbor_id].router_id;
        int neighbor_router_port = routers[neighbor_id].router_port;

        int clientfd;
        struct sockaddr_in addr;
        if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            printf("create socket failed\n");
            return -1;
        }
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(neighbor_router_port);
        if(inet_pton(AF_INET, routers[neighbor_id].router_ip, &addr.sin_addr) <= 0) 
        {
            printf("address failed\n");
            return -1;
        }
        if(connect(clientfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
        {
            printf("connect failed\n");
            return -1;
        }
        if(send(clientfd, &packet, sizeof(packet), 0) == -1)
        {
            printf("send error\n");
            return -1;
        }
        printf("[info]: router %d send packet to %d\n", router_id, neighbor_id);
        
        //return to agent
    }
    return ret;
}

int dv_from_router(Packet *recv, int router_id, int fd)
{   
    int update_id = recv->router_id;
    printf("[info]: received DV from router %d\n", update_id);
    int ret = -1;
    for(int i=0; i<MAXROUTER; i++)
    {
        printf("[RECV]: receiving dv: start = %d, end = %d, cost = %d\n", recv->router_id, i, recv->distance[i]);
    }
    //find
    std::vector<int>::iterator it = std::find(nodes[router_id].neighbors.begin(), nodes[router_id].neighbors.end(), recv->router_id);
    if(it == nodes[router_id].neighbors.end())
        return -1;
    memcpy(nodes[router_id].distance[update_id], recv->distance, sizeof(nodes[router_id].distance[update_id]));
    for(int i=0; i<MAXROUTER; i++)
    {
        printf("[VERIFY]: receiving dv: start = %d, end = %d, cost = %d\n", recv->router_id, i, nodes[router_id].distance[update_id][i]);
    }
    struct Node old_table;
    memset(&old_table, 0, sizeof(old_table));
    for(int i=0; i<MAXROUTER; i++)
    {
        old_table.routing_table[i].distance = nodes[router_id].routing_table[i].distance;
    } 
    calculate_routing_table(router_id);

    for(int i=0; i<MAXROUTER; i++)
    {
        if(old_table.routing_table[i].distance != nodes[router_id].routing_table[i].distance)
        {
            ret = 1;
            break;
        }
    }   
    
    printf("[info]: has changed: %d\n", ret);

    if(ret == -1)
    {
        printf("[info]: dv not changing, not return any message to router\n");
        return -1;
    }
    Packet packet;
    memset(&packet, 0, sizeof(packet));
    memcpy(&packet, recv, sizeof(packet));
    packet.type = CMD_PROP;
    for(int i=0; i<MAXROUTER; i++)
    {
        packet.distance[i] = nodes[router_id].routing_table[i].distance;
    }
    packet.router_id = router_id;
    for(int i=0; i<MAXROUTER; i++)
    {
        printf("[SEND]: sending packet: start: %d, end: %d, cost: %d\n", router_id, i, packet.distance[i]);
    }
    printf("\n");
    for(int i=0; i<nodes[router_id].neighbors.size(); i++)
    {
        printf("i = %d\n", i);
        int neighbor_id = nodes[router_id].neighbors[i];
        int neighbor_router_id = routers[neighbor_id].router_id;
        int neighbor_router_port = routers[neighbor_id].router_port;

        int clientfd;
        struct sockaddr_in addr;
        if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            printf("create socket failed\n");
            return -1;
        }
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(neighbor_router_port);
        if(inet_pton(AF_INET, routers[neighbor_id].router_ip, &addr.sin_addr) <= 0) 
        {
            printf("address failed\n");
            return -1;
        }
        if(connect(clientfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
        {
            printf("connect failed\n");
            return -1;
        }
        if(send(clientfd, &packet, sizeof(packet), 0) == -1)
        {
            printf("send error\n");
            return -1;
        }
        printf("send packet to router %d\n", neighbor_id);
        //wait for dv_from_router()
        
        close(clientfd);
        //return to agent
    }
    printf("before rerturn\n");
    return 1;
}

int show_from_agent(int router_id, int fd)
{
    printf("[info]: received SHOW from agent\n");
    //calculate_routing_table(router_id);
    //printinfo(router_id);
    struct Node *node = &(nodes[router_id]);
    char buffer[MAXLINE];
    memset(buffer, 0, sizeof(buffer));
    ShowCommand show_command;
    memset(&show_command, 0, sizeof(show_command));
    for(int i=0; i<MAXROUTER; i++)
    {
        show_command.cost[i] = -1;
    }
    int cnt = 0;
    for(int i=0; i<MAXROUTER; i++)
    {
        if(node->routing_table[i].distance == -1)
            continue;
        printf("i = %d\n", i);
        show_command.dest[i] = node->routing_table[i].router_id;
        show_command.next[i] = node->routing_table[i].next_hop;
        show_command.cost[i] = node->routing_table[i].distance;
        cnt++;
    }
    /*
    for(int i=0; i<MAXROUTER; i++)
    {
        printf("%d \n %d \n %d \n", show_command.dest[i], show_command.next[i], show_command.cost[i]);
    }
    */
    //send back
    fd_set ready_set;
    FD_ZERO(&ready_set);
    FD_SET(fd, &ready_set);
    while(1)
    {
        select(fd + 1, NULL, &ready_set, NULL, 0);
        if(FD_ISSET(fd, &ready_set))
            break;
    }
    if(send(fd, &(show_command), sizeof(show_command), 0) < 0)
        printf("send error");
    printf("send show message to agent\n");
    
    return 1;
}

int update_from_agent(char *buffer, int router_id, int fd)
{
    printf("[info]: received UPDATE from agent\n");

    Command command;
    parse_command(buffer, &(command));
    int id1 = command.router_id_1;
    int id2 = command.router_id_2;
    int weight = command.weight;
    nodes[id1].cost[id2] = weight;
    
    if(weight == -1 || weight > 1000)
    {
        std::vector<int>::iterator it = std::find(nodes[id1].neighbors.begin(), nodes[id1].neighbors.end(), id2);
        if(it != nodes[id1].neighbors.end())
            nodes[id1].neighbors.erase(it);
    }
    else
    {
        std::vector<int>::iterator it = std::find(nodes[id1].neighbors.begin(), nodes[id1].neighbors.end(), id2);
        if(it == nodes[id1].neighbors.end())
            nodes[id1].neighbors.push_back(id2);
    }

    calculate_routing_table(router_id);
    //send back
    fd_set ready_set;
    FD_ZERO(&ready_set);
    FD_SET(fd, &ready_set);
    while(1)
    {
        select(fd + 1, NULL, &ready_set, NULL, 0);
        if(FD_ISSET(fd, &ready_set))
            break;
    }
    char buf[MAXLINE];
    strcpy(buf, "update finished");
    send(fd, &(buf), strlen(buf), 0);

    return 1;
}

int reset_from_agent(char *buffer, int router_id, int fd)
{
    //send back
    printf("[info]: received RESET from agent\n");

    fd_set ready_set;
    FD_ZERO(&ready_set);
    FD_SET(fd, &ready_set);
    while(1)
    {
        select(fd + 1, NULL, &ready_set, NULL, 0);
        if(FD_ISSET(fd, &ready_set))
            break;
    }
    char buf[MAXLINE];
    strcpy(buf, "reset finished");
    send(fd, &(buf), strlen(buf), 0);

    return 1;
}

int doit(struct Packet *packet, int router_id, int fd)
{
    if(packet->type == CMD_DV)
    {   
        return dv_from_agent(router_id, fd);
    }
    else if(packet->type == CMD_SHOW)
    {
        return show_from_agent(router_id, fd);
    }
    else if(packet->type == CMD_UPDATE)
    {
        return update_from_agent(packet->buffer, router_id, fd);
    }
    else if(packet->type == CMD_RESET)
    {
        return reset_from_agent(packet->buffer, router_id, fd);
    }
    else if(packet->type == CMD_PROP)
    {
        return dv_from_router(packet, router_id, fd);
        //printf("bp\n");
    }
    return 0;
}

int router(int router_id)   
{
    printf("in router()\n");
    while(1)
    {
        int listenfd, connfd;
        int opt = 1;
        struct sockaddr_in addr;
        socklen_t addrlen;
        memset(&(addr), 0, sizeof(addr));

        fd_set read_set, ready_set;

        addr.sin_family = AF_INET;
        addr.sin_port = htons(routers[router_id].router_port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            printf("create socket failed\n");
            return -1;
        }

        if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
        {
            printf("setsockopt failed\n");
            return -1;
        }

        if(bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            printf("bind failed\n");
            return -1;
        }
        
        if(listen(listenfd, backlog) < 0)
        {
            printf("listen failed\n");
            return -1;
        }

        FD_ZERO(&read_set);
        FD_SET(listenfd, &read_set);

        int sockets[MAXROUTER];
        memset(sockets, 0, sizeof(sockets));
        int maxfd = 0;
        int new_socket = 0;
        while(1)
        {
            ready_set = read_set;
            maxfd = listenfd;
            for(int i=0; i<MAXROUTER; i++)
            {
                if(sockets[i])
                    FD_SET(sockets[i], &ready_set);
                if(sockets[i] > maxfd)
                    maxfd = sockets[i];
            }
            printf("blocking on select..\n");
            select(maxfd + 1, &ready_set, NULL, NULL, NULL);
            printf("stop blocking on select..\n");
            if(FD_ISSET(listenfd, &ready_set))
            {
                if((new_socket = accept(listenfd, (struct sockaddr *)&addr, (socklen_t*)&addrlen)) < 0)
                {
                    printf("accept failed\n");
                    return -1;
                }
                printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(addr.sin_addr) , (addr.sin_port));

                struct Packet packet;
                memset(&(packet), 0, sizeof(packet));
                recv(new_socket, &(packet), sizeof(packet), 0);

                doit(&packet, router_id, new_socket);
                close(new_socket);
                /*
                for(int i=0; i<MAXROUTER; i++)
                {
                    if(sockets[i] == 0)
                    {
                        sockets[i] = new_socket;
                        printf("new socket %d has added to cache\n", new_socket);
                        break;
                    }
                }*/
            }
            else
            {
                for(int i=0; i<MAXROUTER; i++)
                {
                    int new_fd = sockets[i];
                    if(FD_ISSET(new_fd, &ready_set))
                    {
                        printf("receiving message from fd = %d\n", new_fd);
                        struct Packet packet;
                        memset(&(packet), 0, sizeof(packet));
                        recv(new_socket, &(packet), sizeof(packet), 0);

                        doit(&packet, router_id, new_socket);

                        close(new_fd);
                        sockets[i] = 0;
                    }
                }
            }
        }
    }
    

    return 1;
}

int main(int argc, char **argv)
{
    printf("./router <router location file> <topology conf file> <router id>\n");
    char *location = argv[1];
    char *topology = argv[2];
    char *router_id = argv[3];
    max_router_id = parse_location(location, routers);
    topo_num = parse_topology(topology, topos);
    initialize_distance(atoi(router_id));

    router(atoi(router_id));
}