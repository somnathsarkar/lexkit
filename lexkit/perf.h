#ifndef __LEXKIT_PERF__
#define __LEXKIT_PERF__

#ifndef MEASURE_PERF
#define MEASURE_PERF 0
#endif

// TODO: Change perf API to blocks to remove all the include guards in user code

#include <lexkit/sizes.h>

i64 timestamp();
int64_t timestamp_res();
void print_time_ms(const char* label, int64_t ts_diff);

#endif  // __LEXKIT_PERF__
