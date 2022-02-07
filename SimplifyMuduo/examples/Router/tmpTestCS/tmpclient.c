#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
 
typedef struct 
{
	int32_t msgId;
	int32_t msgLen;
}MsgHead;
 
#define BUF_SIZE 128
void client_job(int sd);
int main(int argc, char **argv) {
  //命令行传ｉｐ　和　端口
  int sd;
  struct sockaddr_in raddr;
  if (argc < 3) {
    fprintf(stderr, "Usage...\n");
    exit(1);
  }
  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    perror("socket()");
    exit(1);
  }
  raddr.sin_family = AF_INET;
  inet_pton(AF_INET, argv[1], &raddr.sin_addr);
  raddr.sin_port = htons(atoi(argv[2]));
  if (connect(sd, (void *)&raddr, sizeof(raddr)) < 0) {
    perror("connect()");
    exit(1);
  }
  client_job(sd);
  close(sd);
  exit(0);
}

void client_job(int sd) {
  char send_buf[BUF_SIZE];

  MsgHead head;
  MsgHead readhead;
  
  head.msgId = 111;
  head.msgLen = 5;
  memcpy(send_buf, &head, 8);
  memcpy(send_buf + 8, "12345", 5);
  printf("id.%d len.%d str.%s\n", head.msgId, head.msgLen, "12345");
  while (1) 
  {
	  int n = send(sd, send_buf, 8+5, 0);
	  memcpy(&readhead, send_buf, 8);
	  
    printf("send data size.%d id.%d len.%d str.%s\n", n, readhead.msgId, readhead.msgLen, send_buf+8); 
    sleep(1);
  }
}