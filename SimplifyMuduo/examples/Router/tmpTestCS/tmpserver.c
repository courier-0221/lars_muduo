#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>

typedef struct 
{
	int32_t msgId;
	int32_t msgLen;
}MsgHead;

int main()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd!=-1);
    struct sockaddr_in ser,cli;
    memset(&ser,0,sizeof(ser));
    
    ser.sin_family=AF_INET;
    ser.sin_port=htons(11111);
    ser.sin_addr.s_addr=inet_addr("127.0.0.1");
    
    int res=bind(sockfd,(struct sockaddr*)&ser,sizeof(ser));
    
    listen(sockfd,5);
    
    while(1)
    {
        int len=sizeof(cli);
        int c=accept(sockfd,(struct sockaddr*)&cli,&len);
        if(c<0)
        {
            perror("error\n");
            continue;
        }
        while(1)
        {
			MsgHead readhead;
            char recvbuf[128]={0};
            int n=recv(c,recvbuf,127,0);
            if(n<=0)
            {
                printf("one client over\n");
                break;
            }
            memcpy(&readhead, recvbuf, 8);
			printf("recv data size.%d id.%d len.%d str.%s\n", n, readhead.msgId, readhead.msgLen, recvbuf+8);
        }
        close(c);
    }
    close(sockfd);
}
