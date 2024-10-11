#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "processpool.hh"

class cgi_conn
{
private:
    static const int BUFFER_SIZE{1024};
    static int m_epollfd;
    int m_sockfd;
    sockaddr_in m_address;
    char m_buff[BUFFER_SIZE];
    int m_read_idx;

public:
    cgi_conn() {};
    ~cgi_conn() {};
    void init(int epollfd, int sockfd, const sockaddr_in &client_addr)
    {
        m_epollfd = epollfd;
        m_sockfd = sockfd;
        m_address = client_addr;
        memset(m_buff, '\0', BUFFER_SIZE);
        m_read_idx = 0;
    }
    void process()
    {
        int idx = 0;
        int ret = -1;

        while (1)
        {
            idx = m_read_idx;
            ret = recv(m_sockfd, m_buff + idx, BUFFER_SIZE - 1 - idx, 0);
            if (ret < 0)
            {
                if (errno != EAGAIN)
                {
                    removefd(m_epollfd, m_sockfd);
                }
                break;
            }
            else if (ret == 0)
            {
                removefd(m_epollfd, m_sockfd);
                break;
            }
            else
            {
                m_read_idx += ret;
                printf("user content is : %s \n", m_buff);
                for (; idx < m_read_idx; ++idx)
                {
                    if (idx >= 1 && m_buff[idx - 1] == '\r' && m_buff[idx] == '\n')
                    {
                        break;
                    }
                }
                if (idx == m_read_idx)
                    continue;
                m_buff[idx - 1] = '\0';
                char *file_name = m_buff;

                if (access(file_name, F_OK) == -1)
                {
                    removefd(m_epollfd, m_sockfd);
                    break;
                }
                ret = fork();
                if (ret == -1)
                {
                    removefd(m_epollfd, m_sockfd);
                    break;
                }
                else if (ret > 0)
                {
                    removefd(m_epollfd, m_sockfd);
                    break;
                }
                else
                {
                    close(STDOUT_FILENO);
                    dup(m_sockfd);
                    execl(m_buff, m_buff, 0);
                    exit(0);
                }
            }
        }
    }
};
int cgi_conn::m_epollfd = -1;
int main(int argc, char const *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const auto ip{argv[1]};
    auto port{atoi(argv[2])};

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != 1);

    ret = listen(sock, 5);
    assert(ret != 1);
    processpool<cgi_conn> &pool{processpool<cgi_conn>::get_instance(sock)};
    pool.run();
    // processpool<cgi_conn> *pool = processpool<cgi_conn>::create(sock);
    // if (pool)
    // {
    //     pool->run();
    //     delete pool;
    // }
    close(sock);

    return 0;
}
