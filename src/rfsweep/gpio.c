#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
typedef __WCHAR_TYPE__ ___wchar_t; /* resolves a cygwin error in stdatomic.h for some reason*/
#include <stdatomic.h>
#include <signal.h>

#ifdef _RASPI
#include <pigpio.h>
#endif

// this prevents macro errors incurred by redefined macros that are set
// in rfsweep.h->macros.h before debug.h->stdio.h
#include <stdio.h>

#include "rfsweep.h"
// needs to be below "rfsweep.h" due to cygwin header conflicts
#include "toolkit/debug.h"
#include "toolkit/macros.h"



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





#ifndef MAX_STEPS_PER_SEC
// this is the max steps per second for the base step
// size. It is multipled by the microstep resolution
#define MAX_STEPS_PER_SEC (100*5)
#endif
#define MIN_MICROS_PER_STEP ((int64_t)1e6/MAX_STEPS_PER_SEC)

#define STEPS_PER_REV (200*57/11.0)

// higher modes is smaller multiplier
#define MODE_TO_MULTPOW(__mode) (4 - (((__mode) == 0b111) ? 0b100 : (__mode)))



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
#define PI_INIT_FAILED -1

int gpioInitialise(void) {return -1;}
int gpioSetMode(unsigned int, unsigned int) {return -1;}
int gpioSetPullUpDown(unsigned int, unsigned int) {return -1;}
int gpioWrite(unsigned int, unsigned int) {return -1;}
void gpioTerminate(void) {}

#endif


static bool _gpio_enabled = false;







//static __construct void init_gpio(void);
//static __destruct void exit_gpio(void);
static void *_step_thread(void *args);
static void *_multistep_thread(void *args);
static int _stepper_setdir(step_dir_t dir);
static void _step_inc(void);
static void _step_dec(void);
static void *_stepper_stepto_thread(stephandler_t *handle);
static int _stepper_align(step_mode_t newmode);
//static void _signandler_2(int sig);






//static volatile uint8_t g_step;     // full steps 0-199
//static volatile uint8_t g_fstep;    // fractional steps 0-15 (out of 16)
//static volatile step_mode_t g_mode;
static struct {
    volatile _Atomic int32_t     step;       // full steps 0-199 (overflows)
    volatile _Atomic int8_t      fstep;      // fractional steps 0-15 (out of 16)
    volatile         step_mode_t mode;
    volatile _Atomic uint8_t     mode_mult;  // changed this to 2pow exponent
    volatile _Atomic step_dir_t  dir;
    volatile _Atomic uint32_t    multistep;
    volatile _Atomic uint8_t     dostep;
    volatile _Atomic bool        stepto_run; // set to zero to cancel stepto

    pthread_mutex_t step_mutex;
} global = {
    .step_mutex = PTHREAD_MUTEX_INITIALIZER,
};


static pthread_t __unused tthread[3];






uint8_t stepper_get_multpow(void) {
    return global.mode_mult;
}








//static __construct void init_gpio(void) {
void init_gpio(void) {
    int err; 

    #ifndef _RASPI
    warnf("not compiled with pigpio library");
    #endif
    
    // init gpio library
    err = gpioInitialise();
    vassert(("failed to initilize gpio library", err != PI_INIT_FAILED));

    DEBUG(
    debugf("MAX_STEPS_PER_SEC   = %lld", (long long)MAX_STEPS_PER_SEC);
    debugf("MIN_MICROS_PER_STEP = %lld", (long long)MIN_MICROS_PER_STEP);
    )

    // create ctrl-c signal handler
    //signal(2, &_signandler_2);

    // set the used gpio range to outputs with no pull resistors
    for (int i = 16; i <= 23; i++) {
        err = gpioSetMode(i, PI_OUTPUT);
        vassert(("failed to set gpio pin modes", !err));

        err = gpioSetPullUpDown(i, PI_PUD_OFF);
        vassert(("failed to disable gpio pull resistors", !err));
    }

    // some pin output setup and defaults
    err = 0;
    err |= gpioWrite(GPIO_NRESET,  1);
    err |= gpioWrite(GPIO_NSLEEP,  1);
    err |= gpioWrite(GPIO_NENABLE, 1);
    err |= gpioWrite(GPIO_STEP,    0);
    err |= gpioWrite(GPIO_DIR,     0);
    err |= gpioWrite(GPIO_MS1,     0);
    err |= gpioWrite(GPIO_MS2,     0);
    err |= gpioWrite(GPIO_MS3,     0);
    vassert(("failed to set gpio defaults", !err));


    // // create step thread
    // DEBUG(debugf("creating gpio tthread 0");)
    // err = pthread_create(&tthread[0], NULL, &_step_thread, NULL);
    // vassert(("failed to create step thread", !err));

    // // create multistep thread
    // debugf("creating gpio tthread 1");
    // err = pthread_create(&tthread[1], NULL, &_multistep_thread, NULL);
    // vassert(("failed to create step thread", !err));

    _gpio_enabled = true;
}




