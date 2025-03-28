#ifndef METRICS_H
#define METRICS_H

float get_cpu_usage(void);
float get_memory_usage(void);
float get_disk_usage(void);
float get_cpu_temperature(void);
int get_network_traffic(float *rx_rate, float *tx_rate);

/* RAID 및 SSD 상태 정보를 위한 구조체와 함수 선언 */
typedef struct {
    char raid_state[64];   /* 예: "Optimal" */
    char raid_level[16];   /* 예: "1" */
    char ssd0_status[128]; /* 예: "Slot 0: Online, Spun Up" */
    char ssd1_status[128]; /* 예: "Slot 1: Online, Spun Up" */
} RaidInfo;

RaidInfo get_raid_info(void);

/* 전원 정보: Redundant_Power() 함수를 이용 */
typedef struct {
    char power1[16];   /* 예: "OK" 또는 "Fail" */
    char power2[16];   /* 예: "OK" 또는 "Fail" */
} PowerInfo;

PowerInfo get_power_info(void);

#endif // METRICS_H
