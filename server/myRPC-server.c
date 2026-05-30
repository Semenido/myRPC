#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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

static int create_server_socket(void)
{
    int server_socket;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        perror("socket");
        return -1;
    }

    return server_socket;
}

int main(void)
{
    ServerConfig config;
    int server_socket;

    set_default_config(&config);
    print_config(&config);

    server_socket = create_server_socket();

    if (server_socket < 0)
    {
        return EXIT_FAILURE;
    }

    printf("Server socket created\n");

    close(server_socket);

    return EXIT_SUCCESS;
}