#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <string.h>

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

const char* get_power_module_status(void) {
    static char status_buf[64];
    FILE *fp = fopen("/sys/class/power_supply/AC/online", "r");
    if (!fp)
        return "Unknown";
    int status = 0;
    fscanf(fp, "%d", &status);
    fclose(fp);
    snprintf(status_buf, sizeof(status_buf), "%s", (status == 1) ? "OK" : "Not OK");
    return status_buf;
}

const char* get_fan_status(void) {
    static char status_buf[64];
    FILE *fp = fopen("/sys/class/hwmon/hwmon0/fan1_input", "r");
    if (!fp)
        return "Unknown";
    if (fgets(status_buf, sizeof(status_buf), fp) == NULL) {
        fclose(fp);
        return "Unknown";
    }
    fclose(fp);
    status_buf[strcspn(status_buf, "\n")] = '\0';
    return status_buf;
}

const char* get_raid_status(void) {
    static char status_buf[64];
    FILE *fp = fopen("/proc/mdstat", "r");
    if (!fp)
        return "Unknown";
    char line[512];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "md")) {
            found = 1;
            break;
        }
    }
    fclose(fp);
    snprintf(status_buf, sizeof(status_buf), "%s", found ? "OK" : "No RAID");
    return status_buf;
}
