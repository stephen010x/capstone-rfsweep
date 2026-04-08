
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <sched.h>
//#include <sched.h>


#include "toolkit/debug.h"
#include "rfsweep.h"



#define GOLDEN_RATIO ((float)1.61803)

// multiplier for reallocs
#define QUEUE_MULT   GOLDEN_RATIO
// starting queue alloc size
#define QUEUE_START  16

// #define MEASURE_STEP_MODE  STEP_MODE_1_16
// #define STEPS_PER_REV      (200*16)
#define MEASURE_STEP_MODE  STEP_MODE_1_4
#define STEPS_PER_REV      (200*4)

//#define STEPS_PER_REV (200*16)



//#define SERVER_TIMEOUT 10000    /* ten seconds */
#define SERVER_TIMEOUT 600000   /*10 minutes*/




// enum {
//     COMMAND_NULL = 0,
//     COMMAND_RESET,      // reset program
//     COMMAND_RESTART,    // reset device
//     COMMAND_GETLOGS,    // pulls debug and error messages of program
//     COMMAND_MEASURE,    // run measurements
// };
// typedef int8_t command_type_t;

/*
rfsweep send reset
rfsweep send measure --
*/


// typedef struct __packed {
//     int8_t type;
//     
//     union {
//     
//         struct {
//             int8_t command;
// 
//             union {
//                 struct {
//                     int16_t delta;      // resolution (how many total steps)
//                     uint32_t lna_gain;  // steps of 8 dB, 0-40 dB
//                     uint32_t vga_gain;  // steps of 2 dB, 0-62 dB
//                     double   srate_hz;
//                     uint64_t freq_hz;
//                     uint32_t band_hz;
//                     uint32_t samps;     // samples per step
//                     uint8_t  amp_enable;
//                     uint8_t  stepmod;   // mod micro-step size to this. 16 is full step
//                 } measure;
// 
//             };
//             
//         } command;
// 
//         
//         struct {
//             int32_t size;
//             int8_t data[0];
//         } data;
// 
//     }
//     
// } message_t;







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





// static int binqueue_init(void);
// static void binqueue_free(void);

//static databin_t *binqueue_pop(void);
//__inline__ int binqueue_items(void);
//static void binqueue_lock(void);
//static void binqueue_unlock(void);

static int _data_thread_start(const net_t *client);
static int _data_thread_end(bool force, bool end_with_error);
static void *_data_thread(const net_t *client);
static bool _data_thread_is_running(void);

// static int _server_message_handler(const net_t *restrict client, const message_t *restrict msg, const char *restrict logpath);
static int _server_message_handler(const net_t *restrict client, const message_t *restrict msg);
static int _server_measure_start(const net_t *restrict client, const message_t *restrict msg, hparams_t *restrict params, bool shall_rotate);
static int _server_measure(const message_t *restrict msg, hparams_t *restrict params, bool shall_rotate);
// static int _server_send_logs(const net_t *restrict client, const char *restrict logpath);
static int _server_send_logs(const net_t *client);

static void _init_server(void);
static void _exit_server(void);

static int _server_transmit(const net_t *restrict client, const message_t *restrict msg, bool transmit_enable);
static int _server_rotate(const net_t *restrict client, const message_t *restrict msg);


//void time_init(void);
void init_gpio(void);
void init_libhackrf(void);

void exit_gpio(void);
void exit_libhackrf(void);





// thread safe
// for sending databins
// will free databins when it sends them
static volatile struct {
    int        index;
    int        size;    // in terms of entries, not bytes
    databin_t **bins;
    //bool       enable;
} _binqueue = {
    .index = 0,
    .size  = 0,
    .bins  = NULL,
    //.enable = false,
};


static volatile bool _run_server;
static volatile bool _run_data_thread;
static volatile bool _err_data_thread;

static pthread_t tthread[1];
static pthread_mutex_t _binqueue_mutex = PTHREAD_MUTEX_INITIALIZER;

