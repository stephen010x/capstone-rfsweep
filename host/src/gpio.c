#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>

#ifdef _RASPI
#include <pigpio.h>
#endif

#include "utils/debug.h"
#include "utils/macros.h"
#include "rfsweep/host.h"


// https://media.pbclinear.com/pdfs/pbc-linear-data-sheets/data-sheet-stepper-motor-support.pdf
// https://www.allegromicro.com/~/media/Files/Datasheets/A4988-Datasheet.ashx
// http://abyz.me.uk/rpi/pigpio/cif.html
// https://github.com/joan2937/pigpio
// https://github.com/bminor/glibc/blob/master/posix/sys/types.h


/* 
 * For NEMA 17
 * ===========
 * 200 steps / revolution
 * +-5% step accuracy33
 * 0 to 2000 steps per second (recommended)
 */



/*
 * GPIO16 -> !RESET     (logic low to reset)
 * GPIO17 -> !SLEEP     (logic low to sleep)
 * GPIO18 -> !ENABLE    (logic low to enable)
 * GPIO19 -> STEP       (low to high trigger)
 * GPIO20 -> DIR
 * GPIO21 -> MS1
 * GPIO22 -> MS2
 * GPIO23 -> MS3
 *
 * MS1 MS2 MS3  STEP
 *  L   L   L   1/1
 *  H   L   L   1/2
 *  L   H   L   1/4
 *  H   H   L   1/8
 *  H   H   H   1/16
 */





//#define MAX_STEPS_PER_SEC 2000
#ifndef MAX_STEPS_PER_SEC
#define MAX_STEPS_PER_SEC 400
#endif
#define MIN_MICROS_PER_STEP ((int64_t)10e6/MAX_STEPS_PER_SEC)



#define MASK_MS1 0b001
#define MASK_MS2 0b010
#define MASK_MS3 0b100




#define GPIO_NRESET     16
#define GPIO_NSLEEP     17
#define GPIO_NENABLE    18
#define GPIO_STEP       19
#define GPIO_DIR        20
#define GPIO_MS1        21
#define GPIO_MS2        22
#define GPIO_MS3        23





#ifndef _RASPI

#define PI_OUTPUT  -1
#define PI_PUD_OFF -1

int gpioInitialise(void) {return -1;}
int gpioSetMode(unsigned int, unsigned int) {return -1;}
int gpioSetPullUpDown(unsigned int, unsigned int) {return -1;}
int gpioWrite(unsigned int, unsigned int) {return -1;}
void gpioTerminate(void) {}

#endif







static __construct void init_gpio(void);
static __destruct void exit_gpio(void);
static void *_step_thread(void *args);
static void *_multistep_thread(void *args);
static int _stepper_setdir(step_dir_t dir);
static void _step_inc(void);






//static volatile uint8_t g_step;     // full steps 0-199
//static volatile uint8_t g_fstep;    // fractional steps 0-15 (out of 16)
//static volatile step_mode_t g_mode;
static struct {
    volatile _Atomic uint8_t step;       // full steps 0-199
    volatile _Atomic uint8_t fstep;      // fractional steps 0-15 (out of 16)
    volatile step_mode_t mode;
    volatile _Atomic uint8_t mode_mult;
    volatile _Atomic step_dir_t dir;
    volatile _Atomic uint32_t multistep;
    volatile _Atomic uint8_t dostep;

    pthread_mutex_t step_mutex;
} global = {
    .step_mutex = PTHREAD_MUTEX_INITIALIZER,
};


static pthread_t tthread[2];








static __construct void init_gpio(void) {
    int err; 

    // init gpio library
    err = gpioInitialise();
    #ifndef _RASPI
    warnf("not compiled with pigpio library");
    #endif
    vassert(("failed to initilize gpio library", !err));

    // set the used gpio range to outputs with no pull resistors
    for (int i = 16; i <= 23; i++) {
        err = gpioSetMode(i, PI_OUTPUT);
        vassert(("failed to set gpio pin modes", !err));

        err = gpioSetPullUpDown(i, PI_PUD_OFF);
        vassert(("failed to disable gpio pull resistors", !err));
    }

    // some pin output setup and defaults
    err = 0;
    err |= gpioWrite(GPIO_NRESET,  0);
    err |= gpioWrite(GPIO_NSLEEP,  0);
    err |= gpioWrite(GPIO_NENABLE, 1);
    err |= gpioWrite(GPIO_STEP,    0);
    err |= gpioWrite(GPIO_MS1,     0);
    err |= gpioWrite(GPIO_MS2,     0);
    err |= gpioWrite(GPIO_MS3,     0);
    vassert(("failed to set gpio defaults", !err));

    // create step thread
    err = pthread_create(&tthread[0], NULL, &_step_thread, NULL);
    vassert(("failed to create step thread", !err));

    // create multistep thread
    err = pthread_create(&tthread[1], NULL, &_multistep_thread, NULL);
    vassert(("failed to create step thread", !err));
}


