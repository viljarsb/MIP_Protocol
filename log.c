#include <time.h>
#include <stdio.h>
#include "log.h"
void timestamp()
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  printf("%02d:%02d:%02d     ", tm.tm_hour, tm.tm_min, tm.tm_sec);
}
