#ifndef LOGGING_H
#define LOGGING_H

void ensure_log_dir(void);
void write_csv_log(void);
void cleanup_old_csv_logs(void);

#endif // LOGGING_H
