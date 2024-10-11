#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

char buff[1024];

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip port\n", basename(argv[0]));
        return 1;
    }
    const auto ip{argv[1]};
    const auto port{atoi(argv[2])};

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        printf("connection failed\n");
    }
    else
    {
        const char *data = "hello";

        send(sock, data, strlen(data), 0);

        read(sock, buff, 1024);
        // test
        printf("%s", buff);
    }
    close(sock);
}