//static const char *_server_sdr_serial SERVER_SDR_SERIAL;

static globalstate_t *state;


//static const int16_t _msg_received    = MESSAGE_RECEIVED;
//static const int16_t _msg_failure     = MESSAGE_FAILURE;
static const __used int16_t _msg_error       = MESSAGE_ERROR;
static const __used int16_t _msg_success     = MESSAGE_SUCCESS;
static const __used int16_t _msg_unsupported = MESSAGE_UNSUPPORTED;
static const __used int16_t _msg_end         = MESSAGE_END;
static const __used int16_t _msg_poll        = MESSAGE_POLL;

//#define CONST_MSG_RECEIVED     (*(const message_t*)&_msg_received)
//#define CONST_MSG_FAILURE      (*(const message_t*)&_msg_failure)
#define CONST_MSG_ERROR        (*(const message_t*)&_msg_error)
#define CONST_MSG_SUCCESS      (*(const message_t*)&_msg_success)
#define CONST_MSG_UNSUPPORTED  (*(const message_t*)&_msg_unsupported)
#define CONST_MSG_END          (*(const message_t*)&_msg_end)
#define CONST_MSG_POLL         (*(const message_t*)&_msg_poll)




#define PSEUDOMSG (*(message_t*)NULL)




void stop_server(void) {
    _run_server = 0;
}



// returns size of message
// if negative, then an error has occured
ssize_t message_type_getsize(message_type_t type, int32_t data_bytes) {

    if ((type < 0) || (type >= MESSAGE_TYPE_LEN))
        error(("message is an invalid type", -1));

    switch (type) {
        case MESSAGE_DATA:
            return sizeof(PSEUDOMSG.type) + sizeof(PSEUDOMSG.data) + (size_t)data_bytes;
        
        case MESSAGE_MEASURE:
        case MESSAGE_RECEIVE:
            return sizeof(PSEUDOMSG.type) + sizeof(PSEUDOMSG.measure);

        case MESSAGE_ROTATE:
            return sizeof(PSEUDOMSG.type) + sizeof(PSEUDOMSG.rotate);
        
        case MESSAGE_TRANSMIT_ENABLE:
            return sizeof(PSEUDOMSG.type) + sizeof(PSEUDOMSG.transmit_enable);

        // all other message types are just the type
        // with no following data
        default:
            return sizeof(PSEUDOMSG.type);
    }
}





ssize_t message_getsize(const message_t *msg) {
    // to calm down the memory sanitizer
    #ifdef __DEBUG__
    return message_type_getsize(msg->type, 
        (msg->type == MESSAGE_DATA) ? msg->data.size : 0);
    #else
    return message_type_getsize(msg->type, msg->data.size);
    #endif
}





// return value must be freed by "free"
message_t *message_new(message_type_t type, int32_t data_bytes) {
    message_t *msg;
    ssize_t size;

    size = message_type_getsize(type, data_bytes);
    assert(size > 0, NULL);

    msg = malloc(size);

    if (msg != NULL) {
        msg->type = type;

        if (type == MESSAGE_DATA)
            msg->data.size = data_bytes;
    }

    return msg;
}




