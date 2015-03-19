#include "timeutils.h"
#include <stdlib.h>
#include <string.h>

#include "chprintf.h"
#include "chrtclib.h"


time_t secondsinceepoch()
{
    return rtcGetTimeUnixSec((RTCDriver *)&RTCD1);
}

