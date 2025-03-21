#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <string.h>

// 내부에서만 사용하는 구조체 및 함수
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
    if (ret < 4)
        return -1;
    return 0;
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
    if (total_diff == 0)
        return 0;
    return (float)(total_diff - idle_diff) / total_diff * 100;
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
    if (memTotal == 0)
        return 0;
    return (float)(memTotal - memAvailable) / memTotal * 100;
}

float get_disk_usage(void) {
    struct statvfs vfs;
    if (statvfs("/", &vfs) != 0)
        return -1;
    unsigned long total = vfs.f_blocks;
    unsigned long free = vfs.f_bfree;
    unsigned long used = total - free;
    if (total == 0)
        return 0;
    return (float)used / total * 100;
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

// rx = recieve packet | tx = transmit packet
int get_network_traffic(float *rx_rate, float *tx_rate) {
    FILE *fp;
    char line[256];
    unsigned long long rx1 = 0, tx1 = 0, rx2 = 0, tx2 = 0;
    
    fp = fopen("/proc/net/dev", "r");
    if (!fp)
        return -1;
    fgets(line, sizeof(line), fp);  // Skip header
    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "eth0:") != NULL) {
            char *ptr = strchr(line, ':');
            if (ptr) {
                sscanf(ptr+1, " %llu %*s %*s %*s %*s %*s %*s %*s %llu", &rx1, &tx1);
            }
            break;
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
        if (strstr(line, "eth0:") != NULL) {
            char *ptr = strchr(line, ':');
            if (ptr) {
                sscanf(ptr+1, " %llu %*s %*s %*s %*s %*s %*s %*s %llu", &rx2, &tx2);
            }
            break;
        }
    }
    fclose(fp);
    *rx_rate = (float)(rx2 - rx1);
    *tx_rate = (float)(tx2 - tx1);
    return 0;
}

const char* get_power_module_status(void) {
    FILE *fp = fopen("/sys/class/power_supply/AC/online", "r");
    if (!fp)
        return "Unknown";
    int status = 0;
    fscanf(fp, "%d", &status);
    fclose(fp);
    return (status == 1) ? "OK" : "No AC";
}

const char* get_fan_status(void) {
    // 실제 하드웨어에 맞는 경로로 대체 필요. 여기서는 예시로 "Normal" 반환.
    return "Normal";
}

const char* get_raid_status(void) {
    FILE *fp = fopen("/proc/mdstat", "r");
    if (!fp)
        return "Unknown";
    char buffer[1024];
    int found = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, "md")) {
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found ? "OK" : "No RAID";
}
