#include "logging.h"
#include "metrics.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

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
/* Timestamp 형식(YYYY-MM-DDTHH:MM:SS)으로 기록 */
void write_csv_log(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    /* ISO 8601 형식의 Timestamp 생성: ex) 2025-03-21T17:56:51 */
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", tm_info);

    /* 일자별 로그 디렉토리 */
    char daily_dir[512];
    snprintf(daily_dir, sizeof(daily_dir), "%s/%04d%02d%02d", 
             LOG_DIR, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    struct stat st;
    if (stat(daily_dir, &st) == -1) {
        mkdir(daily_dir, 0755);
    }
    
    /* 기본 지표 CSV 파일: basic_YYYYMMDD.csv */
    char basic_csv[512];
    snprintf(basic_csv, sizeof(basic_csv), "%s/basic_%04d%02d%02d.csv", 
             daily_dir, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    int basic_header = (access(basic_csv, F_OK) != 0);
    FILE *fp_basic = fopen(basic_csv, "a");
    if (fp_basic != NULL) {
        if (basic_header) {
            /* 헤더: Timestamp와 각 지표 및 단위 */
            fprintf(fp_basic, "Timestamp,CPU Usage (%%),Memory Usage (%%),Disk Usage (%%),CPU Temp (°C),Net RX (bytes/sec),Net TX (bytes/sec)\n");
        }
        float cpu_usage = get_cpu_usage();
        float mem_usage = get_memory_usage();
        float disk_usage = get_disk_usage();
        float cpu_temp = get_cpu_temperature();
        float rx_rate = 0, tx_rate = 0;
        get_network_traffic(&rx_rate, &tx_rate);
        fprintf(fp_basic, "%s,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n", timestamp, cpu_usage, mem_usage, disk_usage, cpu_temp, rx_rate, tx_rate);
        fclose(fp_basic);
    }

    /* 하드웨어 종속 지표 CSV 파일: hwinfo_YYYYMMDD.csv */
    char hwinfo_csv[512];
    snprintf(hwinfo_csv, sizeof(hwinfo_csv), "%s/hwinfo_%04d%02d%02d.csv", 
             daily_dir, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    int hwinfo_header = (access(hwinfo_csv, F_OK) != 0);
    FILE *fp_hwinfo = fopen(hwinfo_csv, "a");
    if (fp_hwinfo != NULL) {
        if (hwinfo_header) {
            fprintf(fp_hwinfo, "Timestamp,Power Status,Fan Status,RAID Status\n");
        }
        const char *power_status = get_power_module_status();
        const char *fan_status = get_fan_status();
        const char *raid_status = get_raid_status();
        fprintf(fp_hwinfo, "%s,%s,%s,%s\n", timestamp, power_status, fan_status, raid_status);
        fclose(fp_hwinfo);
    }
}

/* CSV 로그 보관 기간 초과된 디렉토리를 삭제하는 함수 */
void cleanup_old_csv_logs(void) {
    DIR *dp;
    struct dirent *entry;
    dp = opendir(LOG_DIR);
    if (dp == NULL)
        return;

    time_t now = time(NULL);
    int retention = global_config.csv_retention_days;  // conf 파일에서 설정한 보관 기간

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_type == DT_DIR) {
            // 디렉토리 이름이 8자리(YYYYMMDD)인지 확인
            if (strlen(entry->d_name) == 8) {
                int valid = 1;
                for (int i = 0; i < 8; i++) {
                    if (!isdigit(entry->d_name[i])) {
                        valid = 0;
                        break;
                    }
                }
                if (!valid)
                    continue;

                // 디렉토리 이름을 파싱하여 time_t로 변환
                char year_str[5], month_str[3], day_str[3];
                strncpy(year_str, entry->d_name, 4);
                year_str[4] = '\0';
                strncpy(month_str, entry->d_name + 4, 2);
                month_str[2] = '\0';
                strncpy(day_str, entry->d_name + 6, 2);
                day_str[2] = '\0';

                struct tm tm_dir = {0};
                tm_dir.tm_year = atoi(year_str) - 1900;
                tm_dir.tm_mon = atoi(month_str) - 1;
                tm_dir.tm_mday = atoi(day_str);
                time_t dir_time = mktime(&tm_dir);
                if (dir_time == -1)
                    continue;

                double diff_days = difftime(now, dir_time) / (60 * 60 * 24);
                if (diff_days > retention) {
                    // 해당 디렉토리 삭제 (rm -rf 사용)
                    char dir_path[512];
                    snprintf(dir_path, sizeof(dir_path), "%s/%s", LOG_DIR, entry->d_name);
                    char cmd[1024];
                    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", dir_path);
                    system(cmd);
                }
            }
        }
    }
    closedir(dp);
}