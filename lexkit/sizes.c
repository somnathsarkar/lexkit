#include <lexkit/sizes.h>

u64 Gigabytes(u64 gb)
{
  return 1024llu * 1024llu * 1024llu * gb;
}

u64 Megabytes(u64 gb)
{
  return 1024llu * 1024llu * gb;
}
