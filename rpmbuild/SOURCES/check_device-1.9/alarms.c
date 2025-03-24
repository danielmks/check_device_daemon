#include "alarms.h"
#include "metrics.h"
#include "config.h"
#include <syslog.h>
#include <string.h>
#include <stdio.h>

/* SNMP 관련 헤더 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#define CONFIG_FILE "/etc/check_device/check_device.conf"

/* 임계치 (필요에 따라 수정) */
#define POWER_STATUS_OK      "OK"
#define FAN_STATUS_OK        "Normal"
#define RAID_STATUS_OK       "OK"

/* SNMP 트랩 전송 함수 (SNMPv2c, 커뮤니티 "public") */
void send_snmp_trap(const char *trap_oid, const char *message) {
    if (global_config.snmp_trap_enable != 1){
        return;
    }
    
    char dest[128];
    snprintf(dest, sizeof(dest), "%s:%d", global_config.snmp_trap_dest, global_config.snmp_trap_port);

    struct snmp_session session, *ss;
    struct snmp_pdu *pdu;
    oid objid[MAX_OID_LEN];
    size_t objid_len = MAX_OID_LEN;

    snmp_sess_init(&session);
    /* 매크로로 정의된 대상 주소 사용 */
    session.peername = strdup(dest);
    session.version = SNMP_VERSION_2c;
    session.community = (u_char *)"public";
    session.community_len = strlen("public");

    ss = snmp_open(&session);
    if (!ss) {
        snmp_perror("snmp_open");
        free(session.peername);
        return;
    }
    free(session.peername);

    pdu = snmp_pdu_create(SNMP_MSG_TRAP2);
    if (!snmp_parse_oid(trap_oid, objid, &objid_len)) {
        snmp_perror(trap_oid);
        snmp_close(ss);
        return;
    }
    snmp_add_var(pdu, objid, objid_len, 's', message);

    if (snmp_send(ss, pdu) == 0) {
        snmp_free_pdu(pdu);
    }
    snmp_close(ss);
}

/* 알람 조건 검사 및 알람 전송 */
void check_and_alarm(void) {
    float cpu_usage = get_cpu_usage();
    float mem_usage = get_memory_usage();
    float disk_usage = get_disk_usage();
    float cpu_temp = get_cpu_temperature();
    float rx_rate = 0, tx_rate = 0;
    get_network_traffic(&rx_rate, &tx_rate);
    const char* power_status = get_power_module_status();
    const char* fan_status = get_fan_status();
    const char* raid_status = get_raid_status();

    if (cpu_usage > global_config.cpu_usage_threshold) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: CPU usage high: %.1f%%", cpu_usage);
        send_snmp_trap(".1.3.6.1.4.1.8072.2.3.0.1", "CPU usage high alarm triggered");
    }
    if (mem_usage > global_config.mem_usage_threshold) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: Memory usage high: %.1f%%", mem_usage);
        send_snmp_trap(".1.3.6.1.4.1.8072.2.3.0.2", "Memory usage high alarm triggered");
    }
    if (disk_usage > global_config.disk_usage_threshold) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: Disk usage high: %.1f%%", disk_usage);
        send_snmp_trap(".1.3.6.1.4.1.8072.2.3.0.3", "Disk usage high alarm triggered");
    }
    if (cpu_temp > global_config.cpu_temp_threshold) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: CPU temperature high: %.1f°C", cpu_temp);
        send_snmp_trap(".1.3.6.1.4.1.8072.2.3.0.4", "CPU temperature high alarm triggered");
    }
    if (rx_rate > global_config.net_rx_threshold) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: Network RX high: %.1f bytes/sec", rx_rate);
        send_snmp_trap(".1.3.6.1.4.1.8072.2.3.0.5", "Network RX high alarm triggered");
    }
    if (tx_rate > global_config.net_tx_threshold) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: Network TX high: %.1f bytes/sec", tx_rate);
        send_snmp_trap(".1.3.6.1.4.1.8072.2.3.0.6", "Network TX high alarm triggered");
    }
    if (strcmp(power_status, "OK") != 0) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: Power module status: %s", power_status);
        send_snmp_trap(".1.3.6.1.4.1.8072.2.3.0.7", "Power module alarm triggered");
    }
    if (strcmp(fan_status, "Normal") != 0) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: Fan status: %s", fan_status);
        send_snmp_trap(".1.3.6.1.4.1.8072.2.3.0.8", "Fan status alarm triggered");
    }
    if (strcmp(raid_status, "OK") != 0) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: RAID status: %s", raid_status);
        send_snmp_trap(".1.3.6.1.4.1.8072.2.3.0.9", "RAID status alarm triggered");
    }
}

/* 초기화 시 설정 파일을 읽어 전역 변수에 저장 */
void init_config(void) {
    if (check_config(CONFIG_FILE, &global_config) != 0) {
        syslog(LOG_ERR, "Failed to read configuration file: %s", CONFIG_FILE);
        /* 실패시 기본값을 설정하거나 종료 처리할 수 있음 */
    }
}