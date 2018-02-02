#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/select.h>
#include<stdlib.h>

int fds[sizeof(fd_set)*8];

static usage(const char* proc)
{
    printf("Usage: %s [local_ip] [[local_port]\n",proc);
}

int startup(const char* ip,int port)
{
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
    {
        perror("socket");
        exit(2);
    }
    int opt = 1;
    //立即重启
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));



    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = inet_addr(ip);

    if(bind(sock,(struct sockaddr*)&local,sizeof(local)) < 0)
    {
        perror("bind");
        exit(3);
    }
    if(listen(sock,10) < 0)
    {
        perror("listen");
        exit(4);
    }
    return sock;
}

int main(int argc,char* argv[])
{
    if(argc != 3)
    {
        usage(argv[0]);
        return 1;
    }
    int listen_socket = startup(argv[1],atoi(argv[2]));

    printf("fd_set:%d\n",sizeof(fd_set)*8);
    int i = 0;
    int nums = sizeof(fds)/sizeof(fds[0]);
    for(;i<nums;i++)
    {
        fds[i] = -1;
    }
    fds[0] = listen_socket;//read
    int maxfd = -1;
    fd_set rfds;
    while(1)
    {
        struct timeval timeout = {2,0};
        FD_ZERO(&rfds);
        i = 0;
        for(;i <nums;i++)
        {
            if(fds[i] == -1)
            {
                continue;
            }
        FD_SET(fds[i],&rfds);
        if(maxfd < fds[i])
            maxfd = fds[i];
         }

        switch(select(maxfd+1,&rfds,NULL,NULL,&timeout))
         {
            case -1:
                perror("select");
                 break;
             case 0:
                 printf("time out..\n");
                 break;
        
             default:
             {
            //at least ont fd ready
                 i = 0;
                 for(; i <nums;++i)
                 {
                     //listen_socket ready,get connect
                     if( i == 0 && FD_ISSET(fds[i],&rfds) )
                     {
                        struct sockaddr_in client;
                        socklen_t len = sizeof(client);
                        int new_sock = accept(listen_socket,(struct sockaddr*)&client,&len);
                         if(new_sock < 0)
                         {
                             perror("acept");
                             continue;
                         }
                         //调试信息

                        printf("get a client:[%s:%d]\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));


                         int j = 1;
                         for(;j<nums;j++)
                         {
                             if(fds[j] == -1)
                             {
                                 break;
                             }
                         }
                         if(j == nums)
                            close(new_sock);
                         else
                         {
                             fds[j] = new_sock;
                         }
                     }
                     else if(i != 0 && FD_ISSET(fds[i],&rfds))
                     {
                         //normal fd read evs ready
                         char buf[1024];
                         ssize_t s = read(fds[i],buf,sizeof(buf)-1);
                         if(s > 0)
                         {
                             buf[s] = 0;
                             printf("client# %s\n",buf);
                         }
                         else if(s == 0)
                         {
                             printf("client quit!\n");
                             close(fds[i]);
                             fds[i] = -1;
                         }
                         else
                         {
                             perror("read");
                             close(fds[i]);
                             fds[i] = -1;
                         }
                     }
                 }
             }
             break;
        }
    }
    return 0;
}

