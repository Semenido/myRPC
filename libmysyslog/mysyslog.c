#include <stdio.h>

#include "mysyslog.h"

void mysyslog_info(const char *message)
{
    printf("[INFO] %s\n", message);
}

void mysyslog_error(const char *message)
{
    fprintf(stderr, "[ERROR] %s\n", message);
}