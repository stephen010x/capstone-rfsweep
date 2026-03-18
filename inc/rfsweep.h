#ifndef RFSWEEP_HOST_H
#define RFSWEEP_HOST_H

#if defined(__arm__) || defined(__aarch64__)
#define _RASPI
#endif


#include <stdint.h>
#include <stdbool.h>




#define STEP_MODE_1_1   0b000
#define STEP_MODE_1_2   0b001
#define STEP_MODE_1_4   0b010
#define STEP_MODE_1_8   0b011
#define STEP_MODE_1_16  0b111

#define STEP_DIR_COUNTERCLOCK   0
#define STEP_DIR_CLOCKWISE      1






#define DEFAULT_IP     "10.42.0.1"
#define DEFAULT_PORT   12345
#define DEFAULT_LOG    NULL
#define DEFAULT_SAMPS  10
#define DEFAULT_SNAP   16
#define DEFAULT_STEPS  360

#define DEFAULT_FREQ   2.4e6
#define DEFAULT_BAND   10e6
#define DEFAULT_SRATE  10e6
#define DEFAULT_LNA    16
#define DEFAULT_VGA    20

#define DEFAULT_RSERIAL  NULL
#define DEFAULT_TSERIAL  NULL





//#define SERVER_SDR_SERIAL NULL
//#define CLIENT_SDR_SERIAL NULL




typedef int step_dir_t;
typedef int step_mode_t;



// typedef struct {
//     //uint8_t real;
//     //uint8_t imag;
//     float real;
//     float imag;
// } fbin_t;


// typedef struct {
//     int8_t real;
//     int8_t imag;
// } fbin8_t;


typedef struct {
    int8_t real;
    int8_t imag;
} fbin_t;


// typedef struct sbins_t {
//     int len;
//     sbin_t *bins;
// } sbins_t;


// // this is for internal use
// // this is where the actual data is stored
// typedef struct {
//     int refcount;
//     int len;
//     fbin_t *data;
// } _bins_data_t;


// // this contains a lot of pointers to the data stored by _bins_data_t
// // allows shallow slice copies
// typedef struct {
//     int flen; // number of fbins
//     int blen; // number of bins per fbin
//     fbin_t **bins;
//     _bins_data_t *ref;
// } fbins_t;




typedef struct __packed {
    float32_t angle;        // 0
    uint32_t  band_hz;      // 4
    uint64_t  freq_hz;      // 8
    float64_t srate_hz;     // 16
    int64_t   timestamp_us; // 24   // added because these can be sent out of order
                            // 32
    int32_t   bcount;
    fbin_t    bins[0];      // 36
} fbins_t;


// for server.c compatability
typedef fbins_t databin_t;




// typedef struct {
//     int count;
//     fbins_t **fbins;
// } fbins_array_t;





#ifndef MY_HACKRF_SOURCE
typedef void hackrf_device_t;
#else
//#include "libhackrf/hackrf.h"
typedef hackrf_device hackrf_device_t;
#endif



// TODO: should rename to hboard_t or hdevice_t
typedef struct {
    double   srate_hz;
    uint64_t freq_hz;
    uint32_t band_hz;
    uint32_t lna_gain;  // steps of 8 dB, 0-40 dB
    uint32_t vga_gain;  // steps of 2 dB, 0-62 dB
    //volatile uint16_t samps;

    int32_t  samps;     // each sample is a collection of bins
    //int32_t  bins;      // how many bins per sample

    uint8_t  amp_enable;

    const char* serial; // serial number of board to open. If NULL then it just
                        // picks the first board it sees (I think)

    float32_t angle;    // for use by server.c


    // backend only
    // int _slen; // number of sbins
    // int _blen; // number of bins per sbin
    // //sbins_t *_sbins;
    // sbin_t **_sbins;
    //fbins_array_t *_fbinsa;
    //volatile int _ibin;
    volatile int _isamp;
    //fbins_t *_fbins;
    hackrf_device_t *_device;
} hparams_t;





#define NET_READ  POLLIN
#define NET_WRITE POLLOUT

//struct _net_struct;