// allocates new message. Must be freed.
message_t *message_read(const net_t *net, int timeout_ms) {
    message_t *msg;
    ssize_t bytes, bytes2;

    msg = NULL;


    DEBUG(
    //printf("\n");
    debugf("reading message from <%s:%d>", net->ip, (int)net->port);
    )
    
    // wait for incoming message
    bytes = net_readsize(net, timeout_ms);
    //debugf("bytes %d", bytes);
    jassert(bytes > 0, _exit_flush);
    
    // allocate  message
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Walloc-size-larger-than="
    msg = malloc(bytes);
    jassert(msg != NULL, _exit_flush);
    #pragma GCC diagnostic pop

    // read incoming message
    bytes2 = net_read(net, (void*)msg, bytes, timeout_ms);
    //debugf("%d", (int)msg->type);
    jassert(("net_read error. Client likely disconnected.", bytes2 > 0), _exit_flush);
    //assert(bytes > 0, (free(msg), NULL));
    //assert(("message size vs bytes read mismatch", bytes2 == bytes), (free(msg), NULL));
    jassert(bytes > 0, _exit_flush);
    if (bytes2 != bytes) {
        alertf(STR_ERROR, "message size (%d) != bytes read (%d)", (int)bytes2, (int)bytes);
    }
    //jassert(("message size vs bytes read mismatch", bytes2 == bytes), _exit_flush);
    //assert(("message size misreported. message corrupt", message_getsize(msg) == bytes), (free(msg), NULL));
    if (message_getsize(msg) != bytes) {
        alertf(STR_ERROR, "message size misreported "
                          "(message_getsize->%ld) (bytes->%ld)",
                          (long)message_getsize(msg), (long)bytes);
        goto _exit_flush;
    }

    DEBUG(
    // debugf("address %s:%d received %s message (%d)", 
    //     net->ip, (int)net->port, message_type_str(msg->type), (int)msg->type);
    debugf("received %s(%d) (%d bytes) from <%s:%d>", 
        message_type_str(msg->type), (int)msg->type, 
        (int)message_getsize(msg), net->ip, (int)net->port);
    )

    return msg;

    _exit_flush:
    free(msg);
    // flush buffer, because I guess leftovers have been creating bugs
    net_flush(net);
    return NULL;
}






int message_write(const net_t *restrict net, const message_t *restrict msg, int timeout_ms) {
    int err;


    DEBUG(
    //printf("\n");
    debugf("sending %s(%d) (%d bytes) to <%s:%d>", 
        message_type_str(msg->type), (int)msg->type, 
        (int)message_getsize(msg), net->ip, (int)net->port);
    )

    err = net_write(net, (void*)msg, message_getsize(msg), timeout_ms);
    assert(!err, -1);

    return 0;
}







//static __construct void _init_server(void) {
static void _init_server(void) {
    //int err;
    // mutex should be locked before anything else.
    // that way everyone who tries to modify binqueue
    // is blocked until it is initilized
    //pthread_mutex_lock(&_binqueue_mutex);

    // err = binqueue_init();
    // fassert(!err);

    // run external constructs
    //time_init();
    init_gpio();
    init_libhackrf();
}




//static __destruct void _exit_server(void) {
static void _exit_server(void) {
    // run external deconstructs
    exit_gpio();
    exit_libhackrf();
    
    //binqueue_free();
}





// don't call this within binqueue_lock
// don't call this function more than once, or 
// really bad things will happen
// Nevermind.
// Just call this from main thread rather than
// the thread handling the queue
//static int binqueue_init(void) {
int binqueue_init(void) {
    void *mem;

    //pthread_mutex_unlock(&_binqueue_mutex);

    DEBUG(debugf("creating binqueue");)
    
    mem = malloc(QUEUE_START * sizeof(void*));
    assert(mem != NULL, -1);

    _binqueue = (typeof(_binqueue)){
        .index = 0,
        .size  = QUEUE_START,
        .bins  = mem,
    };

    // unlock mutex for everyone (nevermind)
    //pthread_mutex_unlock(&_binqueue_mutex);
    
    return 0;
}






// don't call this within binqueue_lock
//static void binqueue_free(void) {
// NOTE: IMPORTANT If debug mode is enabled, freeing memory will not 
//       release it to the OS, causing memory to fill up after enough calls
void binqueue_free(void) {
    void *bin;

    //pthread_mutex_lock(&_binqueue_mutex);

    DEBUG(
    debugf("freeing binqueue");
    )

    if (_binqueue.index != 0)
        warnf("freeing queue with %d entries left", _binqueue.index);

    // pop and free any remaining bins in queue
    for (int i = 0; _binqueue.index > 0; i++) {
        //debugf("popping item %d (%d left) from binqueue", i, _binqueue.index);
        bin = binqueue_pop();
        free(bin);
    }

    // free bin pointer queue
    free(_binqueue.bins);

    //pthread_mutex_unlock(&_binqueue_mutex);
}





