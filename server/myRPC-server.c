#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DEFAULT_PORT 8642
#define DEFAULT_SOCKET_TYPE "stream"

typedef struct
{
    int port;
    char socket_type[16];
} ServerConfig;

static void set_default_config(ServerConfig *config)
{
    config->port = DEFAULT_PORT;
    strcpy(config->socket_type, DEFAULT_SOCKET_TYPE);
}

static void print_config(const ServerConfig *config)
{
    printf("myRPC-server started\n");
    printf("Port: %d\n", config->port);
    printf("Socket type: %s\n", config->socket_type);
}

int main(void)
{
    ServerConfig config;

    set_default_config(&config);
    print_config(&config);

    return EXIT_SUCCESS;
}