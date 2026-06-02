#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8642
#define DEFAULT_USER "semakan"

typedef struct
{
    char host[128];
    int port;
    int use_stream;
    char username[128];
    char command[512];
} ClientConfig;

static void print_help(const char *program_name)
{
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("  -u, --user USER         User name\n");
    printf("  -c, --command COMMAND   Bash command to execute\n");
    printf("  -h, --host HOST         Server address\n");
    printf("  -p, --port PORT         Server port\n");
    printf("  -s, --stream            Use TCP stream socket\n");
    printf("  -d, --dgram             Use UDP datagram socket\n");
    printf("      --help              Show help message\n");
}

static void set_default_config(ClientConfig *config)
{
    strcpy(config->host, DEFAULT_HOST);
    strcpy(config->username, DEFAULT_USER);
    config->port = DEFAULT_PORT;
    config->use_stream = 1;
    config->command[0] = '\0';
}

static int parse_arguments(int argc, char *argv[], ClientConfig *config)
{
    int option;

    static struct option long_options[] =
    {
        {"user", required_argument, 0, 'u'},
        {"command", required_argument, 0, 'c'},
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"stream", no_argument, 0, 's'},
        {"dgram", no_argument, 0, 'd'},
        {"help", no_argument, 0, 1},
        {0, 0, 0, 0}
    };

    while ((option = getopt_long(argc, argv, "u:c:h:p:sd", long_options, NULL)) != -1)
    {
        switch (option)
        {
            case 'u':
                strncpy(config->username, optarg, sizeof(config->username) - 1);
                config->username[sizeof(config->username) - 1] = '\0';
                break;

            case 'c':
                strncpy(config->command, optarg, sizeof(config->command) - 1);
                config->command[sizeof(config->command) - 1] = '\0';
                break;

            case 'h':
                strncpy(config->host, optarg, sizeof(config->host) - 1);
                config->host[sizeof(config->host) - 1] = '\0';
                break;

            case 'p':
                config->port = atoi(optarg);
                break;

            case 's':
                config->use_stream = 1;
                break;

            case 'd':
                config->use_stream = 0;
                break;

            case 1:
                print_help(argv[0]);
                exit(EXIT_SUCCESS);

            default:
                print_help(argv[0]);
                return -1;
        }
    }

    if (config->command[0] == '\0')
    {
        fprintf(stderr, "Error: command is required\n");
        return -1;
    }

    if (config->port <= 0 || config->port > 65535)
    {
        fprintf(stderr, "Error: invalid port\n");
        return -1;
    }

    return 0;
}

static int build_request(const ClientConfig *config,
                         char *request,
                         size_t request_size)
{
    int written;

    written = snprintf(request,
                       request_size,
                       "%s|%s",
                       config->username,
                       config->command);

    if (written < 0 || (size_t)written >= request_size)
    {
        fprintf(stderr, "Error: request is too long\n");
        return -1;
    }

    return 0;
}

static int send_command_to_server(const ClientConfig *config)
{
    int client_socket;
    struct sockaddr_in server_address;
    char request[1024];
    char buffer[4096];
    ssize_t bytes_received;

    if (build_request(config, request, sizeof(request)) < 0)
    {
        return -1;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket < 0)
    {
        perror("socket");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(config->port);

    if (inet_pton(AF_INET, config->host, &server_address.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(client_socket);
        return -1;
    }

    if (connect(client_socket,
                (struct sockaddr *)&server_address,
                sizeof(server_address)) < 0)
    {
        perror("connect");
        close(client_socket);
        return -1;
    }

    if (send(client_socket, request, strlen(request), 0) < 0)
    {
        perror("send");
        close(client_socket);
        return -1;
    }

    memset(buffer, 0, sizeof(buffer));

    bytes_received = recv(client_socket,
                          buffer,
                          sizeof(buffer) - 1,
                          0);

    if (bytes_received < 0)
    {
        perror("recv");
        close(client_socket);
        return -1;
    }

    buffer[bytes_received] = '\0';

    printf("%s", buffer);

    close(client_socket);

    return 0;
}

int main(int argc, char *argv[])
{
    ClientConfig config;

    set_default_config(&config);

    if (parse_arguments(argc, argv, &config) != 0)
    {
        return EXIT_FAILURE;
    }

    return send_command_to_server(&config) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}