//static __destruct void exit_gpio(void) {
void exit_gpio(void) {
    void *retval;
    (void)retval;

    _gpio_enabled = false;

    DEBUG(debugf("canceling gpio threads");)

    // cancel threads
    // if (tthread[0] != 0) {
    //     pthread_cancel(tthread[0]);
    //     pthread_join(tthread[0], &retval);
    //     wassert(("gpio tthread 0 failed to cancel", retval == PTHREAD_CANCELED));
    //     DEBUG(
    //     if (retval == PTHREAD_CANCELED)
    //         debugf("gpio tthread 0 successfully canceled");
    //     )
    // } else {
    //     warnf("tried to cancel gpio tthread 0 not running");
    // }
    
    // if (tthread[1] != 0) {
    //     pthread_cancel(tthread[1]);
    //     pthread_join(tthread[1], &retval);
    //     wassert(("gpio tthread 1 failed to cancel", retval == PTHREAD_CANCELED));
    //     if (retval == PTHREAD_CANCELED)
    //         debugf("gpio tthread 1 successfully canceled");
    // } else {
    //     warnf("tried to cancel gpio tthread 1 not running");
    // }

    // take out of microstepping mode
    //stepper_mode(STEP_MODE_1_1);

    // disable motor
    //stepper_enable(false);

    // terminate gpio
    gpioTerminate();

    // flush all remaining print statements
    fflush(stdout);
}



// static void _signandler_2(int sig) {
//     if (sig == 2) // which it should be
//         exit_gpio();
// }




static __unused void *_step_thread(void *args) {
    (void)args;
    int err;

    // ensure that this thread is cancelable
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    for(;;) {
        for(uint64_t i = 0; !global.dostep; i++) {
            //sched_yield();
            // when active then busy, when idle then block
            if (i < 1000)
                micros_busy_for(1);
            else
                micros_block_for(10000);
                
            pthread_testcancel();
        }

        // turn on step, stepping motor
        err = gpioWrite(GPIO_STEP, 1);
        wassert(("failed write to GPIO_STEP", !err));

        //micros_block_for(5); // minimum of 1us
        micros_busy_for(MIN_MICROS_PER_STEP>>(5-global.mode_mult));
        
        // turn off step, in prep for next turn-on
        err = gpioWrite(GPIO_STEP, 0);
        wassert(("failed write to GPIO_STEP", !err));
        
        micros_busy_for(MIN_MICROS_PER_STEP>>(5-global.mode_mult));

        // reset dostep
        global.dostep = 0;

        //sched_yield();
    }

    (void)err;

    return NULL;
}





