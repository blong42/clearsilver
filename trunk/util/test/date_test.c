
#include "cs_config.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_date.h"

int main(int argc, char *argv[])
{
  time_t t;
  struct tm ttm;
  char buf[256];

  fprintf(stderr, "Starting...\n");
  fprintf(stderr, "TZ is %s\n", getenv("TZ"));
  fprintf(stderr, "TZ Offset is %ld\n", timezone);

  memset(&ttm, 0, sizeof(struct tm));
  t = 996887354;
  fprintf(stderr, "US/Eastern Test\n");
  neo_time_expand(t, "US/Eastern", &ttm);

  fprintf(stderr, "TZ is %s\n", getenv("TZ"));
  fprintf(stderr, "TZ Offset is %ld\n", timezone);
  fprintf(stderr, "TZ Offset is %ld\n", neo_tz_offset(&ttm));
  fprintf(stderr, "From tm: %s %ld\n", ttm.tm_zone, ttm.tm_gmtoff);
  strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &ttm);
  fprintf(stderr, "Time is %s\n", buf);

  fprintf(stderr, "GMT Test\n");
  neo_time_expand(t, "GMT", &ttm);

  fprintf(stderr, "TZ is %s\n", getenv("TZ"));
  fprintf(stderr, "TZ Offset is %ld\n", timezone);
  fprintf(stderr, "TZ Offset is %ld\n", neo_tz_offset(&ttm));
  fprintf(stderr, "From tm: %s %ld\n", ttm.tm_zone, ttm.tm_gmtoff);
  strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &ttm);
  fprintf(stderr, "Time is %s\n", buf);

  return 0;
}
