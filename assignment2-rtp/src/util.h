#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

#include <sys/types.h>
#include <sys/socket.h>

uint32_t compute_checksum(const void* pkt, size_t n_bytes);

int timeout(int sockfd);

void print_ACK(char *buffer);

void print_recv_msg(char* buffer);

void print_send_msg(char* buffer);


#endif