static __destruct void exit_gpio(void) {
    void *retval;

    // cancel threads
    if (tthread[0] != 0) {
        pthread_cancel(tthread[0]);
        pthread_join(tthread[0], &retval);
        wassert(("tthread 0 failed to cancel", retval == PTHREAD_CANCELED));
    }
    if (tthread[1] != 0) {
        pthread_cancel(tthread[1]);
        pthread_join(tthread[1], &retval);
        wassert(("tthread 1 failed to cancel", retval == PTHREAD_CANCELED));
    }

    // terminate gpio
    gpioTerminate();
}




static void *_step_thread(void *args) {
    (void)args;
    int err;

    // ensure that this thread is cancelable
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    for(;;) {
        while(!global.dostep)
            sched_yield();

        // turn on step, stepping motor
        err = gpioWrite(GPIO_STEP, 1);
        wassert(("failed write to GPIO_DIR", !err));

        //micros_block_for(5); // minimum of 1us
        micros_block_for(MIN_MICROS_PER_STEP>>1);
        
        // turn off step, in prep for next turn-on
        err = gpioWrite(GPIO_STEP, 0);
        wassert(("failed write to GPIO_DIR", !err));
        
        micros_block_for(MIN_MICROS_PER_STEP>>1);

        // reset dostep
        global.dostep = 0;

        sched_yield();
    }

    (void)err;

    return NULL;
}




static void *_multistep_thread(void *args) {
    (void)args;
    int err;

    // ensure that this thread is cancelable
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    for(;;) {
        // wait until there are steps to step
        while(global.multistep == 0)
            sched_yield();

        // step in the direction of the last set dir
        stepper_step(global.dir);

        // increment might create a fetch and write atomic race
        //global.multistep--;
        atomic_fetch_sub(&global.multistep, 1);
    }

    (void)err;

    return NULL;
}





// returns true if either stepping or multistepping
bool is_stepping(void) {
    return (global.multistep != 0) || global.dostep;
}


// waits until no longer stepping or multistepping
void stepper_wait(void) {
    while ((global.multistep != 0) || global.dostep)
        sched_yield();
}





int stepper_enable(bool enable) {
    int err = gpioWrite(GPIO_NENABLE, !enable);
    assert(("failed write to GPIO_NENABLE", !err), err);
    return 0;
}



// don't call this while the stepper is running
int stepper_mode(step_mode_t mode) {
    int err;

    DEBUG(
    assert(("step mode must be within the range 0 to 5", (mode >= 0 && mode <= 5)), -1);
    )

    err = 0;
    err |= gpioWrite(GPIO_MS1, mode & MASK_MS1);
    err |= gpioWrite(GPIO_MS2, mode & MASK_MS2);
    err |= gpioWrite(GPIO_MS3, mode & MASK_MS3);
    assert(("failed write to GPIO_MSx", !err), err);

    global.mode = mode;

    // calculate mode multiplier
    global.mode_mult = 1 << (4 - ((mode == 0b111) ? 0b100 : mode));

    return 0;
}



static int _stepper_setdir(step_dir_t dir) {
    int err;

    DEBUG(
    assert(("step direction must be either 0 or 1", (dir == 0 || dir == 1)), -1);
    );

    if (global.dir != dir) {
        err = gpioWrite(GPIO_DIR, dir);
        assert(("failed write to GPIO_DIR", !err), err);
        global.dir = dir;
    }

    return 0;
}




static void _step_inc(void) {
    pthread_mutex_lock(&global.step_mutex);

    uint8_t step  = global.step;
    uint8_t fstep = global.fstep;
    
    fstep += global.mode_mult;
    if (fstep >= 16) {
        step++;
        fstep -= 16;
    }
    if (step >= 200) {
        step = 0;
    }

    global.step = step;
    global.fstep = fstep;

    pthread_mutex_unlock(&global.step_mutex);
}



// this function is blocking if called too frequently
int stepper_step(step_dir_t dir) {
    int err;

    // if current step is active, then block until over
    // the reason is because steps will be lost if we try to 
    // write a 1 to a value that is already set to 1
    while (global.dostep)
        sched_yield();

    // set direction
    err = _stepper_setdir(dir);
    assert(!err, err);

    // increment step and fstep counter
    _step_inc();

    return 0;
}




int stepper_multistep(step_dir_t dir, int32_t steps) {
    int err;

    // block if multistepper is currently running
    while (global.multistep != 0)
        sched_yield();

    // set direction
    err = _stepper_setdir(dir);
    assert(!err, err);

    global.multistep = steps;

    return 0;
}
