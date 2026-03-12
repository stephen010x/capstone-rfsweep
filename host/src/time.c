
#include <stdint.h>
#include <time.h>
#include <sched.h>


#include "toolkit/debug.h"
#include "toolkit/macros.h"
#include "rfsweep/host.h"


static uint64_t start_us = 0;




// __inline__ uint64_t get_time(void) {
//     int err;
//     err = clock_gettime(CLOCK_MONOTONIC, &time);
//     DEBUG(
//     assert(("unable to get system time", !err), 0);
//     );
//     return 
// }



__inline__ int64_t _timespec_to_us(struct timespec *time) {
    return (int64_t)time->tv_nsec / 1000 + (int64_t)time->tv_sec * (int64_t)1e6;
}




static __construct void time_init(void) {
    int err;
    struct timespec time;

    err = clock_gettime(CLOCK_MONOTONIC, &time);
    vassert(("unable to get system time", !err));

    start_us = _timespec_to_us(&time);

    DEBUG(
    err = clock_getres(CLOCK_MONOTONIC, &time);
    vassert(!err);

    double res = (double)time.tv_nsec + (double)time.tv_sec * 10e9;
    debugf("MONOTONIC CLOCK resolution is %.0f ns", res);
    )
}


// process uptime in microseconds
int64_t micros(void) {
    int err;
    struct timespec time;
    (void)err;

    err = clock_gettime(CLOCK_MONOTONIC, &time);

    DEBUG(
    assert(("unable to get system time", !err), 0);
    )

    return _timespec_to_us(&time) - start_us;
}


// yield wait
void micros_block_for(int64_t u) {
    int64_t uend = micros() + u;

    while (micros() < uend) {
        sched_yield();
    }
}


// busy wait
void micros_busy_for(int64_t u) {
    int64_t uend = micros() + u;

    while (micros() < uend);
}



DEBUG(
void micros_test(void) {
    int64_t times[12];
    
    times[0] = micros();
    times[1] = micros();
    micros_block_for(0);
    times[2] = micros();
    micros_block_for(1);
    times[3] = micros();
    micros_block_for(10);
    times[4] = micros();
    micros_block_for(100);
    times[5] = micros();
    micros_block_for(1000);
    times[6] = micros();

    times[7] = micros();
    micros_busy_for(1);
    times[8] = micros();
    micros_busy_for(10);
    times[9] = micros();
    micros_busy_for(100);
    times[10] = micros();
    micros_busy_for(1000);
    times[11] = micros();

    for (int i = 0; i <= 11; i++)
        debugf("%d) %lld us", i, (long long)times[i]);
}
)
