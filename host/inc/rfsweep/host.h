#ifndef RFSWEEP_HOST_H
#define RFSWEEP_HOST_H


#include <stdint.h>
#include <stdbool.h>



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


int fbins_fft(fbins_t *fbins_in, fbins_t *fbins_out);
int fbins_average(fbins_t *fbins_in, fbins_t *fbins_out);


int plot_fbins(fbins_t *fbins, float xstart, float xend, int xlen, bool do_average);


#endif