// void binqueue_wait_until_empty(void) {
//     while 
//     sched_yield();
// }




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

    // if (!binqueue_enabled)
    //     error("binqueue not enabled", -1);

    pthread_mutex_lock(&_binqueue_mutex);

    size = _binqueue.size;

    // will segfault if bin is invalid memory.
    // better here than later.
    DEBUG(
    _sink = bin->bcount;
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
        mem = realloc(_binqueue.bins, size * sizeof(void*));
        // error check
        if (mem == NULL) pthread_mutex_unlock(&_binqueue_mutex);
        assert(mem != NULL, -1);
        

        _binqueue.size = size;
        _binqueue.bins = mem;
    }

    //debugf("binqueue (%p) size (%d) index (%d)",_binqueue.bins, (int)_binqueue.size, (int)_binqueue.index);

    _binqueue.bins[_binqueue.index] = bin;
    _binqueue.index++;

    pthread_mutex_unlock(&_binqueue_mutex);

    return 0;
}





// call this within binqueue_lock
// nevermind
databin_t *binqueue_pop(void) {
    void *mem;

    pthread_mutex_lock(&_binqueue_mutex);

    DEBUG(
    // segfault if no items
    if (_binqueue.index == 0) segfault();
    )

    _binqueue.index--;

    mem = _binqueue.bins[_binqueue.index];
    
    pthread_mutex_unlock(&_binqueue_mutex);
    
    return mem;
}


__weak_inline int binqueue_get_items(void) {
    int retval;

    pthread_mutex_lock(&_binqueue_mutex);
    retval = _binqueue.index;
    pthread_mutex_unlock(&_binqueue_mutex);

    return retval;
}


// // lock binqueue from threads
// static void binqueue_lock(void) {
//     pthread_mutex_lock(&_binqueue_mutex);
// }
// 
// // unlock binqueue from threads
// static void binqueue_unlock(void) {
//     pthread_mutex_unlock(&_binqueue_mutex);
// }







static int _data_thread_start(const net_t *client) {
    int err;

    DEBUG(debugf("creating data thread");)

    err = binqueue_init();
    fassert(!err);

    //err = binqueue_init();
    //assert(("failed to init binqueue", !err), -1);

    //_run_data_thread = true;

    _run_data_thread = true;
    _err_data_thread = false;
    
    err = pthread_create(&tthread[0], NULL, (void*)&_data_thread, (void*)client);
    if (err) {
        _run_data_thread = false;
        _err_data_thread = true;
        tthread[0] = 0;
    }
    
    assert(("failed to create data thread", !err), -1);
    
    return 0;
}



// will block until thread closes naturally, 
// or is already closed
// returns value returned by closed thread
// if force is false, then it will wait until data thread ends normally
static int _data_thread_end(bool force, bool end_with_error) {
    void *retval;
    int err;
    //fbins_t *fbins;

    DEBUG(debugf("waiting for data thread to exit");)


    if (tthread[0] != 0) {
        if (!force) {
            while (binqueue_get_items())
                micros_block_for(1000);
        }

        _run_data_thread = false;
            
        _err_data_thread = end_with_error;
    
        err = pthread_join(tthread[0], &retval);

        // flush binqueue
        // DEBUG(debugf("flushing binqueue");)
        // while(binqueue_get_items()) {
        //     fbins = binqueue_pop();
        //     free(fbins);
        // }

        binqueue_free();
        
        assert(("data thread failed to exit", !err), -1);
        assert(("data thread returned error", retval == 0), (int)(intptr_t)retval);
        DEBUG(debugf("data thread successfully exited");)

        DEBUG(tthread[0] = 0;)
        
    } else {
        warnf("tried to stop data thread, which was never started");
    }

    //binqueue_free();

    return (int)(intptr_t)retval;
}





