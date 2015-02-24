#include "timeutils.h"
#include "shellutils.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chprintf.h"
#include "chrtclib.h"

static time_t unix_time;

void cmd_date(BaseSequentialStream * chp, int argc, char *argv[])
{
  (void)argv;
  struct tm timp;

  if (argc > 1)
    goto ERROR;

  if (argc == 0)
  {
    unix_time = rtcGetTimeUnixSec((RTCDriver *)&RTCD1);

    if (unix_time == -1){
      chprintf(chp, "incorrect time in RTC cell\r\n");
    }
    else{
      // chprintf(chp, "%D\r\n",unix_time);
      rtcGetTimeTm(&RTCD1, &timp);
      chprintf(chp, "%s (GMT)\r\n",strip(asctime(&timp), '\n'));
    }
    return;
  }

  if (argc == 1){
    unix_time = atol(argv[1]);
    if (unix_time > 0){
      rtcSetTimeUnixSec(&RTCD1, unix_time);
      return;
    }
    else{
      goto ERROR;
    }
  }
  else{
    goto ERROR;
  }

ERROR:
  chprintf(chp, "Usage: date\r\n");
  chprintf(chp, "       Tue Feb 24 14:57:09 2015 (GMT)\r\n");
  chprintf(chp, "To set the current date:\r\n");
  chprintf(chp, "       date <N>\r\n");
  chprintf(chp, "where <N> is time in seconds sins Unix epoch\r\n");
  chprintf(chp, "you can get current N value from unix console by the command\r\n");
  chprintf(chp, ">date +\%s\r\n");
  return;
}


