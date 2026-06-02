#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../libmysyslog/mysyslog.h"

#define DEFAULT_PORT 8642
#define DEFAULT_SOCKET_TYPE "stream"
#define CONFIG_PATH "../configs/myRPC.conf"
#define USERS_PATH "../configs/users.conf"
#define RESULT_SIZE 4096

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
        mysyslog_error("configuration file open failed");
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

static int is_user_allowed(const char *username)
{
    FILE *file;
    char line[128];

    file = fopen(USERS_PATH, "r");

    if (file == NULL)
    {
        mysyslog_error("users configuration file open failed");
        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        line[strcspn(line, "\r\n")] = '\0';

        if (strcmp(line, username) == 0)
        {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

static void print_config(const ServerConfig *config)
{
    mysyslog_info("myRPC-server started");
    printf("Port: %d\n", config->port);
    printf("Socket type: %s\n", config->socket_type);
}

static int create_server_socket(void)
{
    int server_socket;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        mysyslog_error("socket creation failed");
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
        mysyslog_error("socket bind failed");
        return -1;
    }

    return 0;
}

static int start_listening(int server_socket)
{
    if (listen(server_socket, 5) < 0)
    {
        mysyslog_error("listen failed");
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
        mysyslog_error("client accept failed");
        return -1;
    }

    printf("Client connected: %s\n",
           inet_ntoa(client_address.sin_addr));

    return client_socket;
}

static int receive_client_request(int client_socket,
                                  char *buffer,
                                  size_t size)
{
    ssize_t bytes_received;

    memset(buffer, 0, size);

    bytes_received = recv(client_socket,
                          buffer,
                          size - 1,
                          0);

    if (bytes_received < 0)
    {
        mysyslog_error("request receiving failed");
        return -1;
    }

    if (bytes_received == 0)
    {
        mysyslog_info("client disconnected");
        return -1;
    }

    buffer[bytes_received] = '\0';

    printf("Request from client: %s\n", buffer);

    return 0;
}

static int execute_command(const char *command,
                           char *result,
                           size_t result_size)
{
    FILE *pipe;
    char buffer[256];
    size_t used;

    memset(result, 0, result_size);
    used = 0;

    pipe = popen(command, "r");

    if (pipe == NULL)
    {
        mysyslog_error("command execution failed");
        snprintf(result, result_size, "Command execution failed\n");
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), pipe) != NULL)
    {
        size_t buffer_length;

        buffer_length = strlen(buffer);

        if (used + buffer_length >= result_size - 1)
        {
            break;
        }

        strcat(result, buffer);
        used += buffer_length;
    }

    pclose(pipe);

    if (strlen(result) == 0)
    {
        snprintf(result, result_size, "Command executed without output\n");
    }

    return 0;
}

static int send_server_response(int client_socket,
                                const char *response)
{
    if (send(client_socket,
             response,
             strlen(response),
             0) < 0)
    {
        mysyslog_error("response sending failed");
        return -1;
    }

    return 0;
}

int main(void)
{
    ServerConfig config;
    int server_socket;
    int client_socket;
    char command[1024];
    char result[RESULT_SIZE];
    const char *current_user = "semakan";

    set_default_config(&config);

    if (load_config(CONFIG_PATH, &config) < 0)
    {
        mysyslog_info("default server configuration is used");
    }

    print_config(&config);

    if (!is_user_allowed(current_user))
    {
        mysyslog_error("access denied");
        return EXIT_FAILURE;
    }

    mysyslog_info("access granted");

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

    mysyslog_info("server socket bound");

    if (start_listening(server_socket) < 0)
    {
        close(server_socket);
        return EXIT_FAILURE;
    }

    mysyslog_info("server is listening");

    client_socket = accept_client(server_socket);

    if (client_socket < 0)
    {
        close(server_socket);
        return EXIT_FAILURE;
    }

    mysyslog_info("client accepted");

    if (receive_client_request(client_socket,
                               command,
                               sizeof(command)) < 0)
    {
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    execute_command(command, result, sizeof(result));

    if (send_server_response(client_socket, result) < 0)
    {
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    close(client_socket);
    close(server_socket);

    return EXIT_SUCCESS;
}