// basically, this just waits until there is something in the queue, and then it sends it off.
static void *_data_thread(const net_t *client) {
    int err;
    fbins_t *fbins;
    message_t *msg;

    // _run_data_thread = true;
    // _err_data_thread = false;

    while (_run_data_thread) {

        // loop as long as there are items in the queue
        while(binqueue_get_items()) {

            //debugf("%d items in binqueue", (int)binqueue_get_items());

            // pop item from queue
            fbins = binqueue_pop();
            if (fbins == NULL) err = -1;
            jassert(fbins != NULL, _exit_loop);

            // create new message
            msg = message_new(MESSAGE_DATA, fbins_sizeof(fbins));

            // populate message and set size
            memcpy(msg->data.data, fbins, fbins_sizeof(fbins));
            //msg->data.size = fbins_sizeof(fbins);

            //debugf("fbins size %d", (int)fbins_sizeof(fbins));

            // send item from queue
            err = message_write(client, msg, SERVER_TIMEOUT);

            // free message and fbins
            free(msg);
            free(fbins);

            // handle error
            jassert(!err, _exit_thread);
        }

        // otherwise send poll and delay for 200 ms
        err = message_write(client, &CONST_MSG_POLL, SERVER_TIMEOUT);
        jassert(!err, _exit_thread);
        micros_block_for(200000);
        //otherwise yield
        //shed_yield();
    }

    _exit_loop:

    // send end or error message
    if (_err_data_thread)
        err = message_write(client, &CONST_MSG_ERROR, SERVER_TIMEOUT);
    else
        err = message_write(client, &CONST_MSG_END, SERVER_TIMEOUT);

    //assert(!err, (void*)(intptr_t)err);
    if (err) alertf(STR_ERROR, "failed to send message to client");

    _exit_thread:
    
    _run_data_thread = 0; // for others to check if this is still active

    return (void*)0;
}




static bool _data_thread_is_running(void) {
    return _run_data_thread;
}












// run server (main thread)
//static void *_server_thread(void *port) {
//int server_run(uint16_t port, const char *logpath) {
int server_run(globalstate_t *_state) {
    int err;
    net_t server, client;
    //ssize_t bytes;
    message_t *msg;


    state = _state;


    // run server init
    _init_server();
    

    // start listening for incoming connections
    //net_start(&server, (uint16_t)(uintptr_t)port, 1);
    net_start(&server, state->port, 1);

    _run_server = true;
    err = 0;
    
    while(_run_server) {

        // to avoid badness if we need to free it after an error
        msg = NULL;

        // wait for incoming connection, no timeout
        err = net_accept(&server, &client, -1);
        assert(!err, -1);

        // message loop until client closes connection
        //while(net_is_open(&client)) {

//         // wait for incoming message
//         bytes = net_readsize(&client, SERVER_TIMEOUT);
//         if (bytes < 1) goto close_client;
//         
//         // allocate buffer for message
//         msg = malloc(bytes);
// 
//         // read incoming message, timeout 1 second
//         bytes = net_read(&client, (void*)msg, bytes, SERVER_TIMEOUT);
//         if (bytes < 1) goto close_client;

        // wait for incoming message
        msg = message_read(&client, SERVER_TIMEOUT);
        if (msg == NULL) {
            _run_server = false;
            err = -1;
            jassert(msg != NULL, _exit_client);
        }

        // handle message
        _server_message_handler(&client, msg);
        
        //}

        // free msg buffer and close client
        close_client:
        (void)&&close_client;
        free(msg);

        _exit_client:
        net_close(&client, SERVER_TIMEOUT);

    }

    // close server listener
    net_close(&server, SERVER_TIMEOUT);

    // run server exit
    _exit_server();

    return err;
}







