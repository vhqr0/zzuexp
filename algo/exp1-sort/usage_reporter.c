#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

FILE *reporter_fp;

unsigned long oused, ototal;

void get_stat(unsigned long *used, unsigned long *total) {
  unsigned long user, nice, system, idle, iowait, irq, softirq;
  FILE *stat = fopen("/proc/stat", "r");
  if (!stat) {
    perror("can't open /proc/stat");
    exit(-1);
  }
  if (fscanf(stat, "cpu %ld %ld %ld %ld %ld %ld %ld", &user, &nice, &system,
             &idle, &iowait, &irq, &softirq) != 7) {
    perror("can't scan /proc/stat");
    exit(-1);
  }
  fclose(stat);
  *used = user + nice + system + irq + softirq;
  *total = *used + idle + iowait;
}

double get_cpu_usage() {
  unsigned long used, total;
  get_stat(&used, &total);
  double res = (used - oused) * 100.0 / (total - ototal);
  oused = used;
  ototal = total;
  return res;
}

double get_memory_usage() {
  unsigned long total, free, available;
  FILE *meminfo = fopen("/proc/meminfo", "r");
  if (!meminfo) {
    perror("can't open /proc/meminfo");
    exit(-1);
  }
  if (fscanf(meminfo, "MemTotal: %ld kB\nMemFree: %ld kB\nMemAvailable: %ld kB",
             &total, &free, &available) != 3) {
    perror("can't scan /proc/meminfo");
    exit(-1);
  }
  fclose(meminfo);
  return (total - available) * 100.0 / total;
}

void sig_alrm_handler(int _signum) {
  fprintf(reporter_fp, "cpu: %lf%%, memory: %lf%%\n", get_cpu_usage(),
          get_memory_usage());
  alarm(1);
}

void set_usage_reporter() {
  reporter_fp = fopen("usage_report.txt", "w");
  get_stat(&oused, &ototal);
  signal(SIGALRM, sig_alrm_handler);
  alarm(1);
}
