/*
 * 파일명: check_device.c
 * 설명: 데몬화하여 매 INTERVAL_MINUTES 분마다 (날짜, 시간, CPU 사용량, 메모리 사용량)을
 *       /var/log/check_device 디렉토리에 기록하는 서비스 프로그램.
 *       로그 파일명은 "check_device_log_YYYYMMDD.log" 형식이며,
 *       지정한 보관 기간(예: 7일) 이후 자동으로 삭제됩니다.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

#define PROCESS_NAME "check_device"          // 프로세스명
#define LOG_RETENTION_DAYS 7                   // 로그 보관 기간 (예: 7일)
#define INTERVAL_MINUTES 5                     // 로그 기록 간격 (n분) - 원하는 값으로 수정 가능
#define LOG_DIR "/var/log/check_device"        // 로그를 저장할 디렉토리

// CPU 시간 정보를 저장할 구조체
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

// /proc/stat에서 CPU 시간을 읽어들이는 함수
int read_cpu_times(cpu_times_t *times) {
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL)
        return -1;
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
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

// 1초 간격으로 측정하여 CPU 사용량(%)을 계산하는 함수
float get_cpu_usage() {
    cpu_times_t t1, t2;
    if (read_cpu_times(&t1) != 0)
        return -1;
    sleep(1);
    if (read_cpu_times(&t2) != 0)
        return -1;
    unsigned long long total1 = t1.user + t1.nice + t1.system + t1.idle + t1.iowait + t1.irq + t1.softirq + t1.steal;
    unsigned long long total2 = t2.user + t2.nice + t2.system + t2.idle + t2.iowait + t2.irq + t2.softirq + t2.steal;
    unsigned long long total_diff = total2 - total1;
    unsigned long long idle_diff = (t2.idle + t2.iowait) - (t1.idle + t1.iowait);
    if (total_diff == 0)
        return 0;
    float usage = (float)(total_diff - idle_diff) / total_diff * 100;
    return usage;
}

// /proc/meminfo에서 메모리 사용량(%)을 계산하는 함수
float get_memory_usage() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp)
        return -1;
    char line[256];
    unsigned long long memTotal = 0, memAvailable = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %llu kB", &memTotal) == 1) {
            // memTotal 읽음
        } else if (sscanf(line, "MemAvailable: %llu kB", &memAvailable) == 1) {
            // memAvailable 읽음
        }
        if (memTotal && memAvailable)
            break;
    }
    fclose(fp);
    if (memTotal == 0)
        return 0;
    float usage = (float)(memTotal - memAvailable) / memTotal * 100;
    return usage;
}

// 데몬화 함수: 프로세스를 백그라운드 데몬으로 전환
void daemonize() {
    pid_t pid, sid;
    
    // 첫 번째 fork: 부모 종료
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // 새로운 세션 생성
    sid = setsid();
    if (sid < 0)
        exit(EXIT_FAILURE);

    // 두 번째 fork: 세션 리더 방지
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // 파일 모드 마스크 초기화
    umask(0);

    // 작업 디렉토리를 루트로 변경
    if (chdir("/") < 0)
        exit(EXIT_FAILURE);

    // 표준 입출력 닫기
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

// 현재 날짜를 이용하여 로그 파일명을 생성하고,
// (날짜, 시간, CPU 사용량, 메모리 사용량) 정보를 기록하는 함수
void write_log() {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char filename[256];
    
    // 로그 파일명: "check_device_log_YYYYMMDD.log"
    snprintf(filename, sizeof(filename), "%s_log_%04d%02d%02d.log",
             PROCESS_NAME, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);

    FILE *fp = fopen(filename, "a");
    if (fp == NULL)
        return;

    // 날짜와 시간 문자열 생성
    char dateStr[16];
    char timeStr[16];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tm_info);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);

    // CPU와 메모리 사용량 측정
    float cpu_usage = get_cpu_usage();
    float mem_usage = get_memory_usage();

    // 로그 기록: 날짜, 시간, CPU 사용량, 메모리 사용량
    fprintf(fp, "%s, %s, CPU: %.1f%%, Memory: %.1f%%\n", dateStr, timeStr, cpu_usage, mem_usage);
    fclose(fp);
}

// 지정된 보관 기간보다 오래된 로그 파일 삭제 함수
void delete_old_logs() {
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // 파일명이 "_log_"와 ".log"를 포함하면 로그 파일로 간주
            if (strstr(dir->d_name, "_log_") != NULL && strstr(dir->d_name, ".log") != NULL) {
                struct stat file_stat;
                if (stat(dir->d_name, &file_stat) == 0) {
                    time_t now = time(NULL);
                    double diff = difftime(now, file_stat.st_mtime);
                    if (diff > LOG_RETENTION_DAYS * 24 * 3600)
                        remove(dir->d_name);
                }
            }
        }
        closedir(d);
    }
}

int main() {
    // 데몬화: 백그라운드에서 실행
    daemonize();

    // 로그 디렉토리로 이동 (없으면 생성)
    if (chdir(LOG_DIR) < 0) {
        if (mkdir(LOG_DIR, 0755) < 0)
            exit(EXIT_FAILURE);
        if (chdir(LOG_DIR) < 0)
            exit(EXIT_FAILURE);
    }

    // 무한 루프: 매 INTERVAL_MINUTES 분마다 로그 기록 및 오래된 로그 삭제
    while (1) {
        write_log();
        delete_old_logs();
        sleep(INTERVAL_MINUTES * 60);
    }

    return 0;
}

