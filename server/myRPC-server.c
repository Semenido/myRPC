#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_PORT 8642
#define DEFAULT_SOCKET_TYPE "stream"
#define CONFIG_PATH "../configs/myRPC.conf"

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

static int load_config(const char *path, ServerConfig *config)
{
    FILE *file;
    char key[64];
    char value[64];

    file = fopen(path, "r");

    if (file == NULL)
    {
        perror("fopen");
        return -1;
    }

    while (fscanf(file, " %63s = %63s", key, value) == 2)
    {
        if (strcmp(key, "port") == 0)
        {
            config->port = atoi(value);
        }
        else if (strcmp(key, "socket_type") == 0)
        {
            strncpy(config->socket_type, value, sizeof(config->socket_type) - 1);
            config->socket_type[sizeof(config->socket_type) - 1] = '\0';
        }
    }

    fclose(file);
    return 0;
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

static int receive_client_request(int client_socket)
{
    char buffer[1024];
    ssize_t bytes_received;

    memset(buffer, 0, sizeof(buffer));

    bytes_received = recv(client_socket,
                          buffer,
                          sizeof(buffer) - 1,
                          0);

    if (bytes_received < 0)
    {
        perror("recv");
        return -1;
    }

    if (bytes_received == 0)
    {
        printf("Client disconnected\n");
        return 0;
    }

    buffer[bytes_received] = '\0';

    printf("Request from client: %s\n", buffer);

    return 0;
}

static int send_server_response(int client_socket)
{
    const char *response = "Request received\n";

    if (send(client_socket,
             response,
             strlen(response),
             0) < 0)
    {
        perror("send");
        return -1;
    }

    return 0;
}

int main(void)
{
    ServerConfig config;
    int server_socket;
    int client_socket;

    set_default_config(&config);

    if (load_config(CONFIG_PATH, &config) < 0)
    {
        printf("Default server configuration is used\n");
    }

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

    client_socket = accept_client(server_socket);

    if (client_socket < 0)
    {
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Client accepted\n");

    if (receive_client_request(client_socket) < 0)
    {
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    if (send_server_response(client_socket) < 0)
    {
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    close(client_socket);
    close(server_socket);

    return EXIT_SUCCESS;
}