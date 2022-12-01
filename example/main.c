#include<cserv/cserv.h>

#include<stdio.h>
#include<stdlib.h>

static void handler(int fd)
{
    printf("Client connected...\n");
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: cserv_example hostname port\n");
        return 1;
    }

    const char *hostname = argv[1];
    const char *port_str = argv[2];
    uint16_t port = (uint16_t)strtoul(port_str, NULL, 10);

    cserver *serv = cserver_init(hostname, port, 0);
    if (!serv)
        return 1;
    
    cserver_start(serv, &handler);
    cserver_free(serv);
}