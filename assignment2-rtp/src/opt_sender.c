#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "util.h"
#include "opt_rtp.h"


int sender(char *receiver_ip, char* receiver_port, int window_size, char* message){

  // create socket
  int sock = 0;
  if ((sock = rtp_socket(window_size)) < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
  // create receiver address
  struct sockaddr_in receiver_addr;
  memset(&receiver_addr, 0, sizeof(receiver_addr));
  receiver_addr.sin_family = AF_INET;
  receiver_addr.sin_port = htons(atoi(receiver_port));

  // convert IPv4 or IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, receiver_ip, &receiver_addr.sin_addr)<=0) {
    perror("address failed");
    exit(EXIT_FAILURE);
  }

  // connect to server
  rtp_connect(sock, (struct sockaddr *)&receiver_addr, sizeof(struct sockaddr));

  // send data
  char test_data[] = "Hello, world!\n";


  char line[MAXLINE];
  char buf[MAXSIZE];
  // TODO: if message is filename, open the file and send its content
  int file_fd;
  size_t nn;
  int current_seq = 0;
  if((file_fd = open(message, O_RDONLY)) > 0)// a file
  {
    printf("open file successfully\n");
    while((nn = read(file_fd, buf, MAXSIZE)) != 0)
    { 
      int tmp = rtp_sendto(sock, (void *)buf, strlen(buf), 0, (struct sockaddr*)&receiver_addr, 
                sizeof(struct sockaddr), current_seq);
      //printf("%s\n\n", buf);
      printf("strlen = %d\n", strlen(buf));
      printf("sendto return num = %d\n", tmp);
      printf("rtp_sendto case finished 1 time\n");
      current_seq += tmp;
      printf("current seq = %d\n", current_seq);
      memset(buf, 0, sizeof(buf));
    }
    rtp_send_END(sock, (struct sockaddr*)&receiver_addr, sizeof(struct sockaddr), 
                current_seq+1);
  }
  else//not a file
  {
    rtp_sendto(sock, (void *)message, strlen(message), 0, (struct sockaddr*)&receiver_addr, sizeof(struct sockaddr), current_seq);
    rtp_send_END(sock, (struct sockaddr*)&receiver_addr, sizeof(struct sockaddr), 
                current_seq+1);
  }

  // close rtp socket
  rtp_close(sock);
  printf("sender has made it to the END!\n");
  return 0;
}



/*
 * main()
 * Parse command-line arguments and call sender function
*/
int main(int argc, char **argv) {
  char *receiver_ip;
  char *receiver_port;
  int window_size;
  char *message;

  if (argc != 5) {
    fprintf(stderr, "Usage: ./sender [Receiver IP] [Receiver Port] [Window Size] [message]");
    exit(EXIT_FAILURE);
  }

  receiver_ip = argv[1];
  receiver_port = argv[2];
  window_size = atoi(argv[3]);
  message = argv[4];
  return sender(receiver_ip, receiver_port, window_size, message);
}