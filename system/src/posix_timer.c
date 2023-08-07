#include "posix_timer.h"
#include <time.h>


#define MILLISEC_PER_SECOND     1000
#define NANOSEC_PER_USEC        1000     /* one thousand */
#define USEC_PER_MILLISEC       1000     /* one thousand */

int posix_sleep_ms(unsigned int timeout_ms) {
    struct timespec sleep_time;

    sleep_time.tv_sec = timeout_ms / MILLISEC_PER_SECOND;
    sleep_time.tv_nsec = (timeout_ms % MILLISEC_PER_SECOND) * (NANOSEC_PER_USEC * USEC_PER_MILLISEC);

    return nanosleep(&sleep_time, NULL);
}
