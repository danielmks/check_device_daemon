#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>

#define MAX_LINE 256

#define CONFIG_FILE "/etc/check_device/check_device.conf"

config_t global_config;  // 전역 변수 정의

//문자열 양쪽의 공백(whitespace)을 제거
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

//설정 파일(check_device.conf)을 파싱하여 프로그램에서 사용할 설정 구조체에 값을 저장
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
        if (strcmp(key, "INTERVAL_SECONDS") == 0)
            config->interval_seconds = atoi(value);
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
        else if (strcmp(key, "SNMP_TRAP_ENABLE") == 0)
            config->snmp_trap_enable = atoi(value);
        else if (strcmp(key, "SNMP_TRAP_DEST") == 0)
            strncpy(config->snmp_trap_dest, value, sizeof(config->snmp_trap_dest)-1);
        else if (strcmp(key, "SNMP_TRAP_PORT") == 0)
            config->snmp_trap_port = atoi(value);
        else if (strcmp(key, "SYSLOG_ENABLE") == 0)
            config->syslog_enable = atoi(value);
        else if (strcmp(key, "NET_INTERFACE") == 0)
            strncpy(config->net_interface, value, sizeof(config->net_interface)-1);
        else if (strcmp(key, "CSV_RETENTION_DAYS") == 0)
            config->csv_retention_days = atoi(value);
        else if (strcmp(key, "POWER_STATUS_PATH") == 0)
            strncpy(config->power_status_path, value, sizeof(config->power_status_path)-1);
        else if (strcmp(key, "FAN_STATUS_PATH") == 0)
            strncpy(config->fan_status_path, value, sizeof(config->fan_status_path)-1);
        else if (strcmp(key, "RAID_STATUS_PATH") == 0)
            strncpy(config->raid_status_path, value, sizeof(config->raid_status_path)-1);
    }
    fclose(fp);
    return 0;
}


/* 초기화 시 설정 파일을 읽어 전역 변수에 저장 */
void init_config(void) {
    if (check_config(CONFIG_FILE, &global_config) != 0) {
        syslog(LOG_ERR, "Failed to read configuration file: %s", CONFIG_FILE);
        /* 실패시 기본값을 설정하거나 종료 처리할 수 있음 */
    }
}