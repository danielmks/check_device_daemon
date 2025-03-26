#include "daemon.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

void daemonize(void) {
    pid_t pid, sid;

    /* 첫 번째 fork: 부모 종료 */
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* 새로운 세션 생성 */
    sid = setsid();
    if (sid < 0)
        exit(EXIT_FAILURE);

    /* 두 번째 fork: 세션 리더 방지 */
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* 파일 모드 마스크 초기화 */
    umask(0);

    /* 작업 디렉토리를 루트로 변경 */
    if (chdir("/") < 0)
        exit(EXIT_FAILURE);

    /* 표준 입출력 닫기 */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}
