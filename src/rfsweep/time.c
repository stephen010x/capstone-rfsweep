#include <stdint.h>
#include <time.h>
#include <sched.h>
#include <errno.h>


#include "toolkit/debug.h"
#include "toolkit/macros.h"
#include "rfsweep.h"


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
//void time_init(void) {
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
// int64_t __hot __flatten __optimize_fast micros(void) {
//     int err;
//     struct timespec time;
//     (void)err;
// 
//     err = clock_gettime(CLOCK_MONOTONIC, &time);
// 
//     DEBUG(
//     assert(("unable to get system time", !err), 0);
//     )
// 
//     return _timespec_to_us(&time) - start_us;
// }




int64_t __hot __flatten __optimize_fast micros(void) {
    return micros_time() - start_us;
}




// system boot(ish) time in microsecnds
__weak_inline int64_t __hot __flatten __optimize_fast micros_time(void) {
    int err;
    struct timespec time;
    (void)err;

    err = clock_gettime(CLOCK_MONOTONIC, &time);

    DEBUG(
    assert(("unable to get system time", !err), 0);
    )

    return _timespec_to_us(&time);
}




// sleep wait
// will segfault if you pass in an invalid value
// nevermind. Will just return immediately if negative
void __hot __flatten __optimize_fast micros_block_for(int64_t u) {
    //int err;
    struct timespec time;
    //struct timespec rem;
    int64_t secs;

    // if negative value, then don't block at all
    if (u < 0) return;

    //int64_t uend = micros() + u;
    
    // set to absolute time
    u += micros() + start_us;
    
    // calculate seconds
    secs = u / (int64_t)1e6;

    // setup time
    time = (struct timespec) {
        .tv_sec  = secs,
        .tv_nsec = (u - secs * (int64_t)1e6) * 1000,
    };

    // nanosleep has a resolution equivalent to the monotonic clock
    // which generally should be at or below 1us
    // loops to guarentee the full time is waited
    // while (nanosleep(&time, &rem) == -1) && (errno == EINTR))
    //     time = rem;

    while ((clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time, NULL) == -1) && 
          (errno == EINTR));

    // DEBUG(
    // // fatal if nanosleep error
    // // I should use more fatals like this
    // fassert(("invalid value for microseconds", errno != EINVAL));
    // )
}


// busy yield wait
void __hot __flatten __optimize_fast micros_busy_for(int64_t u) {
    if (u < 0) return;

    int64_t uend = micros() + u;

    //while (micros() < uend);

    while (micros() < uend) {
        sched_yield();
    }
}



#ifdef __DEBUG__
void micros_test(void) {

    static int delays[] = {-1, 0, 1, 10, 100, 1000, 10000, 100000, /*1000000*/};
    
    //_Static_assert(lenof(delays) == 5);
    
    int64_t times[lenof(delays)*2 + 1];

    times[0] = micros();

    for (int i = 0; i < (int)lenof(delays); i++) {
        micros_block_for(delays[i]);
        times[i+1] = micros();
    }
    for (int i = 0; i < (int)lenof(delays); i++) {
        micros_busy_for(delays[i]);
        times[i+1+lenof(delays)] = micros();
    }
    
    debugf("micros() - micros() = %lld", (long long)(micros() - micros()));

    for (int i = 1; i <= (int)lenof(times)-1; i++) {
        long long delta = (long long)times[i] - (long long)times[i-1];
        debugf("test %s for %7lld us (measured %7lld us)",
                (i <= (int)lenof(delays)) ? "block" : "busy ",
                (long long)delays[(i-1)%lenof(delays)], delta);
    }



//    times[0] = 0;
//       
//    times[1] = micros();
//    times[2] = micros();
//    
//    times[3] = micros();
//    micros_block_for(1);
//    times[4] = micros();
//    micros_block_for(10);
//    times[5] = micros();
//    micros_block_for(100);
//    times[6] = micros();
//    micros_block_for(1000);
//    times[7] = micros();
// 
//    micros_busy_for(0);
//    times[8] = micros();
//    micros_busy_for(1);
//    times[9] = micros();
//    micros_busy_for(10);
//    times[10] = micros();
//    micros_busy_for(100);
//    times[11] = micros();
//    micros_busy_for(1000);
//    times[12] = micros();

    // for (int i = 1; i <= 12; i++) {
    //     long long delta = (long long)times[i] - (long long)times[i-1];
    //     debugf("%d) %lld us (delta %lld us)", i, (long long)times[i], delta);
    // }
}
#endif
