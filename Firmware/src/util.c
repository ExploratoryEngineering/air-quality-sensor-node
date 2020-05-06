#include <zephyr.h>

/*

uint64_t atoll(char *number_string)
{
  uint64_t retval;
  int i;

  retval = 0;
  for (; *number_string; number_string++) {
    retval = 10*retval + (*number_string - '0');
  }
  return retval;
}
*/