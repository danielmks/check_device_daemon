#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>

// CPU 시간을 읽어들이기 위한 구조체 및 내부 함수
typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} cpu_times_t;

static int read_cpu_times(cpu_times_t *times) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp)
        return -1;
    char buffer[256];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    int ret = sscanf(buffer, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
                     &times->user, &times->nice, &times->system, &times->idle,
                     &times->iowait, &times->irq, &times->softirq, &times->steal);
    return (ret >= 4) ? 0 : -1;
}

float get_cpu_usage(void) {
    cpu_times_t t1, t2;
    if (read_cpu_times(&t1) != 0)
        return -1;
    sleep(1);
    if (read_cpu_times(&t2) != 0)
        return -1;
    unsigned long long total1 = t1.user + t1.nice + t1.system + t1.idle +
                                  t1.iowait + t1.irq + t1.softirq + t1.steal;
    unsigned long long total2 = t2.user + t2.nice + t2.system + t2.idle +
                                  t2.iowait + t2.irq + t2.softirq + t2.steal;
    unsigned long long total_diff = total2 - total1;
    unsigned long long idle_diff = (t2.idle + t2.iowait) - (t1.idle + t1.iowait);
    return (total_diff == 0) ? 0 : (float)(total_diff - idle_diff) / total_diff * 100;
}

float get_memory_usage(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp)
        return -1;
    char line[256];
    unsigned long long memTotal = 0, memAvailable = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %llu kB", &memTotal) == 1) { }
        else if (sscanf(line, "MemAvailable: %llu kB", &memAvailable) == 1) { }
        if (memTotal && memAvailable)
            break;
    }
    fclose(fp);
    return (memTotal == 0) ? 0 : (float)(memTotal - memAvailable) / memTotal * 100;
}

float get_disk_usage(void) {
    struct statvfs vfs;
    if (statvfs("/", &vfs) != 0)
        return -1;
    unsigned long total = vfs.f_blocks;
    unsigned long free = vfs.f_bfree;
    unsigned long used = total - free;
    return (total == 0) ? 0 : (float)used / total * 100;
}

float get_cpu_temperature(void) {
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp)
        return -1;
    int temp_milli;
    fscanf(fp, "%d", &temp_milli);
    fclose(fp);
    return temp_milli / 1000.0;
}

int get_network_traffic(float *rx_rate, float *tx_rate) {
    FILE *fp;
    char line[512];
    unsigned long long rx1 = 0, tx1 = 0, rx2 = 0, tx2 = 0;
    
    fp = fopen("/proc/net/dev", "r");
    if (!fp)
        return -1;
    // Skip header lines
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        char iface[32];
        unsigned long long r, t;
        if (sscanf(line, " %[^:]: %llu %*s %*s %*s %*s %*s %*s %*s %llu", iface, &r, &t) == 3) {
            if (strcmp(iface, "lo") != 0) {
                rx1 += r;
                tx1 += t;
            }
        }
    }
    fclose(fp);
    sleep(1);
    fp = fopen("/proc/net/dev", "r");
    if (!fp)
        return -1;
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        char iface[32];
        unsigned long long r, t;
        if (sscanf(line, " %[^:]: %llu %*s %*s %*s %*s %*s %*s %*s %llu", iface, &r, &t) == 3) {
            if (strcmp(iface, "lo") != 0) {
                rx2 += r;
                tx2 += t;
            }
        }
    }
    fclose(fp);
    *rx_rate = (float)(rx2 - rx1);
    *tx_rate = (float)(tx2 - tx1);
    return 0;
}

