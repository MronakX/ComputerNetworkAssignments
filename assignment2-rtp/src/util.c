#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>

#include "util.h"
#include "rtp.h"
#include <sys/select.h>
#include <sys/time.h>

uint32_t crc32_for_byte(uint32_t r) {
    for(int j = 0; j < 8; ++j)
        r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
    return r ^ (uint32_t)0xFF000000L;
}

void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
    static uint32_t table[0x100];
    if(!*table)
        for(size_t i = 0; i < 0x100; ++i)
            table[i] = crc32_for_byte(i);
    for(size_t i = 0; i < n_bytes; ++i)
        *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}

uint32_t compute_checksum(const void* pkt, size_t n_bytes) {
    uint32_t crc = 0;
    crc32(pkt, n_bytes, &crc);
    return crc;
}

/* select returns 0 if timeout, 1 if input available, -1 if error. */
int timeout(int sockfd)
{
    fd_set fdset;
    struct timeval timeout;

    FD_ZERO (&fdset);
    FD_SET(sockfd, &fdset);

  /* Initialize the timeout data structure. */
  timeout.tv_sec = 0;
  timeout.tv_usec = 500;

  /* select returns 0 if timeout, 1 if input available, -1 if error. */
  return (select (sockfd+1, &fdset, NULL, NULL, &timeout));
}

void print_ACK(char* buffer)
{
    rtp_header_t *rtp = (rtp_header_t *)buffer;
    printf("[log] [receive ACK]: type = %d, len = %d, seq = %d\n\n", rtp->type, rtp->length, rtp->seq_num);
    return;
}

void print_recv_msg(char* buffer)
{
    rtp_header_t *rtp = (rtp_header_t *)buffer;
    char *pkt = buffer + sizeof(rtp_header_t);
    printf("[log] [receive msg]:");
    printf("ACK: type = %d, len = %d, seq = %d\n", rtp->type, rtp->length, rtp->seq_num);
    //printf("checksum = %d\n", rtp->checksum);
    printf("\n");
    //printf("message: %s\n\n", pkt);
}

void print_send_msg(char* buffer)
{
    rtp_header_t *rtp = (rtp_header_t *)buffer;
    char *pkt = buffer + sizeof(rtp_header_t);
    printf("[log] [send msg]:");
    printf("ACK: type = %d, len = %d, seq = %d\n", rtp->type, rtp->length, rtp->seq_num);
    //printf("checksum = %d\n", rtp->checksum);
    printf("\n");
    //printf("message: %s\n\n", pkt);
}

