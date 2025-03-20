#include "daemon.h"
#include "alarms.h"
#include "logging.h"
#include <syslog.h>
#include <unistd.h>

#define INTERVAL_MINUTES 5

int main(void) {
    //daemonize();

    openlog("check_device", LOG_PID, LOG_DAEMON);
    ensure_log_dir();

    while (1) {
        check_and_alarm();
        write_csv_log();
        sleep(INTERVAL_MINUTES * 60);
    }

    closelog();
    return 0;
}