static int _server_message_handler(const net_t *restrict client, const message_t *restrict msg) {
    int err;
    hparams_t params;


    msgf("received \"%s\" message (%d)", message_type_str(msg->type), msg->type);
    

    switch (msg->type) {

        // reset program
        case MESSAGE_RESET:
            // err = message_write(client, &CONST_MSG_RECEIVED, SERVER_TIMEOUT);
            err = message_write(client, &CONST_MSG_SUCCESS, SERVER_TIMEOUT);
            _run_server = false;
            return err;


        // restart whole system
        case MESSAGE_RESTART:
            // err = message_write(client, &CONST_MSG_RECEIVED, SERVER_TIMEOUT);
            err = message_write(client, &CONST_MSG_SUCCESS, SERVER_TIMEOUT);
            err |= system("systemctl reboot");
            return err;


        case MESSAGE_PING:
            err = message_write(client, &CONST_MSG_SUCCESS, SERVER_TIMEOUT);
            return err;


        // return error logs
        case MESSAGE_GETLOGS:
            err = _server_send_logs(client);
            if (err) err |= message_write(client, &CONST_MSG_ERROR, SERVER_TIMEOUT);
            return err;


        case MESSAGE_MEASURE:
            err = _server_measure_start(client, msg, &params, true);
            //if (err) err |= message_write(client, &CONST_MSG_ERROR, SERVER_TIMEOUT);
            //else     err |= message_write(client, &CONST_MSG_END,   SERVER_TIMEOUT);
            return err;

        case MESSAGE_RECEIVE:
            //err = _server_receive_start(client, msg, &params);
            err = _server_measure_start(client, msg, &params, false);
            return err;
        
        case MESSAGE_ROTATE:
            err = _server_rotate(client, msg);
            return err;
        
        case MESSAGE_TRANSMIT_ENABLE:
            err = _server_transmit(client, msg, true);
            if (err) message_write(client, &CONST_MSG_ERROR,   SERVER_TIMEOUT);
            else     message_write(client, &CONST_MSG_SUCCESS, SERVER_TIMEOUT);
            return err;
            
        
        case MESSAGE_TRANSMIT_DISABLE:
            err = _server_transmit(client, msg, false);
            if (err) message_write(client, &CONST_MSG_ERROR,   SERVER_TIMEOUT);
            else     message_write(client, &CONST_MSG_SUCCESS, SERVER_TIMEOUT);
            return err;
            


        // client only or invalid messages
        default:
            message_write(client, &CONST_MSG_UNSUPPORTED, SERVER_TIMEOUT);
            warnf("received unsupported message \"%s\" (%d)", message_type_str(msg->type), msg->type);
            return -1;
    }
}






char* message_type_str(message_type_t type) {
    switch (type) {
        case MESSAGE_NULL:        return "MESSAGE_NULL";
        //case MESSAGE_RECEIVED:    return "MESSAGE_RECEIVED";
        case MESSAGE_SUCCESS:     return "MESSAGE_SUCCESS";
        case MESSAGE_DATA:        return "MESSAGE_DATA";
        case MESSAGE_RESET:       return "MESSAGE_RESET";
        case MESSAGE_RESTART:     return "MESSAGE_RESTART";
        case MESSAGE_GETLOGS:     return "MESSAGE_GETLOGS";
        case MESSAGE_MEASURE:     return "MESSAGE_MEASURE";
        case MESSAGE_UNSUPPORTED: return "MESSAGE_UNSUPPORTED";
        case MESSAGE_END:         return "MESSAGE_END";
        case MESSAGE_POLL:        return "MESSAGE_POLL";
        case MESSAGE_ERROR:       return "MESSAGE_ERROR";
        case MESSAGE_PING:        return "MESSAGE_PING";
        case MESSAGE_RECEIVE:     return "MESSAGE_RECEIVE";
        case MESSAGE_ROTATE:      return "MESSAGE_ROTATE";
        case MESSAGE_TRANSMIT_ENABLE: return "MESSAGE_TRANSMIT_ENABLE";
        case MESSAGE_TRANSMIT_DISABLE: return "MESSAGE_TRANSMIT_DISABLE";
        default:                  return "INVALID MESSAGE";
    }
}








