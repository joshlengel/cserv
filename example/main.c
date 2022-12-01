#include<cserv/cserv.h>

#include<stdio.h>

static void handler(int fd)
{
    printf("Client connected...\n");
}

int main()
{
    cserver *serv = cserver_init("localhost", 8080, 0);
    if (!serv)
        return 1;
    
    cserver_start(serv, &handler);
    cserver_free(serv);
}