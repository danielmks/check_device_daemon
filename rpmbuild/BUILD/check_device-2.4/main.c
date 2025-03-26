#include "daemon.h"
#include "alarms.h"
#include "logging.h"
#include "config.h"
#include <syslog.h>
#include <unistd.h>

//#define INTERVAL_MINUTES 5

int main(void) {
    //daemonize();

    //syslog 열기
    openlog("check_device", LOG_PID, LOG_DAEMON);

    init_config();

    ensure_log_dir();

    while (1) {
        check_and_alarm();
        write_csv_log();
        cleanup_old_csv_logs();
        sleep(global_config.interval_seconds);
    }

    //syslog 닫기
    closelog();
    return 0;
}
