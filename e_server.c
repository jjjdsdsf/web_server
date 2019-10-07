#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
//epoll版服务端
int main(int argc,char* argv[])
{
    if(argc < 2){
        printf("Lack port ...\n");
        exit(1);
    }
    //创建套接字
    int lfd = socket(AF_INET,SOCK_STREAM,0);
    if(lfd == -1){
        perror("socket error:");
        exit(1);
    }
    //绑定ip和端口
    struct sockaddr_in ser;
    memset(&ser,0,sizeof(ser));
    ser.sin_family = AF_INET;
    ser.sin_port = htons(atoi(argv[1]));
    ser.sin_addr.s_addr = htonl(INADDR_ANY);
    //设置端口复用
    int flag = 1;
    int ret = setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));
    if(-1 == ret){
        perror("setsockopt error:");
        exit(1);
    }
    //绑定
    ret = bind(lfd,(struct sockaddr*)&ser,sizeof(ser));
    if(-1 == ret){
        perror("bind error:");
        exit(1);
    }
    //监听
    ret = listen(lfd,36);
    if(-1 == ret){
        perror("listen error:");
        exit(1);
    }
    struct sockaddr_in cli;
    socklen_t len = sizeof(cli);
    //创建树的根节点
    int epfd = epoll_create(3000);
    if(-1 == epfd){
        perror("epoll_create error:");
        exit(1);
    }
    struct epoll_event ev;
    ev.data.fd = lfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);
    struct epoll_event all[1024];
    while(1){
        //内核检测
        ret = epoll_wait(epfd,all,1024,-1);
        if(-1 == ret){
            perror("epoll_wait error:");
            exit(1);
        }
        //遍历all数组前ret个
        for(int i = 0;i < ret;++i){
            int fd = all[i].data.fd;
            //判断是否有新连接
            if(fd == lfd){
                //接收连接
                int cfd = accept(lfd,(struct sockaddr*)&cli,&len);
                if(cfd == -1){
                    perror("accept error:");
                    exit(1);
                }
                char ipbuf[128] = {0};
                printf("ip : %s  , port : %d  连接成功\n",inet_ntop(AF_INET,&cli.sin_addr.s_addr,ipbuf,sizeof(ipbuf)),ntohs(cli.sin_port));
                //挂到树上
                struct epoll_event newev;
                newev.events = EPOLLIN;
                newev.data.fd = cfd;
                epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&newev);

            }else{
                //判断是否是读操作
                if(!(all[i].events & EPOLLIN)){
                    continue;
                }
                char buf[128] = {0};
                int rnum = recv(fd,buf,sizeof(buf),0);
                if(rnum < 0){
                    perror("recv error:");
                    exit(1);
                }else if(rnum == 0){
                    printf("断开连接,...\n");
                    //把树上删除
                    struct epoll_event delev;
                    delev.data.fd = fd;
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&delev);
                    close(fd);
                    break;
                }else{
                    printf("收到了 : %s\n",buf);
                    //转大写
                    for(int i = 0;i < strlen(buf);++i){
                        buf[i] = toupper(buf[i]);
                    }
                    //发送
                    send(fd,buf,sizeof(buf),0);
                }
            }
        }
    }

    return 0;
}

