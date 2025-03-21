#include "logging.h"
#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#define PROCESS_NAME "check_device"
#define LOG_DIR "/var/log/check_device"

/* 현재 날짜에 해당하는 로그 디렉토리 (/var/log/check_device/YYYYMMDD)를 확인하고 없으면 생성 */
void ensure_log_dir(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char daily_dir[512];
    snprintf(daily_dir, sizeof(daily_dir), "%s/%04d%02d%02d", 
             LOG_DIR, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    struct stat st;
    if (stat(daily_dir, &st) == -1) {
        mkdir(daily_dir, 0755);
    }
}

/* CSV 파일에 기본 지표와 하드웨어 종속 지표를 분리하여 기록하는 함수 */
void write_csv_log(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char dateStr[16], timeStr[16];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tm_info);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);

    /* 일자별 로그 디렉토리 */
    char daily_dir[512];
    snprintf(daily_dir, sizeof(daily_dir), "%s/%04d%02d%02d", 
             LOG_DIR, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    /* 디렉토리가 없으면 생성 */
    struct stat st;
    if (stat(daily_dir, &st) == -1) {
        mkdir(daily_dir, 0755);
    }
    
    /* 기본 지표 CSV 파일: basic_YYYYMMDD.csv */
    char basic_csv[512];
    snprintf(basic_csv, sizeof(basic_csv), "%s/basic_%04d%02d%02d.csv", 
             daily_dir, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    /* 하드웨어 종속 지표 CSV 파일: hwinfo_YYYYMMDD.csv */
    char hwinfo_csv[512];
    snprintf(hwinfo_csv, sizeof(hwinfo_csv), "%s/hwinfo_%04d%02d%02d.csv", 
             daily_dir, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);

    /* 기본 CSV 파일: 헤더 및 데이터 기록 */
    int basic_header = (access(basic_csv, F_OK) != 0);
    FILE *fp_basic = fopen(basic_csv, "a");
    if (fp_basic != NULL) {
        if (basic_header) {
            /* 헤더: 각 항목에 단위 명시 */
            fprintf(fp_basic, "Date,Time,CPU Usage (%%),Memory Usage (%%),Disk Usage (%%),CPU Temp (°C),Net RX (bytes/sec),Net TX (bytes/sec)\n");
        }
        float cpu_usage = get_cpu_usage();
        float mem_usage = get_memory_usage();
        float disk_usage = get_disk_usage();
        float cpu_temp = get_cpu_temperature();
        float rx_rate = 0, tx_rate = 0;
        get_network_traffic(&rx_rate, &tx_rate);
        fprintf(fp_basic, "%s,%s,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n", dateStr, timeStr, cpu_usage, mem_usage, disk_usage, cpu_temp, rx_rate, tx_rate);
        fclose(fp_basic);
    }

    /* 하드웨어 종속 CSV 파일: 헤더 및 데이터 기록 */
    int hwinfo_header = (access(hwinfo_csv, F_OK) != 0);
    FILE *fp_hwinfo = fopen(hwinfo_csv, "a");
    if (fp_hwinfo != NULL) {
        if (hwinfo_header) {
            fprintf(fp_hwinfo, "Date,Time,Power Status,Fan Status,RAID Status\n");
        }
        const char *power_status = get_power_module_status();
        const char *fan_status = get_fan_status();
        const char *raid_status = get_raid_status();
        fprintf(fp_hwinfo, "%s,%s,%s,%s,%s\n", dateStr, timeStr, power_status, fan_status, raid_status);
        fclose(fp_hwinfo);
    }
}
