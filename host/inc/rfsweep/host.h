#ifndef RFSWEEP_HOST_H
#define RFSWEEP_HOST_H

#if defined(__arm__)
#define _RASPI
#endif


#include <stdint.h>
#include <stdbool.h>




#define STEP_MODE_1_1   0b000
#define STEP_MODE_1_2   0b001
#define STEP_MODE_1_4   0b010
#define STEP_MODE_1_8   0b011
#define STEP_MODE_1_16  0b111

#define STEP_DIR_CLOCKWISE      0
#define STEP_DIR_COUNTERCLOCK   1

typedef int step_dir_t;
typedef int step_mode_t;



typedef struct {
    //uint8_t real;
    //uint8_t imag;
    float real;
    float imag;
} fbin_t;

// typedef struct sbins_t {
//     int len;
//     sbin_t *bins;
// } sbins_t;

typedef struct {
    int flen; // number of sbins
    int blen; // number of bins per sbin
    fbin_t **bins;
} fbins_t;




#ifndef MY_HACKRF_SOURCE
typedef void hackrf_device_t;
#else
#include "libhackrf/hackrf.h"
typedef hackrf_device hackrf_device_t;
#endif




// TODO: should rename to hboard_t or hdevice_t
typedef struct {
    double   srate_hz;
    uint64_t freq_hz;
    uint32_t band_hz;
    uint32_t lna_gain;
    uint32_t vga_gain;
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
// hackrf.c
int fbins_init(fbins_t *fbins, int flen, int blen);
fbins_t *fbins_new(int flen, int blen);
void fbins_free(fbins_t *fbins);

void hparams_defaults(hparams_t *params);
int hparams_init(hparams_t *params);
void hparams_free(hparams_t *params);


uint32_t hackrf_real_bandwidth(uint32_t band_hz);
int hackrf_read(hparams_t *params, fbins_t *fbins);
int hackrf_stop(hparams_t *params);
int hackrf_is_finished(hparams_t *params);
void hackrf_wait_until_finished(hparams_t *params);

// consider moving to source file
int hackrf_open_board(hackrf_device_t **device);
int hackrf_free_board(hackrf_device_t *device);



///////////////
// fft.c
int fbins_fft(fbins_t *fbins_in, fbins_t *fbins_out);
int fbins_average(fbins_t *fbins_in, fbins_t *fbins_out);



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


#endif
