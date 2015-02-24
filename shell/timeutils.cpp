#include "timeutils.h"
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

  if (argc == 0) {
    goto ERROR;
  }

  if ((argc == 1) && (strcmp(argv[0], "get") == 0)){
    unix_time = rtcGetTimeUnixSec((RTCDriver *)&RTCD1);

    if (unix_time == -1){
      chprintf(chp, "incorrect time in RTC cell\r\n");
    }
    else{
      // chprintf(chp, "%D\r\n",unix_time);
      rtcGetTimeTm(&RTCD1, &timp);
      chprintf(chp, "%s (GMT)\r\n",asctime(&timp));
    }
    return;
  }

  if ((argc == 2) && (strcmp(argv[0], "set") == 0)){
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
  chprintf(chp, "Usage: date get\r\n");
  chprintf(chp, "       date set N\r\n");
  chprintf(chp, "where N is time in seconds sins Unix epoch\r\n");
  chprintf(chp, "you can get current N value from unix console by the command\r\n");
  chprintf(chp, "%s", "date +\%s\r\n");
  return;
}


