#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>

#include "rtp.h"
#include "util.h"
#include <sys/select.h>
#include <sys/time.h>


void rcb_init(uint32_t window_size) {
    if (rcb == NULL) {
        rcb = (rcb_t *) calloc(1, sizeof(rcb_t));
    } else {
        perror("The current version of the rtp protocol only supports a single connection");
        exit(EXIT_FAILURE);
    }
    rcb->window_size = window_size;
    // TODO: you can initialize your RTP-related fields here
}

/*********************** Note ************************/
/* RTP in Assignment 2 only supports single connection.
/* Therefore, we can initialize the related fields of RTP when creating the socket.
/* rcb is a global varialble, you can directly use it in your implementatyion.
/*****************************************************/
int rtp_socket(uint32_t window_size) {
    rcb_init(window_size); 
    // create UDP socket
    return socket(AF_INET, SOCK_DGRAM, 0);  
}


int rtp_bind(int sockfd, struct sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

//TODO, done tmply
int rtp_listen(int sockfd, struct sockaddr *addr, socklen_t addrlen, int backlog) {
    // TODO: listen for the START message from sender and send back ACK
    // In standard POSIX API, backlog is the number of connections allowed on the incoming queue.
    // For RTP, backlog is always 1

    char buffer[2048];
    int recv_bytes = recvfrom(sockfd, buffer, 2048, 0, addr, (socklen_t *)&addrlen);
    if (recv_bytes < 0) {
        perror("receive error");
        exit(EXIT_FAILURE);
    }
    buffer[recv_bytes] = '\0';
    print_recv_msg(buffer);

    // extract header
    rtp_header_t *recv_rtp = (rtp_header_t *)buffer;

    if(check_checksum(recv_rtp, buffer, recv_bytes) == -1)
    {
        perror("START msg checksum failed\n");
        return -1;
    }


    // send back ACK
    char send_buffer[BUFFER_SIZE];
    rtp_header_t* send_rtp = (rtp_header_t*)buffer;
    send_rtp->length = 0;
    send_rtp->checksum = 0;
    send_rtp->seq_num = recv_rtp->seq_num;
    send_rtp->type = RTP_ACK;
    send_rtp->checksum = compute_checksum((void *)buffer, sizeof(rtp_header_t) + 0);

    int sent_bytes = sendto(sockfd, (void*)buffer, sizeof(rtp_header_t) + 0, 0, addr, addrlen);
    print_send_msg(buffer);
    if (sent_bytes != (sizeof(struct RTP_header) + 0)) {
        perror("send error");
        exit(EXIT_FAILURE);
        return -1;
    }
    return 0;
    
}

// done
int rtp_accept(int sockfd, struct sockaddr *addr, socklen_t addrlen) {
    // Since RTP in Assignment 2 only supports one connection,
    // there is no need to implement accpet function.
    // You don’t need to make any changes to this function.

    return 1;
}

//TODO
int rtp_send_END(int sockfd, struct sockaddr *to, socklen_t tolen, int seq_num)
{
    char buffer[BUFFER_SIZE];
    int len = 0;
    int flags = 0;

    rtp_header_t* rtp = (rtp_header_t*)buffer;
    rtp->length = len;
    rtp->checksum = 0;
    rtp->seq_num = seq_num;
    rtp->type = RTP_END;
    rtp->checksum = compute_checksum((void *)buffer, sizeof(rtp_header_t) + len);

    int sent_bytes = sendto(sockfd, (void*)buffer, sizeof(rtp_header_t) + len, flags, to, tolen);
    if (sent_bytes != (sizeof(struct RTP_header) + len)) {
        perror("send error");
        exit(EXIT_FAILURE);
    }
    print_send_msg(buffer);
    //receive ACK until get a END ack.
    while(1)
    {
        int ret = timeout(sockfd);
        if(ret == 0)//END ACK lost
        {
            perror("END ACK has lost, shut down the process");
            rtp_close(sockfd);
            exit(EXIT_FAILURE);
        }
        else if(ret == -1)
        {
            perror("select error occured!");
            exit(EXIT_FAILURE);
        }
        else
        {   
            int recv_seq = receive_ACK(sockfd, to, tolen);
            if(recv_seq != rtp->seq_num)// other ACK, drop it
                continue;
            else if(recv_seq == rtp->seq_num && recv_seq == -1)//END ACK broken because checksum not equal
            {
                perror("END ACK has broken, shut down the process");
                rtp_close(sockfd);
                exit(EXIT_FAILURE);
            }
            else //receive the true END ACK
            {
                printf("receive good END ACK\n");
                rtp_close(sockfd);
                return 1;
            }
        }
    }
    return -1;
}

//TODO, done tmply
int rtp_connect(int sockfd, struct sockaddr *addr, socklen_t addrlen) {
    // TODO: send START message and wait for its ACK

    //send START
    char buffer[BUFFER_SIZE];
    rtp_header_t* rtp = (rtp_header_t*)buffer;

    rtp->type = RTP_START;
    rtp->length = 0;
    srand(time(NULL));
    rtp->seq_num = rand()+1;
    rtp->checksum = compute_checksum((void*)buffer, sizeof(rtp_header_t) + rtp->length);

    int sent_bytes = sendto(sockfd, (void*)buffer, sizeof(rtp_header_t) + rtp->length,
                            0, addr, addrlen);
    
    if(sent_bytes != (sizeof(struct RTP_header) + rtp->length))
    {
        printf("sent_bytes = %d\n", sent_bytes);
        printf("actual bytes = %ld\n", (sizeof(struct RTP_header) + rtp->length));
        perror("send error in rtp_connect");
        exit(EXIT_FAILURE);
    }
    print_send_msg(buffer);

    //check if overtime
    int ret = timeout(sockfd);
    printf("blocking on select..\n");
    if(ret == -1)
    {
        perror("select error occured!");
        exit(EXIT_FAILURE);
    }
    if(ret == 0)//overtime, send END
    {
        perror("START ack overtime, sending END..");
        if(rtp_send_END(sockfd, addr, addrlen, 0) == -1)
        {
            perror("connect failed, failed shuting down sender\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            perror("connect failed, received good END ACK");
            exit(EXIT_SUCCESS);
        }
    }

    printf("receive an ACK, stop blocking on select!\n");

    //receive ACK from receiver
    int seq_num = receive_ACK(sockfd, addr, addrlen);

    if(seq_num == rtp->seq_num)// ACK received, connect successfully.
    {
        printf("receiving correct START ACK, connect successfully\n");
    }
    else if((seq_num != rtp->seq_num) || seq_num == -1)//broken or seq not equal, send END
    {
        perror("START ack broke, sending END..");
        if(rtp_send_END(sockfd, addr, addrlen, rtp->seq_num+1) == -1)
        {
            perror("connect failed, failed shuting down sender\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("connect failed, receiving good END ACK, exit with success\n");
            rtp_close(sockfd);
            exit(EXIT_SUCCESS);
        }
    }
    printf("returning from rtp_connect\n");
    return 1;
}


// return rtp's seq_num
int check_seq_num(rtp_header_t *rtp)
{
    printf("checking ACK seq...\n");
    printf("seq num is %d\n", rtp->seq_num);

    return rtp->seq_num;
}


// return -1 if not equal;
// return 1 if equal.
int check_checksum(rtp_header_t *rtp, char* buffer, int recv_bytes)
{
    uint32_t checksum_1 = rtp->checksum;
    rtp->checksum = 0;
    uint32_t computed_checksum = compute_checksum(buffer, recv_bytes);
    //printf("ori = %u, new = %u\n", checksum_1, computed_checksum);
    if(checksum_1 != computed_checksum)
    {
        perror("checksums not match");
        return -1;
    }
    return 1;
}

// return rtp's seq_num 
// return -1 if checksum failed
int receive_ACK(int sockfd, struct sockaddr *receiver_addr, socklen_t receiver_addrlen)
{
    char buffer[2048];
    int flags = 0;
    int recv_bytes = recvfrom(sockfd, buffer, 2048, flags, receiver_addr, (socklen_t *)&receiver_addrlen);
    if (recv_bytes < 0) {
        perror("receive error");
        exit(EXIT_FAILURE);
    }
    buffer[recv_bytes] = '\0';
    print_ACK(buffer);
    // extract header
    rtp_header_t *rtp = (rtp_header_t *)buffer;
    
    //verify ACK
    if(rtp->type != RTP_ACK)
    {
        perror("Receiver sent not-ACK message, this WON'T happen!\n");
        exit(EXIT_FAILURE);
    }

    // verify checksum
    if(check_checksum(rtp, buffer, recv_bytes) == -1)
    {
        perror("checksum failed when receiving ACK, return -1 from receive_ACK()");
        return -1;
    }
    return rtp->seq_num;
}


int rtp_close(int sockfd) {
    return close(sockfd);
}


int rtp_sendto(int sockfd, const void *msg, int len, int flags, const struct sockaddr *to, 
                socklen_t tolen, int start_seq) {
    // TODO: send message
    //printf("%s\n", msg);
    char packets[MAX_PACKET_NUM][MAXLINE+1];
    memset(packets, 0, sizeof(packets)); 
    int cnt = 0;
    int bytes_cnt = 0;
    while(bytes_cnt < len)
    {
        if(bytes_cnt + MAXLINE < len)
        {
            strncpy(packets[cnt], msg, MAXLINE);
            msg += MAXLINE;
            cnt++;
            packets[cnt][MAXLINE] = '\0';
            bytes_cnt += MAXLINE;
        }
        else
        {
            strcpy(packets[cnt], msg);
            bytes_cnt += MAXLINE;
        }
    }
    //printf("%s\n", packets[12]);

    int windowsize = rcb->window_size;
    int base = start_seq;
    int nextseqnum = start_seq;
    int isTimeout = 0;

    int padding = 1;
    if(len%MAXLINE == 0)
    {
        padding = 0;
    }
    int endnum = (len/MAXLINE)+ start_seq + padding;
    printf("endnum = %d\n", endnum);
    while(base != endnum)
    {
        //printf("base = %d, endnum = %d\n", base, endnum);
        while(nextseqnum < base + windowsize && nextseqnum < endnum)
        {
            char buffer[BUFFER_SIZE];
            rtp_header_t* rtp = (rtp_header_t*)buffer;
            rtp->length = strlen(packets[nextseqnum-start_seq]);
            rtp->checksum = 0;
            rtp->seq_num = nextseqnum;
            rtp->type = RTP_DATA;
            memcpy((void *)buffer+sizeof(rtp_header_t), packets[nextseqnum-start_seq], rtp->length);
            rtp->checksum = compute_checksum((void *)buffer, sizeof(rtp_header_t) + rtp->length);
            sendto(sockfd, (void*)buffer, sizeof(rtp_header_t) + rtp->length, 0, to, tolen);
            print_send_msg(buffer);
            nextseqnum++;
        }
        //一定保证运行到这里时，窗口大小为n（或者已经到终点）

        //select timeout
        
        isTimeout = timeout(sockfd);
        
        while(isTimeout == 0)//超时 无任何收取
        {
            printf("[log] [resend]: timeout, resend all outstanding packets\n");
            printf("base = %d, endnum = %d\n", base, endnum);
            for(int i = base-start_seq; i<nextseqnum-start_seq; i++)
            {
                //sendto(i);
                char buffer[BUFFER_SIZE];
                rtp_header_t* rtp = (rtp_header_t*)buffer;
                rtp->length = strlen(packets[i]);
                rtp->checksum = 0;
                rtp->seq_num = i;
                rtp->type = RTP_DATA;
                memcpy((void *)buffer+sizeof(rtp_header_t), packets[i], rtp->length);
                rtp->checksum = compute_checksum((void *)buffer, sizeof(rtp_header_t) + rtp->length);
                sendto(sockfd, (void*)buffer, sizeof(rtp_header_t) + rtp->length, 0, to, tolen);
                print_send_msg(buffer);
            }
            isTimeout = timeout(sockfd);
        }

        //有recv
        //isTimeout = timeout(sockfd);
        //printf("timeout = %d\n", isTimeout);
        while(isTimeout == 1)
        {
            //recvfrom
            char buffer[BUFFER_SIZE];
            int recv_bytes = recvfrom(sockfd, buffer, BUFFER_SIZE, flags, NULL, NULL);
            if (recv_bytes < 0) {
                perror("receive error");
                exit(EXIT_FAILURE);
            }
            buffer[recv_bytes] = '\0';
            print_ACK(buffer);
            // extract header
            rtp_header_t *recv_rtp = (rtp_header_t *)buffer;
            int isChecksumLegal = check_checksum(recv_rtp, buffer, recv_bytes);
            int ACK_seq = recv_rtp->seq_num;

            if(isChecksumLegal == 1 && ACK_seq >= base)//窗口一定前进
            {
                printf("[log]: window move forward, base = %d, last = %d\n", base, nextseqnum);
                base = ACK_seq;
                isTimeout = timeout(sockfd);
                //if(base == nextseq)   
            }
            else if(isChecksumLegal == -1 || ACK_seq < base)
            {
                printf("[log]: illegal ACK, discard\n");
                isTimeout = timeout(sockfd);
                continue;
            }
            isTimeout = timeout(sockfd);
        }
        //没有可recv的，再次循环。
    }
    //printf("%c%c%c\n", msg, msg+1, msg+2);
    return base - start_seq;
}

int rtp_recvfrom(int sockfd, void *buf, int len, int flags,  struct sockaddr *from, 
                socklen_t *fromlen, char *file_name) {
    // TODO: recv message
    printf("call recvfrom\n");
    int expect_seq_num = 0;
    int isTimeout = 0;
    isTimeout = timeout(sockfd);

    int highest_received_seq_num = -1;
    int write_fd = open(file_name, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    while(1)
    {
        char buffer[2048];
        int recv_bytes = recvfrom(sockfd, buffer, 2048, 0, from, fromlen);
        if (recv_bytes < 0) {
            perror("receive error");
            exit(EXIT_FAILURE);
        }
        buffer[recv_bytes] = '\0';
        print_recv_msg(buffer);
        
        rtp_header_t *rtp = (rtp_header_t *)buffer;
        int isChecksumEqual = check_checksum(rtp, buffer, recv_bytes);
        if((isChecksumEqual == 1) && rtp->type == RTP_END)
        {
            sendback_ACK(sockfd, rtp, buffer, rtp->seq_num, from, fromlen);
            break;
        }
        else if((isChecksumEqual == 1) && (rtp->seq_num == expect_seq_num))
        {
            expect_seq_num++;
            sendback_ACK(sockfd, rtp, buffer, expect_seq_num, from, fromlen);
            if(write(write_fd, (void*)buffer+sizeof(rtp_header_t), rtp->length) != -1)
            {
                printf("write success\n");
            }
        }
        else if(isChecksumEqual == 0)
        {
            printf("broken packet, do nothing \n");
            continue;
        }
        else
        {
            sendback_ACK(sockfd, rtp, buffer, expect_seq_num, from, fromlen);
        }
    }


    //memcpy(buf, buffer+sizeof(rtp_header_t), rtp->length);

    return 1;
}


int sendback_ACK(int sockfd, rtp_header_t* recv_rtp, char* buffer, int seqnum, struct sockaddr *to, socklen_t *tolen)
{
    char send_buffer[BUFFER_SIZE];
    rtp_header_t* send_rtp = (rtp_header_t*)send_buffer;
    send_rtp->length = 0;
    send_rtp->checksum = 0;
    send_rtp->seq_num = seqnum;
    send_rtp->type = RTP_ACK;
    send_rtp->checksum = compute_checksum((void *)send_buffer, sizeof(rtp_header_t) + 0);
    //printf("to %d\n", to);
    int sent_bytes = sendto(sockfd, (void*)send_buffer, sizeof(rtp_header_t) + 0, 0, to, sizeof(struct sockaddr));
    if (sent_bytes != (sizeof(struct RTP_header) + 0)) {
        perror("send error");
        //exit(EXIT_FAILURE);
        return -1;
    }
    print_send_msg(send_buffer);
    
    return 1;
}