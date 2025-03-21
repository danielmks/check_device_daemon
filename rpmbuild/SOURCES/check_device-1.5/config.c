#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 256

/* 문자열 앞뒤 공백 제거 */
static char* trim(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0)
        return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = 0;
    return str;
}

int check_config(const char *conf_path, config_t *config) {
    FILE *fp = fopen(conf_path, "r");
    if (!fp) {
        perror("fopen config");
        return -1;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        char *p = trim(line);
        if (p[0] == '#' || p[0] == '\0')
            continue;

        char *eq = strchr(p, '=');
        if (!eq)
            continue;
        *eq = '\0';
        char *key = trim(p);
        char *value = trim(eq + 1);

        if (strcmp(key, "INTERVAL_MINUTES") == 0)
            config->interval_minutes = atoi(value);
        else if (strcmp(key, "CPU_USAGE_THRESHOLD") == 0)
            config->cpu_usage_threshold = atof(value);
        else if (strcmp(key, "MEM_USAGE_THRESHOLD") == 0)
            config->mem_usage_threshold = atof(value);
        else if (strcmp(key, "DISK_USAGE_THRESHOLD") == 0)
            config->disk_usage_threshold = atof(value);
        else if (strcmp(key, "CPU_TEMP_THRESHOLD") == 0)
            config->cpu_temp_threshold = atof(value);
        else if (strcmp(key, "NET_RX_THRESHOLD") == 0)
            config->net_rx_threshold = atof(value);
        else if (strcmp(key, "NET_TX_THRESHOLD") == 0)
            config->net_tx_threshold = atof(value);
    }
    fclose(fp);
    return 0;
}
