#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
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

static volatile sig_atomic_t server_running = 1;
static volatile sig_atomic_t reload_config = 0;

static void handle_signal(int signal_number)
{
    if (signal_number == SIGINT)
    {
        mysyslog_info("SIGINT received, server stopping");
        server_running = 0;
    }
    else if (signal_number == SIGHUP)
    {
        mysyslog_info("SIGHUP received, configuration reload requested");
        reload_config = 1;
    }
    else if (signal_number == SIGCHLD)
    {
        mysyslog_info("SIGCHLD received, child process finished");

        while (waitpid(-1, NULL, WNOHANG) > 0)
        {
        }
    }
}

static void setup_signal_handlers(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGHUP, handle_signal);
    signal(SIGCHLD, handle_signal);
}

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

static int parse_request(const char *request,
                         char *username,
                         size_t username_size,
                         char *command,
                         size_t command_size)
{
    const char *separator;
    size_t username_length;

    separator = strchr(request, '|');

    if (separator == NULL)
    {
        mysyslog_error("invalid request format");
        return -1;
    }

    username_length = (size_t)(separator - request);

    if (username_length == 0 || username_length >= username_size)
    {
        mysyslog_error("invalid username length");
        return -1;
    }

    strncpy(username, request, username_length);
    username[username_length] = '\0';

    strncpy(command, separator + 1, command_size - 1);
    command[command_size - 1] = '\0';

    if (command[0] == '\0')
    {
        mysyslog_error("empty command");
        return -1;
    }

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
    int option;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        mysyslog_error("socket creation failed");
        return -1;
    }

    option = 1;

    if (setsockopt(server_socket,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &option,
                   sizeof(option)) < 0)
    {
        mysyslog_error("setsockopt failed");
        close(server_socket);
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
        if (errno == EINTR)
        {
            return -1;
        }

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

static int read_file_to_buffer(const char *path,
                               char *buffer,
                               size_t buffer_size)
{
    FILE *file;
    size_t used;
    int character;

    file = fopen(path, "r");

    if (file == NULL)
    {
        mysyslog_error("temporary file read failed");
        return -1;
    }

    used = 0;

    while ((character = fgetc(file)) != EOF && used < buffer_size - 1)
    {
        buffer[used] = (char)character;
        used++;
    }

    buffer[used] = '\0';

    fclose(file);

    return 0;
}

static int execute_command(const char *command,
                           char *result,
                           size_t result_size)
{
    char stdout_template[] = "/tmp/myRPC_XXXXXX.stdout";
    char stderr_template[] = "/tmp/myRPC_XXXXXX.stderr";
    char shell_command[2048];
    int stdout_fd;
    int stderr_fd;
    int status;

    stdout_fd = mkstemps(stdout_template, 7);

    if (stdout_fd < 0)
    {
        mysyslog_error("stdout temporary file creation failed");
        snprintf(result, result_size, "stdout temporary file creation failed\n");
        return -1;
    }

    stderr_fd = mkstemps(stderr_template, 7);

    if (stderr_fd < 0)
    {
        close(stdout_fd);
        mysyslog_error("stderr temporary file creation failed");
        snprintf(result, result_size, "stderr temporary file creation failed\n");
        return -1;
    }

    close(stdout_fd);
    close(stderr_fd);

    snprintf(shell_command,
             sizeof(shell_command),
             "%s > %s 2> %s",
             command,
             stdout_template,
             stderr_template);

    status = system(shell_command);

    if (status != 0)
    {
        read_file_to_buffer(stderr_template, result, result_size);

        if (strlen(result) == 0)
        {
            snprintf(result, result_size, "Command finished with error\n");
        }

        return -1;
    }

    if (read_file_to_buffer(stdout_template, result, result_size) < 0)
    {
        snprintf(result, result_size, "Command executed, but stdout read failed\n");
        return -1;
    }

    if (strlen(result) == 0)
    {
        snprintf(result, result_size, "Command executed without output\n");
    }

    return 0;
}

static void handle_client(int client_socket)
{
    char request[1024];
    char username[128];
    char command[1024];
    char result[RESULT_SIZE];

    if (receive_client_request(client_socket,
                               request,
                               sizeof(request)) < 0)
    {
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (parse_request(request,
                      username,
                      sizeof(username),
                      command,
                      sizeof(command)) < 0)
    {
        send_server_response(client_socket, "Invalid request format\n");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (!is_user_allowed(username))
    {
        mysyslog_error("access denied");
        send_server_response(client_socket, "Access denied\n");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    execute_command(command, result, sizeof(result));

    send_server_response(client_socket, result);

    close(client_socket);
    exit(EXIT_SUCCESS);
}

int main(void)
{
    ServerConfig config;
    int server_socket;

    set_default_config(&config);

    if (load_config(CONFIG_PATH, &config) < 0)
    {
        mysyslog_info("default server configuration is used");
    }

    setup_signal_handlers();
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

    mysyslog_info("server socket bound");

    if (start_listening(server_socket) < 0)
    {
        close(server_socket);
        return EXIT_FAILURE;
    }

    mysyslog_info("server is listening");

    while (server_running)
    {
        int client_socket;
        pid_t child_pid;

        if (reload_config)
        {
            set_default_config(&config);
            load_config(CONFIG_PATH, &config);
            reload_config = 0;
            mysyslog_info("configuration reloaded");
        }

        client_socket = accept_client(server_socket);

        if (client_socket < 0)
        {
            continue;
        }

        child_pid = fork();

        if (child_pid < 0)
        {
            mysyslog_error("fork failed");
            close(client_socket);
            continue;
        }

        if (child_pid == 0)
        {
            close(server_socket);
            handle_client(client_socket);
        }

        close(client_socket);
    }

    close(server_socket);
    mysyslog_info("server stopped");

    return EXIT_SUCCESS;
}