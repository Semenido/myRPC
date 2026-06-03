#include <stdio.h>
#include <time.h>

#include "mysyslog.h"

#define LOG_FILE "/var/log/myrpc.log"

static void write_log(const char *level, const char *message)
{
    FILE *log = fopen(LOG_FILE, "a");

    if (log == NULL)
        return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    fprintf(log,
            "[%04d-%02d-%02d %02d:%02d:%02d] [%s] %s\n",
            tm_info->tm_year + 1900,
            tm_info->tm_mon + 1,
            tm_info->tm_mday,
            tm_info->tm_hour,
            tm_info->tm_min,
            tm_info->tm_sec,
            level,
            message);

    fclose(log);
}

void mysyslog_info(const char *message)
{
    printf("[INFO] %s\n", message);
    write_log("INFO", message);
}

void mysyslog_error(const char *message)
{
    fprintf(stderr, "[ERROR] %s\n", message);
    write_log("ERROR", message);
}