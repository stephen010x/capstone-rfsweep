
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
//#include <sched.h>


#include "toolkit/debug.h"
#include "rfsweep/host.h"



#define GOLDEN_RATIO ((float)1.61803)

// multiplier for reallocs
#define QUEUE_MULT   GOLDEN_RATIO
// starting queue alloc size
#define QUEUE_START  16




// all of these must be explicit sizes to ensure proper serialization
// so no int. Instead, int32_t
// also explicit packing is a good idea as well
enum {
    MESSAGE_NULL = 0,
    MESSAGE_RECEIVED,
    MESSAGE_SUCCESS,
    MESSAGE_COMMAND,
    MESSAGE_DATA,
    MESSAGE_END = -1,   // signal to client that measurements are done
};
typedef int8_t message_type_t;


enum {
    COMMAND_NULL = 0,
    COMMAND_RESET,      // reset program
    COMMAND_RESTART,    // reset device
    COMMAND_GETLOGS,    // pulls debug and error messages of program
    COMMAND_MEASURE,    // run measurements
};
typedef int8_t command_type_t;

/*
rfsweep send reset
rfsweep send measure --
*/


typedef struct __packed {
    int8_t type;
    
    union {
    
        struct {
            int8_t command;

            union {
                struct {
                    int16_t delta;      // resolution (how many total steps)
                    uint32_t lna_gain;  // steps of 8 dB, 0-40 dB
                    uint32_t vga_gain;  // steps of 2 dB, 0-62 dB
                    double   srate_hz;
                    uint64_t freq_hz;
                    uint32_t band_hz;
                    uint32_t samps;     // samples per step
                    uint8_t  amp_enable;
                    uint8_t  stepmod;   // mod micro-step size to this. 16 is full step
                } measure;

            };
            
        } command;

        
        struct {
            int32_t size;
            int8_t data[0];
        } data;

    }
    
} message_t;





typedef struct {
    float32_t angle;
    uint32_t band_hz;
    uint64_t freq_hz;
    int32_t bincount;    // number of bins
    fbin8_t bins[0];
} databin_t;


// we don't need this, as we can just send one message
// at a time, forcing the client to keep tally of how many
// databins we are sending
// this way the bins all don't have to be the same size either
// though they probably all still will be.
// // all databins should be of equal size
// typedef struct {
//     int32_t binsize;    // bytes per bin
//     int32_t bincount;   // number of bins
//     int8_t data[0];
// } databin_array_t;




// thread safe
// for sending databins
// will free databins when it sends them
static volatile struct {
    int        index;
    int        size;    // in terms of entries, not bytes
    databin_t *bins;
    //bool       enable;
} _binqueue = {
    .index = 0,
    .size  = 0,
    .bins  = NULL,
    //.enable = false,
};






static pthread_t tthread[1];

static pthread_mutex_t _binqueue_mutex = PTHREAD_MUTEX_INITIALIZER;





static __construct void _init(void) {
    // mutex should be locked before anything else.
    // that way everyone who tries to modify binqueue
    // is blocked until it is initilized
    pthread_mutex_lock(&_binqueue_mutex);
}





// don't call this within binqueue_lock
// don't call this function more than once, or 
// really bad things will happen
static int binqueue_init(void) {
    void *mem;
    
    mem = malloc(QUEUE_START * sizeof(void*));
    assert(mem != NULL);

    _binqueue = (typeof(_binqueue)){
        .index = 0,
        .alloc = QUEUE_START,
        .bins  = mem,
    }

    // unlock mutex for everyone
    pthread_mutex_unlock(&_binqueue_mutex);
    
    return 0;
}






// don't call this within binqueue_lock
static void binqueue_free(void) {
    void *bin;

    pthread_mutex_lock(&_binqueue_mutex);

    if (_binqueue.index != 0)
        warnf("freeing queue with %d entries left", _binqueue.index);

    // pop and free any remaining bins in queue
    for (int i = 0; i < _binqueue.index; i++) {
        bin = binqueue_pop();
        free(bin);
    }

    // free bin pointer queue
    free(_binqueue.bins);

    pthread_mutex_unlock(&_binqueue_mutex);
}




DEBUG(
// I wonder if making this static, or declaring it in a function
// would still force it to behave the way I want it to
// (that is, immune to compiler optimizations)
volatile int _sink;
)


// don't call this within binqueue_lock
int binqueue_push(databin_t *bin) {
    void *mem;
    int size;

    pthread_mutex_lock(&_binqueue_mutex);

    size = _binqueue.size;

    // will segfault if bin is invalid memory.
    // better here than later.
    DEBUG(
    _sink = bin->bincount;
    )

    // if new index will exceed queue size,
    // reallocate to bigger queue
    if (_binqueue.index + 1 == size) {

        // if multiplier too small to change it, then increment
        if (size * (QUEUE_MULT-1) >= 1)
            size *= QUEUE_MULT;
        else
            size++;

        // reallocate bins to new size
        mem = realloc(_binqueue.bins, _binqueue.size);
        // error check
        if (mem == NULL) pthread_mutex_unlock(&_binqueue_mutex);
        assert(mem != NULL, -1);
        

        _binqueue.size = size;
        _binqueue.bins = mem;
    }

    _binqueue.bins[_binqueue.index] = bin;
    _binqueue.index++;

    pthread_mutex_unlock(&_binqueue_mutex);

    return 0;
}





// call this within binqueue_lock
static databin_t *binqueue_pop(void) {
    void *mem;

    //pthread_mutex_lock(&_binqueue_mutex);

    DEBUG(
    // segfault if no items
    if (_binqueue.index == 0) segfault();
    )

    _binqueue.index--;

    mem = _binqueue.bins[_binqueue.index]
    
    //pthread_mutex_unlock(&_binqueue_mutex);
    
    return mem;
}



// call this within binqueue_lock
__inline__ int binqueue_items(void) {
    int retval;

    //pthread_mutex_lock(&_binqueue_mutex);
    retval = _binqueue.index;
    //pthread_mutex_unlock(&_binqueue_mutex);

    return retval;
}



// lock binqueue from threads
static void binqueue_lock(void) {
    pthread_mutex_lock(&_binqueue_mutex);
}

// unlock binqueue from threads
static void binqueue_unlock(void) {
    pthread_mutex_unlock(&_binqueue_mutex);
}







// starts server thread
int server_start(void) {
    int err;

    debugf("creating server thread");

    err = binqueue_init();
    assert(("failed to init binqueue", !err), -1);
    
    err = pthread_create(&tthread[0], NULL, &_server_thread, NULL);
    assert(("failed to create server thread", !err), -1);
    
    return 0;
}



// will block until server closes naturally, 
// or is already closed
int server_end(void) {
    void *retval;
    int err;

    debugf("waiting for server thread to exit");

    // cancel threads
    if (tthread[0] != 0) {
        err = pthread_join(tthread[0], &retval);
        assert(("server thread failed to exit", !err), -1);
        assert(("server thread returned error", retval == 0), -2);
    } else {
        debugf("server thread successfully exited");
    }

    binqueue_free();

    return 0;
}




static void _server_thread(void*) {

}




int 
