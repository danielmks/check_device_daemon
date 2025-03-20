/*
 * 파일명: check_device.c
 * 설명: 데몬화하여 매 INTERVAL_MINUTES 분마다 시스템 자원(CPU, 메모리, 디스크 사용률, CPU 온도)을 측정하고,
 *       CSV 파일에 기록하며(파일명: check_device_log_YYYYMMDD.csv),
 *       특정 임계치를 초과하면 syslog를 통해 알람을 전송하는 서비스 프로그램.
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
 #include <sys/statvfs.h>
 #include <syslog.h>
 
 #define PROCESS_NAME         "check_device"
 #define INTERVAL_MINUTES     5    // 체크 간격 (분)
 
 // 임계치 설정 (필요에 따라 조정)
 #define CPU_USAGE_THRESHOLD  80.0    // CPU 사용률 임계치 (%)
 #define MEM_USAGE_THRESHOLD  90.0    // 메모리 사용률 임계치 (%)
 #define DISK_USAGE_THRESHOLD 95.0    // 디스크 사용률 임계치 (%)
 #define CPU_TEMP_THRESHOLD   75.0    // CPU 온도 임계치 (°C)
 
 #define LOG_DIR "/var/log/check_device"  // CSV 파일을 저장할 디렉토리
 
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
 
 // 1초 간격으로 측정하여 CPU 사용률(%) 계산 함수
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
     return (float)(total_diff - idle_diff) / total_diff * 100;
 }
 
 // /proc/meminfo에서 메모리 사용률(%) 계산 함수
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
     return (float)(memTotal - memAvailable) / memTotal * 100;
 }
 
 // statvfs()를 이용해 루트 파일시스템의 디스크 사용률(%) 계산 함수
 float get_disk_usage() {
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
 
 // CPU 온도를 읽어 °C 단위로 반환하는 함수
 float get_cpu_temperature() {
     FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
     if (!fp)
         return -1;
     int temp_milli;
     fscanf(fp, "%d", &temp_milli);
     fclose(fp);
     return temp_milli / 1000.0;
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
 
 // 측정값이 임계치를 초과하면 syslog로 알람 전송
 void check_and_alarm() {
     float cpu_usage = get_cpu_usage();
     float mem_usage = get_memory_usage();
     float disk_usage = get_disk_usage();
     float cpu_temp = get_cpu_temperature();
 
     if (cpu_usage < 0 || mem_usage < 0 || disk_usage < 0 || cpu_temp < 0) {
         syslog(LOG_ERR, "Error reading system metrics");
         return;
     }
 
     if (cpu_usage > CPU_USAGE_THRESHOLD)
         syslog(LOG_ALERT, "ALARM: CPU usage high: %.1f%%", cpu_usage);
     if (mem_usage > MEM_USAGE_THRESHOLD)
         syslog(LOG_ALERT, "ALARM: Memory usage high: %.1f%%", mem_usage);
     if (disk_usage > DISK_USAGE_THRESHOLD)
         syslog(LOG_ALERT, "ALARM: Disk usage high: %.1f%%", disk_usage);
     if (cpu_temp > CPU_TEMP_THRESHOLD)
         syslog(LOG_ALERT, "ALARM: CPU temperature high: %.1f°C", cpu_temp);
 }
 
 // CSV 파일에 (날짜, 시간, CPU 사용률, 메모리 사용률, 디스크 사용률, CPU 온도)를 기록하는 함수
 void write_csv_log() {
     time_t now = time(NULL);
     struct tm *tm_info = localtime(&now);
     char csv_filename[256];
     
     // CSV 파일명: "check_device_log_YYYYMMDD.csv"
     snprintf(csv_filename, sizeof(csv_filename), "%s_log_%04d%02d%02d.csv",
              PROCESS_NAME, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
 
     char csv_filepath[512];
     snprintf(csv_filepath, sizeof(csv_filepath), "%s/%s", LOG_DIR, csv_filename);
 
     // 측정값 읽기
     float cpu_usage = get_cpu_usage();
     float mem_usage = get_memory_usage();
     float disk_usage = get_disk_usage();
     float cpu_temp = get_cpu_temperature();
 
     // 날짜와 시간 문자열
     char dateStr[16];
     char timeStr[16];
     strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tm_info);
     strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);
 
     // CSV 파일에 기록 (파일이 없으면 헤더를 먼저 작성)
     int write_header = (access(csv_filepath, F_OK) != 0);
     FILE *fp = fopen(csv_filepath, "a");
     if (fp != NULL) {
         if (write_header) {
             fprintf(fp, "Date,Time,CPU Usage,Memory Usage,Disk Usage,CPU Temp\n");
         }
         fprintf(fp, "%s,%s,%.1f,%.1f,%.1f,%.1f\n", dateStr, timeStr, cpu_usage, mem_usage, disk_usage, cpu_temp);
         fclose(fp);
     }
 }
 
 // 로그 저장 디렉토리가 없으면 생성
 void ensure_log_dir() {
     struct stat st;
     if (stat(LOG_DIR, &st) == -1) {
         mkdir(LOG_DIR, 0755);
     }
 }
 
 int main() {
     // 데몬화
     daemonize();
 
     // syslog 오픈: 식별자와 facility 지정
     openlog(PROCESS_NAME, LOG_PID, LOG_DAEMON);
 
     // CSV 파일을 저장할 로그 디렉토리 확인 및 생성
     ensure_log_dir();
 
     // 무한 루프: 매 INTERVAL_MINUTES 분마다 측정, 알람 전송, CSV 기록
     while (1) {
         check_and_alarm();
         write_csv_log();
         sleep(INTERVAL_MINUTES * 60);
     }
 
     // unreachable
     closelog();
     return 0;
 }
 