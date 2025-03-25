#ifndef METRICS_H
#define METRICS_H

float get_cpu_usage(void);
float get_memory_usage(void);
float get_disk_usage(void);
float get_cpu_temperature(void);
int get_network_traffic(float *rx_rate, float *tx_rate);

const char* get_power_module_status(void);
const char* get_fan_status(void);
const char* get_raid_status(void);

#endif // METRICS_H
