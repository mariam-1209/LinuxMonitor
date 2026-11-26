#ifndef METRICS_H
#define METRICS_H
#ifdef __cplusplus
extern "C" {
#endif
int cpu_ticks(unsigned long long *total, unsigned long long *idle);
int memory_info(double *ram,double *swap);
int disk_info(double *used);
double uptime_seconds(void);
#ifdef __cplusplus
}
#endif
#endif
