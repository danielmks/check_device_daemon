#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int interval_minutes;
    float cpu_usage_threshold;
    float mem_usage_threshold;
    float disk_usage_threshold;
    float cpu_temp_threshold;
    float net_rx_threshold;
    float net_tx_threshold;
    int snmp_trap_enable;
    char snmp_trap_dest[64];
    int snmp_trap_port;
    int syslog_enable;
    char net_interface[64];
} config_t;

extern config_t global_config;

/* 설정 파일을 읽어 config 구조체에 값을 채운다.
   설정 파일 경로를 인자로 받는다. */
int check_config(const char *conf_path, config_t *config);

#endif // CONFIG_H