RaidInfo get_raid_info(void) {
    RaidInfo info;
    memset(&info, 0, sizeof(info));
    FILE *fp;
    char line[256];

    /* 1. RAID 상태 및 레벨 확인 명령어 */
    const char *raid_cmd =
        "/opt/MegaRAID/MegaCli/MegaCli64 -ShowSummary -aALL | grep -A 7 \"Storage\" | "
        "awk \"{ if(\\$0 ~ /State/) { stat=\\$NF } else if(\\$0 ~ /RAID Level/) { raid=\\$NF } } "
        "END { print stat, raid }\"";
    
    //Optimal 1 로출력됨(쉼표 안나옴)
    fp = popen(raid_cmd, "r");
    if (fp != NULL) {
        //syslog(LOG_DEBUG, "DEBUG: Executing command: %s", raid_cmd);
        if (fgets(line, sizeof(line), fp) != NULL) {
            //syslog(LOG_DEBUG, "DEBUG: popen output: %s", line);
            // 공백을 기준으로 토큰 분리
            char *token = strtok(line, " \t\n");
            if (token != NULL) {
                strncpy(info.raid_state, token, sizeof(info.raid_state) - 1);
                token = strtok(NULL, " \t\n");
                if (token != NULL) {
                    strncpy(info.raid_level, token, sizeof(info.raid_level) - 1);
                } else {
                    strncpy(info.raid_level, "Unknown", sizeof(info.raid_level) - 1);
                }
            }
        }
        pclose(fp);
    } else {
        syslog(LOG_ERR, "DEBUG: RAID popen() failed: %s", raid_cmd);
        strncpy(info.raid_state, "Unknown", sizeof(info.raid_state)-1);
        strncpy(info.raid_level, "Unknown", sizeof(info.raid_level)-1);
    }
    
    /* 2. 슬롯(SSD) 상태 확인 명령어 */
    const char *pd_cmd =
        "/opt/MegaRAID/MegaCli/MegaCli64 -PDList -aALL | grep -E \"Slot Number:|Firmware state:\" | "
        "awk '{ if($0 ~ /Slot Number:/) { slot=$3 } else if($0 ~ /Firmware state:/) { print \"Slot \" slot \": \" $3, substr($0, index($0,$4)) } }'";
    
    fp = popen(pd_cmd, "r");
    if (fp != NULL) {
        //syslog(LOG_DEBUG, "DEBUG: Executing PD command: %s", pd_cmd);
        int count = 0;
        while (fgets(line, sizeof(line), fp) != NULL && count < 2) {
            //syslog(LOG_DEBUG, "DEBUG: PD popen output: %s", line);
            line[strcspn(line, "\n")] = '\0';  // 개행 제거
            /* 만약 "Online" 문자열이 있으면 "Online", 아니면 "Not Online" */
            if (strstr(line, "Online") != NULL) {
                if (count == 0)
                    strncpy(info.ssd0_status, "Online", sizeof(info.ssd0_status)-1);
                else if (count == 1)
                    strncpy(info.ssd1_status, "Online", sizeof(info.ssd1_status)-1);
            } else {
                if (count == 0)
                    strncpy(info.ssd0_status, "Not Online", sizeof(info.ssd0_status)-1);
                else if (count == 1)
                    strncpy(info.ssd1_status, "Not Online", sizeof(info.ssd1_status)-1);
            }
            count++;
        }
        pclose(fp);
    } else {
        syslog(LOG_ERR, "DEBUG: PD popen() failed: %s", pd_cmd);
        strncpy(info.ssd0_status, "Unknown", sizeof(info.ssd0_status)-1);
        strncpy(info.ssd1_status, "Unknown", sizeof(info.ssd1_status)-1);
    }
    
    return info;
}

/* 외부 라이브러리의 함수 선언 */
extern int Redundant_Power(int *status);

/* 전원 정보를 개별로 확인하는 함수 */
PowerInfo get_power_info(void) {
    PowerInfo pinfo;
    memset(&pinfo, 0, sizeof(pinfo));
    int re = -1;
    
    if (Redundant_Power(&re) != 0) {
        strncpy(pinfo.power1, "Unknown", sizeof(pinfo.power1)-1);
        strncpy(pinfo.power2, "Unknown", sizeof(pinfo.power2)-1);
        return pinfo;
    }
    /* 예를 들어, 아래와 같이 처리:
       - re == 0: "OK", "OK"
       - re == 1: "Fail", "OK"
       - re == 2: "OK", "Fail"
       (필요에 따라 다른 값도 처리)
    */
    if (re == 0) {
        strncpy(pinfo.power1, "OK", sizeof(pinfo.power1)-1);
        strncpy(pinfo.power2, "OK", sizeof(pinfo.power2)-1);
    } else if (re == 1) {
        strncpy(pinfo.power1, "Fail", sizeof(pinfo.power1)-1);
        strncpy(pinfo.power2, "OK", sizeof(pinfo.power2)-1);
    } else if (re == 2) {
        strncpy(pinfo.power1, "OK", sizeof(pinfo.power1)-1);
        strncpy(pinfo.power2, "Fail", sizeof(pinfo.power2)-1);
    } else {
        strncpy(pinfo.power1, "Unknown", sizeof(pinfo.power1)-1);
        strncpy(pinfo.power2, "Unknown", sizeof(pinfo.power2)-1);
    }
    return pinfo;
}