static int _server_measure_start(const net_t *restrict client, const message_t *restrict msg, hparams_t *restrict params, bool shall_rotate) {
        int err;

        DEBUG(debugf("server writing poll message");)
        message_write(client, &CONST_MSG_POLL, SERVER_TIMEOUT);


        // start data thread
        err = _data_thread_start(client);
        assert(!err, err);

        // init hackrf
        err = hparams_init(params, state->rserial);
        if (err) goto end_thread;

        // TODO what I really should have done is 
        //      make it so that a step count of zero would 
        //      not perform any rotations
        if (shall_rotate) {
            // enable motor
            stepper_enable(true);
            // wait for stepper motor to align after turnon
            micros_block_for(1e6);
            // set origin
            //stepper_setorigin();
            // set step mode
            stepper_mode(MEASURE_STEP_MODE);
        }

        // start measurements
        err = _server_measure(msg, params, shall_rotate);
        wassert(("_server_measure failed", !err));

    end_motor:
        (void)&&end_motor;
        
        if (shall_rotate) {
            // quick pause
            micros_block_for(1e6);
            // step back to origin
            stepper_stepto(0);
            // take out of microstepping mode
            stepper_mode(STEP_MODE_1_1);
            // wait for motor to settle before turnoff
            micros_block_for(1e6);
            // disable motor
            stepper_enable(false);
        }

        // close hackrf
        hparams_free(params);

        // end data thread
    end_thread:
        msgf("Waiting for client to receive all messages...");
        err |= _data_thread_end(err, err);
        assert(!err, err);
        
        return err;
}








static int _server_measure(const message_t *restrict msg, hparams_t *restrict params, bool shall_rotate) {
    int err;
    int steps;
    
    // full rotation is from 0.0 to 1.0
    //float angle, delta;

    // set vars
    steps = (int)msg->measure.steps;
    // angle = 0.0;
    // delta = 1.0 / steps;

    // setup params
    params->srate_hz   = msg->measure.srate_hz;
    params->freq_hz    = msg->measure.freq_hz;
    params->band_hz    = msg->measure.band_hz;
    params->lna_gain   = msg->measure.lna_gain;
    params->vga_gain   = msg->measure.vga_gain;
    params->amp_enable = msg->measure.amp_enable;
    params->samps      = msg->measure.samps;
    //params->serial     = _server_sdr_serial;
    //params->serial     = state->rserial;

    // start data collection loop
    for (int i = 0; i < steps; i++) {
        int32_t angle;

        // check to see if data loop is still running
        // if not, then abort
        assert(_data_thread_is_running(), -1);

        msgf("collecting data [%d/%d]", i+1, steps);

        // calculate angle
        angle = (int32_t)roundf((float)STEPS_PER_REV * (float)i / (float)steps);
        // round to snap step size
        angle = (angle >> msg->measure.snappow) << msg->measure.snappow;
        // set param "pretty" angle
        params->angle = 360.0f * angle / (float)STEPS_PER_REV;

        // move to desired angle
        if (shall_rotate)
            stepper_stepto(angle);

        // take measurement
        err = hackrf_read(params);
        assert(!err, -1);

        // wait until measurement complete
        hackrf_wait_until_finished(params);
    }
    
    return 0;
}