typedef int net_mode_t;
// TODO: these two should be combined into a struct
typedef uint32_t magic_num_t;
typedef int32_t size_num_t;
typedef struct _net_struct net_t;

extern const magic_num_t magic_num;
extern const char *const LOOPBACK;
extern const char *const LOCALHOST;







// all of these must be explicit sizes to ensure proper serialization
// so no int. Instead, int32_t
// also explicit packing is a good idea as well
// all client commands requires the server to send a response
enum {
    MESSAGE_NULL = 0,
    MESSAGE_ERROR,      // Ideally this would return an error code, but I am lazy
    //MESSAGE_RECEIVED,
    //MESSAGE_FAILURE,
    MESSAGE_SUCCESS,
    MESSAGE_POLL,       // server lets client know it is busy to avoid timeout
    //MESSAGE_COMMAND,
    MESSAGE_DATA,
    MESSAGE_RESET,      // reset program
    MESSAGE_RESTART,    // reset device
    MESSAGE_GETLOGS,    // pulls debug and error messages of program
    MESSAGE_MEASURE,    // run measurements
    MESSAGE_TRANSMIT_ENABLE,
    MESSAGE_TRANSMIT_DISABLE,
    MESSAGE_UNSUPPORTED,// message is not supported by server (likely a response only message)
    MESSAGE_END,        // signal to client that measurements are done

    // TODO: implement this message
    //       Also implement a ctrl-c capture that will close the connection before killing
    MESSAGE_CANCEL,     // client tell server to stop

    MESSAGE_TYPE_LEN,   // this or anything past this is invalid
};
typedef int8_t message_type_t;








typedef struct __packed {
    int16_t type;
    
    union {
    
        struct {
            // receiver
            //uint32_t _p0;       // padding
            uint32_t lna_gain;  // steps of 8 dB, 0-40 dB
            uint32_t vga_gain;  // steps of 2 dB, 0-62 dB
            
            float64_t srate_hz;
            uint64_t freq_hz;
            
            uint32_t band_hz;
            int32_t  samps;     // samples per angle
            //int32_t  bins;      // how many bins per sample
            
            int16_t  steps;     // how many total steps in a full revolution
            
            uint8_t  amp_enable;
            uint8_t  snappow;   // round micro-step size to this pow2 exponent.

            // transmitter
            // TODO: add transmitter params here
            
        } measure;


        struct {
            uint64_t freq_hz;
            uint32_t band_hz;
            uint8_t  amp_enable;
        } transmit_enable;

        
        struct {
            int32_t size;
            union {
                int8_t data[0];
                int8_t *dataptr;    // NOTE: I should have done this for sending messages
                                    // Actually, next time, it should only be a poiner
                                    // and reads and writes write to and from that pointer
                                    // while the actual message data is simply packed along with it
            }
        } data;

    }
    
} message_t;






extern const char *str_help;
extern const char *str_help_server;
extern const char *str_help_misc;
extern const char *str_help_transmit;
extern const char *str_help_measure;



// extern const int16_t _msg_received;
// extern const int16_t _msg_failure;
// extern const int16_t _msg_success;
// extern const int16_t _msg_unsupported;
// extern const int16_t _msg_end;




typedef struct {
    int mode;

    union {

        // general
        const char *logpath;
        const char *fpath;
        const char *ip;
        uint16_t  port;
        bool      out_binary;

        // control
        const char *rserial;
        const char *tserial;
        int32_t   samps;
        uint8_t   snappow;
        bool      transmit_enable;
        int16_t   steps;
        float32_t angle;
        bool      is_verbose;

        // hackrf
        uint64_t  freq_hz;
        uint32_t  band_hz;
        uint8_t   amp_enable;
        float64_t srate_hz;
        uint32_t  lna_gain;
        uint32_t  vga_gain;
    };
} globalstate_t;








// #######################################################################
// #######################################################################
//      FUNCTIONS




///////////////
// bins.c
//int fbins_init(fbins_t *fbins, int flen, int blen);
//fbins_t *fbins_new(int flen, int blen);
//void fbins_free(fbins_t *fbins);
//int fbins_segment(fbins_t *fbins_in, fbins_t *fbins_out, int bins, int overlap);






