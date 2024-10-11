#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define BUFFER_SIZE 500
int setnonblock(int fd)
{
    int op = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, op | O_NONBLOCK);
    return op;
}
void addfd(int epollfd, int fd, bool use_et)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (use_et)
    {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblock(fd);
}
int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip port\n", basename(argv[0]));
        return 1;
    }
    const auto ip{argv[1]};
    const auto port{atoi(argv[2])};

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);
    server_address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("socket create failed\n");
        exit(-1);
    }

    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("connection failed\n");
        close(sockfd);
        return 1;
    }

    int epfd = epoll_create(5);
    if (epfd < 0)
    {
        printf("epoll create failed..\n");
        exit(-1);
    }
    struct epoll_event events[2];
    addfd(epfd, 0, 1);
    addfd(epfd, sockfd, 1);
    char readbuff[BUFFER_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    if (ret < 0)
    {
        printf("pipe create failed\n");
        exit(-1);
    }
    while (1)
    {
        int res = epoll_wait(epfd, events, 2, -1);
        if (res < 0)
        {
            printf("epoll failed\n");
            break;
        }
        for (int i = 0; i < res; ++i)
        {
            int cur_fd = events[i].data.fd;
            if (cur_fd == 0)
            {
                ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
                ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            }
            else
            {
                memset(readbuff, '\0', BUFFER_SIZE);
                recv(cur_fd, readbuff, BUFFER_SIZE - 1, 0);
                printf("%s\n", readbuff);
            }
        }
    }
    close(sockfd);
    close(epfd);
    // int pid;
    // int pipe_fd[2];
    // char message[BUFFER_SIZE];
    // int epoll_events_count;
    // static struct epoll_event events[2];
    // if (pipe(pipe_fd) < 0)
    // {
    //     printf("create pipe fail.\n");
    //     exit(-1);
    // }
    // // 创建epoll instance
    // int epfd = epoll_create(1000);
    // if (epfd < 0)
    // {
    //     printf("create epoll instance fail.\n");
    //     exit(-1);
    // }
    // // 添加sockt 和 管道读文件描述符到 interest list
    // addfd(epfd, sockfd, 1);
    // addfd(epfd, pipe_fd[0], 1);
    // int isClientWork = 1;
    // pid = fork();
    // if (pid < 0)
    // {
    //     printf("fork fail.\n");
    //     exit(-1);
    // }
    // else if (pid == 0)
    // {
    //     // 这里是子进程
    //     // 子进程关闭读端
    //     close(pipe_fd[0]);
    //     printf("input 'exit' if you want quit chat room.\n");
    //     while (isClientWork)
    //     {
    //         bzero(&message, BUFFER_SIZE);
    //         // 从标准输入读取数据
    //         fgets(message, BUFFER_SIZE, stdin);
    //         // 判断是否输入了'exit'
    //         if (strncasecmp(message, "exit", 4) == 0)
    //         {
    //             isClientWork = 0;
    //         }
    //         else
    //         {
    //             // 从标准输入读取的数据写入管道
    //             if (write(pipe_fd[1], message, strlen(message) - 1) < 0)
    //             {
    //                 printf("client write pip fail.\n");
    //                 exit(-1);
    //             }
    //         }
    //     }
    // }
    // else
    // {
    //     // 这里是父进程
    //     // 父进程关闭写端
    //     close(pipe_fd[1]);
    //     while (isClientWork)
    //     {
    //         epoll_events_count = epoll_wait(epfd, events, 2, -1);
    //         // 处理事件
    //         for (int i = 0; i < epoll_events_count; i++)
    //         {
    //             bzero(&message, BUFFER_SIZE);
    //             // 服务端发来的消息
    //             if (events[i].data.fd == sockfd)
    //             {
    //                 if (recv(sockfd, message, BUFFER_SIZE, 0) == 0)
    //                 {
    //                     printf("server error\n");
    //                     isClientWork = 0;
    //                     close(sockfd);
    //                 }
    //                 else
    //                 {
    //                     printf("%s\n", message);
    //                 }
    //             }
    //             else
    //             {
    //                 // 子进程写管道事件
    //                 // 从管道中读取数据
    //                 if (read(events[i].data.fd, message, BUFFER_SIZE) == 0)
    //                 {
    //                     printf("parent read pipe fail\n");
    //                     isClientWork = 0;
    //                 }
    //                 else
    //                 {
    //                     send(sockfd, message, BUFFER_SIZE, 0);
    //                 }
    //             }
    //         }
    //     }
    // }
    // if (pid > 0)
    // {
    //     close(pipe_fd[0]);
    //     close(sockfd);
    // }
    // else
    // {
    //     close(pipe_fd[1]);
    // }
}