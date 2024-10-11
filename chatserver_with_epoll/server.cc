#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_set>
#include <signal.h>

#define USER_LIMIT 5
#define BUFFER_SIZE 500
#define FD_LIMIT 65535
#define EPOLL_SIZE 1000

struct sockaddr_in server_address;
int listenfd, clientfd, epfd;
const char *TOO_LONG = "too many users\n";
const char *TOO_LONELY = "you are alone\n";
static epoll_event events[EPOLL_SIZE];
char msg[BUFFER_SIZE];
int epoll_event_count, user_count = 0;
unsigned int client_address_length;
int ret = 0;
std::unordered_set<int> users;
// struct client_data
// {
//     sockaddr_in address;
//     char *write_buffer;
//     char buffer[BUFFER_SIZE];
//     bool alive{0};
// };

// struct client_data users[FD_LIMIT];

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

int broadcast(int src_fd)
{
    int len;
    char what_i_said[BUFFER_SIZE];
    char what_you_look[BUFFER_SIZE * 2];
    bzero(&what_i_said, BUFFER_SIZE);
    bzero(&what_you_look, BUFFER_SIZE * 2);
    len = recv(src_fd, what_i_said, BUFFER_SIZE, 0);
    printf("%d %d\n", src_fd, len);
    if (len == 0)
    {
        close(src_fd);
        printf("%d is offline\n", src_fd);
        --user_count;
        users.erase(src_fd);
    }
    else
    {
        if (user_count == 1)
        {
            send(src_fd, TOO_LONELY, strlen(TOO_LONELY), 0);
            return len;
        }

        for (const auto &user : users)
        {
            if (user != src_fd)
            {
                sprintf(what_you_look, "client %d says>> %s\n", src_fd, what_i_said);
                if (send(user, what_you_look, strlen(what_you_look), 0) < 0)
                {
                    printf("broadcast failed\n");
                    exit(-1);
                }
            }
        }
    }
    return len;
}

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);
    if (argc <= 2)
    {
        printf("usage: %s ip port\n", basename(argv[0]));
        return 1;
    }
    const auto ip{argv[1]};
    const auto port{atoi(argv[2])};

    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);
    server_address.sin_port = htons(port);

    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        printf("create socket failed...\n");
        exit(-1);
    }
    printf("socket created...\n");
    ret = bind(listenfd, (struct sockaddr *)&server_address, sizeof(server_address));
    if (ret < 0)
    {
        printf("bind failed..\n");
        exit(-1);
    }
    ret = listen(listenfd, 10);
    if (ret < 0)
    {
        printf("listen connect failed...\n");
        exit(-1);
    }
    printf("server listen at %s : %d\n", ip, port);

    epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0)
    {
        printf("epoll create failed..\n");
        exit(-1);
    }
    addfd(epfd, listenfd, 1);

    while (1)
    {
        epoll_event_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (epoll_event_count < 0)
        {
            printf("epoll error\n");
            break;
        }
        printf("got %d epoll events\n", epoll_event_count);

        for (int i = 0; i < epoll_event_count; ++i)
        {
            int cur_fd = events[i].data.fd;
            printf("current fd is %d\n", cur_fd);
            if (cur_fd == listenfd)
            {
                struct sockaddr_in client_address;
                client_address_length = sizeof(client_address);
                clientfd = accept(listenfd, (struct sockaddr *)&client_address, &client_address_length);
                if (clientfd < 0)
                {
                    printf("errno is %d\n", errno);
                    continue;
                }
                if (user_count >= USER_LIMIT)
                {
                    printf("%s", TOO_LONG);
                    send(clientfd, TOO_LONG, strlen(TOO_LONG), 0);
                    close(clientfd);
                    continue;
                }
                ++user_count;
                addfd(epfd, clientfd, 1);
                users.emplace(clientfd);
                printf("Here are %d user(s)\n", user_count);
                sprintf(msg, "Welcome %d", clientfd);
                ret = send(clientfd, msg, BUFFER_SIZE, 0);
                if (ret < 0)
                {
                    printf("send welcome msg error\n");
                    break;
                }
            }
            else
            {
                if (broadcast(cur_fd) < 0)
                {
                    printf("send messeage failed\n");
                }
            }
        }
    }
    close(listenfd);
    close(epfd);
}