static int _server_send_logs(const net_t *client) {
    FILE *file;
    long size;
    size_t rsize;
    int err;
    //int8_t *buff;
    message_t *msg;

    // open file
    file = fopen(state->logpath, "rb");
    assert(("problem opening file. path may be invalid", file != NULL), -1);

    // get file length
    // seek end of file
    err = fseek(file, 0, SEEK_END);
    if (err) {
        fclose(file);
        error(-2);
    }
    size = ftell(file);
    if (size < 0) {
        fclose(file);
        error(-3);
    }

    // allocate message
    //buff = malloc(size+1);  // plus one extra to inject null terminator
    msg = message_new(MESSAGE_DATA, size+1); // plus one extra to inject null terminator
    if (msg == NULL) {
        fclose(file);
        error(("failed to allocate space for message", -4));
    }

    // rewind file for actual reading
    rewind(file);

    // read file into string
    rsize = fread(msg->data.data, 1, size, file);
    //debugf("filesize %d", (int)rsize);
    
    // close file.
    fclose(file);

    // error check fread
    if (rsize != (size_t)size) {
        free(msg);
        error(-5);
    }

    // add null terminator and data size
    msg->data.data[size] = '\n';
    //msg->data.size = size+1;

    // send message
    err = message_write(client, msg, SERVER_TIMEOUT);

    // free buffer
    free(msg);

    // check message errors
    assert(!err, -6);
    
    return 0;
}





static int _server_rotate(const net_t *restrict client, const message_t *restrict msg) { 
    int err;
    stephandler_t *handle;

    (void)client;


    //if (msg->rotate.is_angle) {
    
    err = stepper_enable(true);
    if (err) goto _exit_rotate;
    
    err = stepper_mode(msg->rotate.stepmode);
    if (err) goto _exit_rotate;
    
    micros_block_for(10e3); // 10ms
    

    // begin nonblocking rotation
    if (msg->rotate.is_angle) {
        msgf("rotating from %.1f\xC2\xB0 to %.1f\xC2\xB0 degrees", 
                (double)step_to_angle(stepper_getmsteps()>>4), 
                (double)msg->rotate.angle);
        handle = stepper_stepto_noblock(angle_to_step(msg->rotate.angle));
    } else {
        msgf("rotating from step %d to step %d index in microstep %d",
                (int)stepper_getmsteps()>>(4-msg->rotate.stepmode),
                (int)msg->rotate.steps, 
                (int)1<<msg->rotate.stepmode);
        handle = stepper_stepto_noblock(msg->rotate.steps);
    }

    if (handle == NULL) {
        err = -1;
        alertf(STR_ERROR, "failed to create rotate handle");
        goto _exit_rotate;
    }

    // poll until finished
    while(stepper_is_stepping_to(handle)) {
        err = message_write(client, &CONST_MSG_POLL, SERVER_TIMEOUT);
        //assert(!err, err);
        if (err) {
            alertf(STR_ERROR, "failed to poll");
            goto _exit_handle;
        }
        micros_block_for(200000);
    }


    _exit_handle:
    
    //free(handle);
    err = stepper_stepto_free(handle);
    
    if (err) {
        alertf(STR_ERROR, "rotate handle returned error");
        goto _exit_rotate;
    }


    _exit_rotate:
    
    stepper_enable(false);
    micros_block_for(10e3); // 10ms
    //}
    

    if (err) message_write(client, &CONST_MSG_ERROR,   SERVER_TIMEOUT);
    else     message_write(client, &CONST_MSG_SUCCESS, SERVER_TIMEOUT);

    return err;
}







static int _server_transmit(const net_t *restrict client, const message_t *restrict msg, bool transmit_enable) {
    int err;
    static hparams_t params = {0};

    (void)client;


    // if not transmit_enable, then disable and return early
    if (!transmit_enable) {
        err = hackrf_stop(&params);
        assert(!err, -1);
        hparams_free(&params);
        return 0;
    }

    // init hackrf
    err = hparams_init(&params, state->tserial);
    if (err) return -1;

    // setup transmission params
    params = (hparams_t){
        .freq_hz     = msg->transmit_enable.freq_hz,
        .tx_vga_gain = msg->transmit_enable.vga_gain,
        .tx_amp      = msg->transmit_enable.tx_amp,
        .amp_enable  = msg->transmit_enable.amp_enable,
    };

    // begin transmission
    err = hackrf_write(&params);
    if (err) {
        hparams_free(&params);
        return -1;
    }

    return 0;
}


