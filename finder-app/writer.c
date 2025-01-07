// This program run in finder-test.sh, which uses the makefile
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

void log_message(int priority, const char *message)
{
    openlog("Writer", LOG_PID | LOG_CONS, LOG_USER);
    syslog(priority, "%s", message);
    closelog();
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <file_path> <string_to_write>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *file_path = argv[1];
    const char *string_to_write = argv[2];

    log_message(LOG_DEBUG, "Writing to file");
    log_message(LOG_DEBUG, file_path);
    log_message(LOG_DEBUG, string_to_write);

    FILE *file = fopen(file_path, "a");
    if (file == NULL)
    {
        log_message(LOG_ERR, "Error opening file");
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    if (fprintf(file, "%s\n", string_to_write) < 0)
    {
        log_message(LOG_ERR, "Error writing to file");
        perror("Error writing to file");
        fclose(file);
        return EXIT_FAILURE;
    }

    fclose(file);
    log_message(LOG_DEBUG, "Write operation completed successfully");
    return EXIT_SUCCESS;
}
