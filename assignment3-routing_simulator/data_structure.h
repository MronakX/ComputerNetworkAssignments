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
#include <algorithm>

#define MAXLINE 512
#define MAXROUTER 20
#define MAXTOPO 110

#define CMD_DV 0
#define CMD_UPDATE 1
#define CMD_SHOW 2
#define CMD_RESET 3
#define CMD_PROP 4

#define SENDER_AGENT 0
#define SENDER_ROUTER 1

/* @param
 * router_id: not hashing
 * next_hop: not hashing
 * distance: integer
 */
struct RoutingTable
{
    int router_id;
    int next_hop;
    int distance;
};


/* @param
 * hash_id: 0-10
 * router_id: integer
 * cost: array, index is hash_id
 * neighbors: vector, hash_id inside
 * distance:2-D array, index is hash_id
 * routing_table: struct RoutingTable
 */
struct Node
{
    int router_id;
    int cost[MAXROUTER];
    std::vector<int> neighbors;
    int distance[MAXROUTER][MAXROUTER];
    struct RoutingTable routing_table[MAXROUTER];
};

struct Router
{
    char router_ip[MAXLINE];
    int router_port;
    int router_id;
};

struct Topology
{
    int router_id_1;
    int router_id_2;
    int weight;  
};

struct Command
{
    int type;
    int router_id_1;
    int router_id_2;
    int weight;
    int sender_type;
};

struct ShowCommand
{
    int dest[MAXROUTER];
    int next[MAXROUTER];
    int cost[MAXROUTER];
};

struct Packet
{
   int type;
   char buffer[128];
   int router_id;
   int distance[MAXROUTER];
};