#pragma once

#include<stddef.h>
#include<stdint.h>

#define CSERVER_DEFAULT_MAX_CONNECTIONS 1000

struct _cserver;
typedef struct _cserver cserver;

cserver *cserver_init(const char *host, uint16_t port, size_t max_connections);
void cserver_start(cserver *cserver, void(*handler)(int));
void cserver_free(cserver *cserver);