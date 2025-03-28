#include "alarms.h"
#include "metrics.h"
#include "config.h"
#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fanmonitor.h"

/* SNMP 관련 헤더 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#define RAID_OID ".1.3.6.1.4.1.8072.2.3.0.9"
#define SSD0_OID ".1.3.6.1.4.1.8072.2.3.0.10"
#define SSD1_OID ".1.3.6.1.4.1.8072.2.3.0.11"
#define FAN_OID  ".1.3.6.1.4.1.8072.2.3.0.8"
#define POWER_OID ".1.3.6.1.4.1.8072.2.3.0.7"

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
    session.community = (u_char *)global_config.snmp_trap_community;
    session.community_len = strlen(global_config.snmp_trap_community);

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

    RaidInfo raidInfo = get_raid_info();
    /* RAID 상태 알람: RAID 상태가 "Optimal"이 아니면 알람 */
    if(strcasecmp(raidInfo.raid_state, "Optimal") != 0) {
        if(global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: RAID state abnormal: %s, Level: %s", raidInfo.raid_state, raidInfo.raid_level);
        send_snmp_trap(RAID_OID, "RAID state alarm triggered");
    }

    /* SSD 슬롯 상태 알람 */
    if (strcasecmp(raidInfo.ssd0_status, "Online") != 0) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: SSD0 status abnormal: %s", raidInfo.ssd0_status);
        send_snmp_trap(SSD0_OID, "SSD0 status alarm triggered");
    }
    if (strcasecmp(raidInfo.ssd1_status, "Online") != 0) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: SSD1 status abnormal: %s", raidInfo.ssd1_status);
        send_snmp_trap(SSD1_OID, "SSD1 status alarm triggered");
    }


    /* 팬 상태 알람 */
    FanInfo fanInfo = get_fan_info();
    if (fanInfo.cpuFan <= 0 || fanInfo.auxFan <= 0 || 
        fanInfo.fan1 <= 0 || fanInfo.fan2 <= 0 || fanInfo.fan3 <= 0) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: Fan speed abnormal: CPU Fan=%d, Aux Fan=%d, FAN1=%d, FAN2=%d, FAN3=%d",
                   fanInfo.cpuFan, fanInfo.auxFan, fanInfo.fan1, fanInfo.fan2, fanInfo.fan3);
        send_snmp_trap(FAN_OID, "Fan speed alarm triggered");
    }

    /* 전원(Power) 상태 알람 */
    PowerInfo powerInfo = get_power_info();
    /* 두 채널 모두 "OK"여야 정상. 하나라도 "OK"가 아니면 알람 발생 */
    if (strcasecmp(powerInfo.power1, "OK") != 0 || strcasecmp(powerInfo.power2, "OK") != 0) {
        if (global_config.syslog_enable)
            syslog(LOG_ALERT, "ALARM: Power state abnormal: Power1=%s, Power2=%s", 
                   powerInfo.power1, powerInfo.power2);
        send_snmp_trap(POWER_OID, "Power state alarm triggered");
    }

}
