#include"cserv/cserv.h"

#include"tools.h"

#include<assert.h>
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<stdatomic.h>

#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h>
#include<signal.h>
#include<pthread.h>

struct _cserver
{
    int fd;

    _Atomic(int) running;

    pthread_mutex_t client_mut;
    pthread_cond_t client_cond;
    FDQueue client_queue;

    size_t num_workers;
    pthread_t *workers;

    void(*handler)(int);
};

cserver *cserver_init(const char *host, uint16_t port, size_t max_connections)
{
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error initializing cserver (syscall: socket)");
        return NULL;
    }

    const int true_val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &true_val, sizeof(true_val)) < 0)
    {
        perror("Error initializing cserver (syscall: setsockopt)");
        close(fd);
        return NULL;
    }

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%hu", port);

    struct addrinfo addr_hints, *addr;
    int gai_error;
    if ((gai_error = getaddrinfo(host, port_str, &addr_hints, &addr)) < 0)
    {
        const char *gai_str = gai_strerror(gai_error);
        printf("Error initializing cserver (syscall: getaddrinfo): %s\n", gai_str);
        close(fd);
        return NULL;
    }

    int err = bind(fd, addr->ai_addr, addr->ai_addrlen);
    freeaddrinfo(addr);
    if (err < 0)
    {
        perror("Error initializing cserver (syscall: bind)");
        close(fd);
        return NULL;
    }

    cserver *serv = malloc(sizeof(cserver));
    if (!serv)
        return NULL;

    long cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpus <= 0)
    {
        printf("Error initializing cserver: Can't determine number of cpus, using 1");
        serv->num_workers = 1;
    }
    else
    {
        serv->num_workers = (size_t)cpus;
    }

    serv->workers = calloc(serv->num_workers, sizeof(pthread_t));
    if (!serv->workers)
    {
        free(serv);
        return NULL;
    }

    fdqueue_init(&serv->client_queue, max_connections == 0? CSERVER_DEFAULT_MAX_CONNECTIONS : max_connections);
    
    pthread_mutex_init(&serv->client_mut, NULL);
    pthread_cond_init(&serv->client_cond, NULL);

    serv->fd = fd;
    return serv;
}

static void _cserver_start_sigint_handler(int sig)
{}

static void *_cserver_worker(void *arg)
{
    cserver *serv = (cserver*)arg;

    while (serv->running)
    {
        pthread_mutex_lock(&serv->client_mut);
        if (fdqueue_empty(&serv->client_queue))
            pthread_cond_wait(&serv->client_cond, &serv->client_mut);
        
        if (!serv->running)
        {
            pthread_mutex_unlock(&serv->client_mut);
            break;
        }

        const int fd = fdqueue_pop(&serv->client_queue);
        
        pthread_mutex_unlock(&serv->client_mut);

        serv->handler(fd);

        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

    return (void*)0;
}

void cserver_start(cserver *cserver, void(*handler)(int))
{
    assert(cserver != NULL);

    cserver->handler = handler;

    if (listen(cserver->fd, -1) < 0)
    {
        perror("Error starting cserver (syscall: listen)");
        return;
    }
    
    struct sigaction sig_handler, old_sig_handler;
    memset(&sig_handler, 0, sizeof(sig_handler));
    sig_handler.sa_handler = &_cserver_start_sigint_handler;
    sigaction(SIGINT, &sig_handler, &old_sig_handler);

    cserver->running = 1;
    for (size_t i = 0; i < cserver->num_workers; ++i)
        pthread_create(cserver->workers + i, NULL, &_cserver_worker, cserver);

    while (cserver->running)
    {
        int client_fd;
        if ((client_fd = accept(cserver->fd, NULL, NULL)) < 0)
        {
            switch (errno)
            {
                case ENETDOWN:
                case EPROTO:
                case ENOPROTOOPT:
                case EHOSTDOWN:
                case ENONET:
                case EHOSTUNREACH:
                case EOPNOTSUPP:
                case ENETUNREACH:
                case EAGAIN:
                    perror("Error running cserver (syscall: accept)");
                    printf("Retrying...\n");
                    continue;
                
                case EINTR:
                    printf("Interrupt signal received. Shutting down server...\n");
                    cserver->running = 0;
                    continue;

                default:
                    perror("Error running cserver (syscall: accept)");
                    cserver->running = 0;
                    continue;
            }
        }

        pthread_mutex_lock(&cserver->client_mut);
        if (fdqueue_push(&cserver->client_queue, client_fd) < 0)
        {
            printf("Error running cserv: Maximum connection limit reached\n");
            shutdown(client_fd, SHUT_RDWR);
            close(client_fd);
        }
        pthread_cond_signal(&cserver->client_cond);
        pthread_mutex_unlock(&cserver->client_mut);
    }

    pthread_mutex_lock(&cserver->client_mut);
    pthread_cond_broadcast(&cserver->client_cond);
    pthread_mutex_unlock(&cserver->client_mut);

    for (size_t i = 0; i < cserver->num_workers; ++i)
        pthread_join(cserver->workers[i], NULL);

    sigaction(SIGINT, &old_sig_handler, NULL);
}

void cserver_free(cserver *cserver)
{
    assert(cserver != NULL);

    shutdown(cserver->fd, SHUT_RDWR);
    close(cserver->fd);

    fdqueue_free(&cserver->client_queue);
    pthread_mutex_destroy(&cserver->client_mut);
    pthread_cond_destroy(&cserver->client_cond);

    free(cserver->workers);

    free(cserver);
}