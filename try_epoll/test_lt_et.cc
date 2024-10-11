#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <arpa/inet.h>

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

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

void test(epoll_event *events, int number, int epollfd, int listenfd, bool use_et)
{
    char buff[BUFFER_SIZE];
    for (int i = 0; i < number; ++i)
    {
        int sockfd = events[i].data.fd;

        if (sockfd == listenfd)
        {
            struct sockaddr_in client_address;
            unsigned int client_addrlength{sizeof(client_address)};
            int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            addfd(epollfd, connfd, use_et);
        }
        else if (events[i].events & EPOLLIN)
        {
            printf("event trigger once\n");
            if (!use_et)
            {
                printf("lt start!\n");
                memset(buff, '\0', BUFFER_SIZE);
                int ret = recv(sockfd, buff, BUFFER_SIZE - 1, 0);
                if (ret <= 0)
                {
                    close(sockfd);
                    continue;
                }
                printf("get %d bytes of content: %s\n", ret, buff);
            }
            else
            {
                printf("et start!\n");
                while (1)
                {
                    memset(buff, '\0', BUFFER_SIZE);
                    int ret = recv(sockfd, buff, BUFFER_SIZE - 1, 0);
                    if (ret < 0)
                    {
                        if ((errno == EAGAIN) || errno == EWOULDBLOCK)
                        {
                            printf("read later\n");
                            break;
                        }
                        close(sockfd);
                        break;
                    }
                    else if (ret == 0)
                    {
                        close(sockfd);
                    }
                    else
                    {
                        printf("get %d bytes of content: %s\n", ret, buff);
                    }
                }
            }
        }
        else
        {
            printf("shit happened\n");
        }
    }
}
int main(int argc, char *argv[])
{
    if (argc <= 3)
    {
        printf("usage: %s ip port use_et\n", basename(argv[0]));
    }
    const auto ip{argv[1]};
    const auto port{atoi(argv[2])};
    const auto use_et{atoi(argv[3])};
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, 1);
    while (1)
    {
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (ret < 0)
        {
            printf("epoll failed\n");
            break;
        }
        test(events, ret, epollfd, listenfd, use_et);
    }
    close(listenfd);
}