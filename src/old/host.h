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








typedef struct {
    float32_t angle;
    uint32_t band_hz;
    uint64_t freq_hz;
    int32_t bincount;    // number of bins
    fbin_t bins[0];
} databin_t;




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
    uint16_t samps;
    uint8_t  amp_enable;

    // backend only
    // int _slen; // number of sbins
    // int _blen; // number of bins per sbin
    // //sbins_t *_sbins;
    // sbin_t **_sbins;
    fbins_t *_fbins;
    volatile int _i;
    hackrf_device_t *_device;
} hparams_t;







///////////////
// bins.c
int fbins_init(fbins_t *fbins, int flen, int blen);
fbins_t *fbins_new(int flen, int blen);
void fbins_free(fbins_t *fbins);
int fbins_segment(fbins_t *fbins_in, fbins_t *fbins_out, int bins, int overlap);






///////////////
// hackrf.c
void hparams_defaults(hparams_t *params);
int hparams_init(hparams_t *params);
void hparams_free(hparams_t *params);

uint32_t hackrf_real_bandwidth(uint32_t band_hz);
int hackrf_read(hparams_t *params, fbins_t *fbins);
int hackrf_stop(hparams_t *params);
int hackrf_is_finished(hparams_t *params);
void hackrf_wait_until_finished(hparams_t *params);




///////////////
// fft.c
int fbins_fft(fbins_t *fbins_in, fbins_t *fbins_out);
int fbins_average(fbins_t *fbins_in, fbins_t *fbins_out);
int fbins_log(fbins_t *fbins_in, fbins_t *fbins_out);



///////////////
// plot.c
int plot_fbins(fbins_t *fbins, float xstart, float xend, int xlen, bool do_average);



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



int net_connect(net_t *net, const char *ip, uint16_t port, int timeout_ms);
int net_start(net_t *net, uint16_t port, int backlog);
int net_accept(const net_t *restrict netin, net_t *restrict netout, int timeout_ms)
int net_await(const net_t *net, net_mode_t mode, int timeout_ms);
bool net_is_open(const net_t *net);
int net_close(net_t *net, int timeout_sec);
int net_write(size_num_t count; const net_t *restrict net, const int8_t buff[restrict count], size_num_t count, int timeout_ms);
int net_write_raw(size_t count; const net_t *restrict net, const int8_t buff[restrict count], size_t count, int timeout_ms);
size_num_t net_readsize(const net_t *net, int timeout_ms);
ssize_t net_read(size_num_t count; const net_t *restrict net, int8_t buff[restrict count], size_num_t count, int timeout_ms);
ssize_t net_read_raw(size_t count; const net_t *restrict net, int8_t buff[restrict count], size_t count, int timeout_ms, int flags);
void net_tests(void);


#endif
