#include "logging.h"
#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#define PROCESS_NAME "check_device"
#define LOG_DIR "/var/log/check_device"

void ensure_log_dir(void) {
    struct stat st;
    if (stat(LOG_DIR, &st) == -1) {
        mkdir(LOG_DIR, 0755);
    }
}

void write_csv_log(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char csv_filename[256];
    snprintf(csv_filename, sizeof(csv_filename), "%s_log_%04d%02d%02d.csv",
             PROCESS_NAME, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    char csv_filepath[512];
    snprintf(csv_filepath, sizeof(csv_filepath), "%s/%s", LOG_DIR, csv_filename);

    // 측정값 읽기
    float cpu_usage = get_cpu_usage();
    float mem_usage = get_memory_usage();
    float disk_usage = get_disk_usage();
    float cpu_temp = get_cpu_temperature();
    float rx_rate = 0, tx_rate = 0;
    get_network_traffic(&rx_rate, &tx_rate);
    const char* power_status = get_power_module_status();
    const char* fan_status = get_fan_status();
    const char* raid_status = get_raid_status();

    // 날짜와 시간 문자열
    char dateStr[16];
    char timeStr[16];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tm_info);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);

    int write_header = (access(csv_filepath, F_OK) != 0);
    FILE *fp = fopen(csv_filepath, "a");
    if (fp != NULL) {
        if (write_header) {
            fprintf(fp, "Date,Time,CPU Usage,Memory Usage,Disk Usage,CPU Temp,Net RX,Net TX,Power Status,Fan Status,RAID Status\n");
        }
        fprintf(fp, "%s,%s,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%s,%s,%s\n",
                dateStr, timeStr, cpu_usage, mem_usage, disk_usage, cpu_temp, rx_rate, tx_rate,
                power_status, fan_status, raid_status);
        fclose(fp);
    }
}
