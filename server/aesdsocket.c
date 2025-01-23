#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define BACKLOG 5
#define BUFFER_SIZE 1024

int server_socket = -1;
int client_socket = -1;
int file_fd = -1;

void handle_signal(int signal)
{
    syslog(LOG_INFO, "Caught signal %d", signal);
    if (server_socket != -1)
    {
        close(server_socket);
    }
    if (client_socket != -1)
    {
        close(client_socket);
    }
    remove(FILE_PATH);
    closelog();
    exit(EXIT_SUCCESS);
}

void setup_signal_handler()
{
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void run_as_daemon()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }
    if (setsid() < 0)
    {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    if (chdir("/") < 0)
    {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY); // STDIN
    open("/dev/null", O_WRONLY); // STDOUT
    open("/dev/null", O_RDWR);   // STDERR
}

int main(int argc, char *argv[])
{
    int daemon_mode = 0;
    if (argc == 2 && strcmp(argv[1], "-d") == 0)
    {
        daemon_mode = 1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);

    if (daemon_mode)
    {
        run_as_daemon();
    }

    setup_signal_handler();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        syslog(LOG_ERR, "Failed to create socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        syslog(LOG_ERR, "Failed to set socket options");
        close(server_socket);
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(PORT)};


    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        syslog(LOG_ERR, "Failed to bind socket");
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, BACKLOG) == -1)
    {
        syslog(LOG_ERR, "Failed to accpet connection.");
        close(server_socket);
        return -1;
    }

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1)
        {
            syslog(LOG_ERR, "Failed to accept connection.");
            continue;
        }

        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

        file_fd = open(FILE_PATH, O_CREAT | O_APPEND | O_RDWR, 0644);
        if (file_fd == -1)
        {
            syslog(LOG_ERR, "File to open file");
            close(client_socket);
            continue;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_received;
        while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0)
        {
            if (write(file_fd, buffer, bytes_received) != bytes_received)
            {
                syslog(LOG_ERR, "Failed to write to file");
                break;
            }
            if (buffer[bytes_received - 1] == '\n')
            {
                lseek(file_fd, 0, SEEK_SET);
                while ((bytes_received = read(file_fd, buffer, BUFFER_SIZE)) > 0)
                {
                    if (send(client_socket, buffer, bytes_received, 0) != bytes_received)
                    {
                        syslog(LOG_ERR, "Failed to send data to client");
                        break;
                    }
                }
                break;
            }
        }

        close(file_fd);
        file_fd = -1;
        close(client_socket);
        client_socket = -1;
        syslog(LOG_INFO, "Close connection from %s", inet_ntoa(client_addr.sin_addr));
    }

    close(server_socket);
    closelog();
    return 0;
}