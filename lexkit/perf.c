#include <windows.h>
#include <stdio.h>
#include <lexkit/perf.h>

int64_t timestamp() {
  LARGE_INTEGER res = {0};
  QueryPerformanceCounter(&res);
  return res.QuadPart;
}

int64_t timestamp_res() {
  static int64_t res = -1;
  if (res == -1)
  {
    LARGE_INTEGER qpf = {0};
    QueryPerformanceFrequency(&qpf);
    res = qpf.QuadPart;
  }
  return res;
}

void print_time_ms(const char* label, int64_t ts_diff)
{
  printf("%s: %g ms\n", label, ts_diff * 1000.0 / timestamp_res());
}