static __unused void *_multistep_thread(void *args) {
    (void)args;
    int err;

    // ensure that this thread is cancelable
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    for(;;) {
        // wait until there are steps to step
        for(uint64_t i = 0; global.multistep == 0; i++) {
            sched_yield();
            // if (i < 1000)
            //     micros_busy_for(1);
            // else
            //     micros_block_for(100);
                
            pthread_testcancel();
        }

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
    while (_gpio_enabled && ((global.multistep != 0) || global.dostep))
        //sched_yield();
        micros_busy_for(1);
}





int stepper_enable(bool enable) {
    int err = gpioWrite(GPIO_NENABLE, !enable);
    assert(("failed write to GPIO_NENABLE", !err), err);
    return 0;
}



// don't call this while the stepper is running
int stepper_mode(step_mode_t mode) {
    int err;
    #ifdef __DEBUG__
    int nsteps, msteps;
    #endif

    DEBUG(
    assert(("step mode must be within the range 0 to 4 or 7", 
        (mode >= 0 && mode <= 4) || mode == 7), -1);
    )

    err  = _stepper_align(mode);
    assert(!err, err);

    err = 0;
    err |= gpioWrite(GPIO_MS1, !!(mode & MASK_MS1));
    err |= gpioWrite(GPIO_MS2, !!(mode & MASK_MS2));
    err |= gpioWrite(GPIO_MS3, !!(mode & MASK_MS3));
    assert(("failed write to GPIO_MSx", !err), err);

    global.mode = mode;

    // calculate mode multiplier
    //global.mode_mult = (4 - ((mode == 0b111) ? 0b100 : mode));
    global.mode_mult = MODE_TO_MULTPOW(mode);

    DEBUG(
    // assert that no steps are lost when changing modes
    msteps = stepper_getmsteps();
    nsteps = (msteps >> global.mode_mult) << global.mode_mult;
    assert(msteps == nsteps, -1);
    )
    
    msgf("mode set to %d with mult %d", mode, 1<<global.mode_mult);

    return 0;
}




// snaps motor to alignment for changing stepper mode without loosing steps
// uses the currently set direction.
// snaps based on 16 mode microstepping
static int _stepper_align(step_mode_t newmode) {
    int msteps, nsteps, msnap_pow, err;

    // if the new mode is larger then we can just return early
    if (newmode > global.mode)
        return 0;

    msnap_pow = MODE_TO_MULTPOW(newmode);

    // step until aligned
    // retains direction
    for (;;) {
        msteps = stepper_getmsteps();
        nsteps = (msteps >> msnap_pow) << msnap_pow;
        if (msteps == nsteps)
            break;

        err = stepper_step(global.dir);
        if (err) return -1;
    }

    return 0;
}



static int _stepper_setdir(step_dir_t dir) {
    int err;

    DEBUG(
    assert(("step direction must be either 0 or 1", (dir == 0 || dir == 1)), -1);
    );

    if (global.dir != dir) {
        msgf("changing direction to %d", dir);
        err = gpioWrite(GPIO_DIR, dir);
        assert(("failed write to GPIO_DIR", !err), err);
        global.dir = dir;
    }

    return 0;
}




static void _step_inc(void) {
    pthread_mutex_lock(&global.step_mutex);

    int32_t step  = global.step;
    int8_t fstep = global.fstep;
    
    fstep += 1<<global.mode_mult;
    if (fstep >= 16) {
        step++;
        fstep -= 16;
    }
    // let the programmer mod to get this
    // that way they can also get total steps
    // if (step >= 200) {
    //     step = 0;
    // }

    global.step = step;
    global.fstep = fstep;

    pthread_mutex_unlock(&global.step_mutex);
}




static void _step_dec(void) {
    pthread_mutex_lock(&global.step_mutex);

    int32_t step  = global.step;
    int8_t fstep = global.fstep;
    
    fstep -= 1<<global.mode_mult;
    if (fstep < 0) {
        step--;
        fstep += 16;
    }

    global.step = step;
    global.fstep = fstep;

    pthread_mutex_unlock(&global.step_mutex);
}



// this function is blocking if called too frequently
int stepper_step_noblock(step_dir_t dir) {
    int err;

    // if current step is active, then block until over
    // the reason is because steps will be lost if we try to 
    // write a 1 to a value that is already set to 1
    while (global.dostep) {
        // if gpio not enabled, then return
        if (!_gpio_enabled)
            return -1;
    
        //sched_yield();
        micros_busy_for(1);
    }

    // activate stepper
    // the stepper thread will set this to zero when finished
    global.dostep = 1;

    // set direction
    err = _stepper_setdir(dir);
    assert(!err, err);

    // increment step and fstep counter
    if (dir == STEP_DIR_CLOCKWISE)
        _step_inc();
    else
        _step_dec();

    return 0;
}



int stepper_step(step_dir_t dir) {
    int err;

    // set direction
    err = _stepper_setdir(dir);
    assert(!err, err);

    // step
    
    // turn on step, stepping motor
    err = gpioWrite(GPIO_STEP, 1);
    assert(("failed write to GPIO_STEP", !err), -1);

    //micros_block_for(5); // minimum of 1us
    micros_busy_for(MIN_MICROS_PER_STEP>>(5-global.mode_mult));
    
    // turn off step, in prep for next turn-on
    err = gpioWrite(GPIO_STEP, 0);
    assert(("failed write to GPIO_STEP", !err), -1);
    
    micros_busy_for(MIN_MICROS_PER_STEP>>(5-global.mode_mult));


    // increment step and fstep counter
    if (dir == STEP_DIR_CLOCKWISE)
        _step_inc();
    else
        _step_dec();

    return 0;
}




int stepper_multistep(step_dir_t dir, int32_t steps) {
    int err;

    // block if multistepper is currently running
    while (global.multistep != 0)
        //sched_yield();
        micros_busy_for(1);

    // set direction
    err = _stepper_setdir(dir);
    assert(!err, err);

    global.multistep = steps;

    return 0;
}



#ifdef __DEBUG__
void stepper_test(void) {
        int err;
        //long long int lastus;
        //long long int totalus;
        long long startus;
        long long endus;

        // init gpio
        init_gpio();

        err = stepper_mode(STEP_MODE_1_16);
        jassert(!err, _exit_gpio);

        //lastus = micros();
        //totalus = 0;

        startus = micros();
        for(int i = 0; i < 400; i++) {
            //long long int newus;
            
            err = stepper_step(STEP_DIR_CLOCKWISE);
            jassert(!err, _exit_gpio);
            
            //newus = micros();
            //debugf("%lld %lld", newus - lastus, micros());
            //totalus += newus - lastus;
            //lastus = newus;
        }
        endus = micros();
        
        // debugf("step average delta %lld us (target %lld us)",
        //         totalu
        debugf("step average delta %lld us (target %lld us)",
                (long long)(endus-startus)/400, 
                (long long)MIN_MICROS_PER_STEP>>(4-global.mode_mult));

        err = stepper_mode(STEP_MODE_1_16);
        jassert(!err, _exit_gpio);

        debugf("stepping to 0 degrees");
        stepper_stepto(0*16);
        micros_block_for(1e6);
        
        debugf("stepping to 90 degrees");
        stepper_stepto(50*16);
        micros_block_for(1e6);
        
        debugf("stepping to 180 degrees");
        stepper_stepto(100*16);
        micros_block_for(1e6);

        debugf("stepping to 360 degrees");
        stepper_stepto(200*16);
        micros_block_for(1e6);

        debugf("stepping to 0 degrees");
        stepper_stepto(0*16);
        micros_block_for(1e6);

    _exit_gpio:
        exit_gpio();
}
#endif


//                  signed  unsigned
// returns 0b 0000 SSSS SSSS FFFF
// or 0x0SSF
int32_t stepper_getsteps(void) {
    int32_t retval;
    pthread_mutex_lock(&global.step_mutex);
    retval = (int32_t)(global.step<<4) | (int32_t)global.fstep;
    pthread_mutex_unlock(&global.step_mutex);
    return retval;
}


// returns steps in terms of microsteps
int32_t stepper_getmsteps(void) {
    int32_t retval;
    pthread_mutex_lock(&global.step_mutex);
    retval = (int32_t)(global.step<<4) + (int32_t)global.fstep;
    pthread_mutex_unlock(&global.step_mutex);
    return retval;
}



float stepper_getangle(void) {
    int32_t msteps;
    msteps = stepper_getmsteps();
    return msteps * 360.0 / (((float)STEPS_PER_REV) * (1 << 4));
}



void stepper_setorigin(void) {
    pthread_mutex_lock(&global.step_mutex);
    global.step = 0;
    pthread_mutex_unlock(&global.step_mutex);
}





// full revolution is from 0-200 or 0-3200 depending on mode
// blocks until it reaches that angle
int stepper_steptomod(int32_t angle, step_dir_t dir) {
    int err;
    while (mod(angle - (stepper_getmsteps()>>global.mode_mult), 200) != 0) {
        err = stepper_step(dir);
        assert(!err, err);
    }
    stepper_wait();
    return 0;
}




int stepper_stepto(int32_t angle) {
    int err;
    global.stepto_run = true;
    step_dir_t dir = (angle > (stepper_getmsteps()>>global.mode_mult)) ? 
                        STEP_DIR_CLOCKWISE : STEP_DIR_COUNTERCLOCK;
    while (global.stepto_run && angle != (stepper_getmsteps()>>global.mode_mult)) {
        err = stepper_step(dir);
        assert(!err, err);
    }
    stepper_wait();
    return 0;
}





// snaps to nearest step (round)
__weak_inline int32_t angle_to_step(float32_t angle) {
    // steps per revolution
    float spr = (float)STEPS_PER_REV * (1 << (4 - global.mode_mult));
    return (int32_t)(angle * spr / 360.0 + 0.5);
}


__weak_inline float32_t step_to_angle(int32_t step) {
    // steps per revolution
    float spr = (float)STEPS_PER_REV * (1 << (4 - global.mode_mult));
    return step * 360.0 / spr;
}




//static int stepto_thread;

// the handle this returns is zero when active
// but nonzero if finished (will be the thread id)
// the handler this returns must be freed
stephandler_t *stepper_stepto_noblock(int32_t angle) {
    // first item is 
    int err;
    pthread_t tid;
    stephandler_t *handle = malloc(sizeof(stephandler_t));

    handle->is_active = true;
    handle->angle = angle;
    //handle->err = 0;
    
    err = pthread_create(&tid, NULL, (void*)&_stepper_stepto_thread, handle);
    assert(!err, NULL);

    handle->tid = tid;
    
    return handle;
}


static void *_stepper_stepto_thread(stephandler_t *handle) {
    int err;
    err = stepper_stepto((int32_t)handle->angle);
    handle->is_active = false;
    return (void*)(intptr_t)err;
}


bool stepper_is_stepping_to(stephandler_t *handle) {
    //void *err;

    if (handle->is_active) {
        // handle->err = 0;
        return true;

    } else {
        // pthread_join(handle->tid, &err);
        // handle->err = (int)(intptr_t)err;
        return false;
    }
    
}




// will cancel stepto if running
int stepper_stepto_free(stephandler_t *handle) {
    void *err;
    
    global.stepto_run = false;
    pthread_join(handle->tid, &err);
    
    DEBUG(*handle = (stephandler_t){0};)
    free(handle);

    return (int)(intptr_t)err;
}