///////////////
// hackrf.c
fbins_t *fbins_new(int32_t bcount);
size_t fbins_sizeof(fbins_t *fbins);

void hparams_defaults(hparams_t *params);
int hparams_init(hparams_t *params);
void hparams_free(hparams_t *params);

uint32_t hackrf_real_bandwidth(uint32_t band_hz);
// allocates new fbins_t
int hackrf_read(hparams_t *params);
int hackrf_stop(hparams_t *params);
int hackrf_is_finished(hparams_t *params);
void hackrf_wait_until_finished(hparams_t *params);




///////////////
// fft.c
// int fbins_fft(fbins_t *fbins_in, fbins_t *fbins_out);
// int fbins_average(fbins_t *fbins_in, fbins_t *fbins_out);
// int fbins_log(fbins_t *fbins_in, fbins_t *fbins_out);



///////////////
// plot.c
//int plot_fbins(fbins_t *fbins, float xstart, float xend, int xlen, bool do_average);



///////////////
// time.c
int64_t micros(void);
void micros_block_for(int64_t u);
void micros_busy_for(int64_t u);
void micros_test(void);




///////////////
// gpio.c
bool is_stepping(void);
void stepper_wait(void);
int stepper_enable(bool enable);
int stepper_mode(step_mode_t mode);
int stepper_step(step_dir_t dir);
//int stepper_multistep(step_dir_t dir, int steps, float steps_per_sec);
int stepper_multistep(step_dir_t dir, int32_t steps);
//int stepper_set_angle(int dir, float angle);
//int stepper_set_origin(void);
//int32_t stepper_get_offset(void);
void stepper_test(void);
int32_t stepper_getsteps(void);
void stepper_setorigin(void);
uint8_t stepper_get_multpow(void);
//int16_t stepper_get_steps_per_rev(void);
int stepper_stepto(int32_t angle, step_dir_t dir);






///////////////
// net.c
int net_await(const net_t *net, net_mode_t mode, int timeout_ms);
int net_close(net_t *net, int timeout_sec);
bool net_is_open(const net_t *net);
// server
int net_start(net_t *net, uint16_t port, int backlog);
int net_accept(const net_t *restrict netin, net_t *restrict netout, int timeout_ms)
// client
int net_connect(net_t *net, const char *ip, uint16_t port, int timeout_ms);
// read/write
int net_write(size_num_t count; const net_t *restrict net, const int8_t buff[restrict count], size_num_t count, int timeout_ms);
int net_write_raw(size_t count; const net_t *restrict net, const int8_t buff[restrict count], size_t count, int timeout_ms);
size_num_t net_readsize(const net_t *net, int timeout_ms);
ssize_t net_read(size_num_t count; const net_t *restrict net, int8_t buff[restrict count], size_num_t count, int timeout_ms);
ssize_t net_read_raw(size_t count; const net_t *restrict net, int8_t buff[restrict count], size_t count, int timeout_ms, int flags);
void net_tests(void);



///////////////
// server.c
ssize_t message_type_getsize(message_type_t type, int32_t data_bytes);
ssize_t message_getsize(const message_t *msg);
message_t *message_new(message_type_t type, int32_t data_bytes);
message_t *message_read(const net_t *net, int timeout_ms);
int message_write(const net_t *restrict net, const message_t *restrict msg, int timeout_ms);

int binqueue_push(databin_t *bin);
databin_t *binqueue_pop(void);
int binqueue_get_items(void);

//int server_run(uint16_t port, const char *logpath);
int server_run(globalstate_t *state);






///////////////
// strto.c
float64_t strtof64(const char *str);
float32_t strtof32(const char *str);
uint64_t  strtou64(const char *str);
uint32_t  strtou32(const char *str);
uint16_t  strtou16(const char *str);
uint8_t    strtou8(const char *str);
int64_t   strtoi64(const char *str);
int32_t   strtoi32(const char *str);
int16_t   strtoi16(const char *str);
int8_t     strtoi8(const char *str);






///////////////
// client.c
int client_request_reset(globalstate_t *state);
int client_request_restart(globalstate_t *state);
int client_request_getlogs(globalstate_t *state);
int client_request_measure(globalstate_t *state);




#endif
