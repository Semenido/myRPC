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

static int bind_server_socket(int server_socket, int port)
{
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(server_socket,
             (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0)
    {
        perror("bind");
        return -1;
    }

    return 0;
}

static int start_listening(int server_socket)
{
    if (listen(server_socket, 5) < 0)
    {
        perror("listen");
        return -1;
    }

    return 0;
}

static int accept_client(int server_socket)
{
    struct sockaddr_in client_address;
    socklen_t client_address_length;
    int client_socket;

    client_address_length = sizeof(client_address);

    client_socket = accept(server_socket,
                           (struct sockaddr *)&client_address,
                           &client_address_length);

    if (client_socket < 0)
    {
        perror("accept");
        return -1;
    }

    printf("Client connected: %s\n",
           inet_ntoa(client_address.sin_addr));

    return client_socket;
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

    if (bind_server_socket(server_socket, config.port) < 0)
    {
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Server bound to port %d\n", config.port);

    if (start_listening(server_socket) < 0)
    {
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Server is listening\n");

    int client_socket;

    client_socket = accept_client(server_socket);

    if (client_socket < 0)
    {
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Client accepted\n");

    close(client_socket);

    close(server_socket);

    return EXIT_SUCCESS;
}