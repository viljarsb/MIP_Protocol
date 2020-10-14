#include <time.h> //time.
#include <stdio.h> //printf.
#include "log.h" //Signatures of this file.

//This function only prints out a timestamp for debug purposes.
void timestamp()
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  printf("%02d:%02d:%02d     ", tm.tm_hour, tm.tm_min, tm.tm_sec);
}
