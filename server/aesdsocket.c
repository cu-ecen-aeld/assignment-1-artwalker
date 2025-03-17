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
#include <pthread.h>
#include <time.h>

#define PORT 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define BACKLOG 5
#define BUFFER_SIZE 1024
#define TIMESTAMP_INTERVAL 10

int server_socket = -1;
int client_socket = -1;
int file_fd = -1;

struct thread_data 
{
    int client_socket;
    struct sockaddr_in client_addr;
    pthread_t thread_id;
    struct thread_data *next;
};

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
struct thread_data *thread_list = NULL;

void *handle_connection(void *thread_param)
{
    struct thread_data *data = (struct thread_data *)thread_param;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    while ((bytes_received = recv(data->client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        pthread_mutex_lock(&file_mutex);
        int file_fd = open(FILE_PATH, O_CREAT | O_APPEND | O_RDWR, 0644);
        if (file_fd == -1) {
            syslog(LOG_ERR, "Failed to open file");
            pthread_mutex_unlock(&file_mutex);
            break;
        }

        if (write(file_fd, buffer, bytes_received) != bytes_received) {
            syslog(LOG_ERR, "Failed to write to file");
            close(file_fd);
            pthread_mutex_unlock(&file_mutex);
            break;
        }

        if (buffer[bytes_received - 1] == '\n') {
            lseek(file_fd, 0, SEEK_SET);
            while ((bytes_received = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
                if (send(data->client_socket, buffer, bytes_received, 0) != bytes_received) {
                    syslog(LOG_ERR, "Failed to send data to client");
                    break;
                }
            }
        }
        close(file_fd);
        pthread_mutex_unlock(&file_mutex);
    }

    close(data->client_socket);
    syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(data->client_addr.sin_addr));

    pthread_mutex_lock(&list_mutex);
    struct thread_data **pp = &thread_list;
    while (*pp && (*pp) != data) {
        pp = &(*pp)->next;
    }
    if (*pp) {
        *pp = data->next;
    }
    pthread_mutex_unlock(&list_mutex);
    
    free(data);
    return NULL;
}

void *timestamp_thread(void *arg)
{
    (void)arg; 
    while (1)
    {
        sleep(TIMESTAMP_INTERVAL);

        time_t now = time(NULL);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", localtime(&now));

        pthread_mutex_lock(&file_mutex);
        int file_fd = open(FILE_PATH, O_CREAT | O_APPEND | O_RDWR, 0644);
        if (file_fd != -1)
        {
            write(file_fd, timestamp, strlen(timestamp));
            close(file_fd);
        }
        pthread_mutex_unlock(&file_mutex);
    }
    return NULL;
}

void cleanup_threads()
{
    pthread_mutex_lock(&list_mutex);
    struct thread_data *current = thread_list;
    while (current)
    {
        pthread_join(current->thread_id, NULL);
        struct thread_data *next = current->next;
        free(current);
        current = next;
    }
    thread_list = NULL;
    pthread_mutex_unlock(&list_mutex);
}

void handle_signal(int signal)
{
    syslog(LOG_INFO, "Caught signal %d", signal);
    if (server_socket != -1)
    {
        close(server_socket);
    }
    cleanup_threads();
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

    pthread_t timestamp_thread_id;
    if (pthread_create(&timestamp_thread_id, NULL, timestamp_thread, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed to create timestamp thread");
        return -1;
    }

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int new_client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (new_client_socket == -1)
        {
            syslog(LOG_ERR, "Failed to accept connection.");
            continue;
        }

        struct thread_data *data = malloc(sizeof(struct thread_data));
        if (!data)
        {
            syslog(LOG_ERR, "Failed to allocate memory for thread data");
            close(new_client_socket);
            continue;
        }

        data->client_socket = new_client_socket;
        data->client_addr = client_addr;
        data->next = NULL;

        pthread_mutex_lock(&list_mutex);
        data->next = thread_list;
        thread_list = data;
        pthread_mutex_unlock(&list_mutex);

        if (pthread_create(&data->thread_id, NULL, handle_connection, data) != 0)
        {
            syslog(LOG_ERR, "Failed to create thread");
            close(new_client_socket);
            free(data);
            continue;
        }

        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));
    }

    close(server_socket);
    closelog();
    return 0;
}
