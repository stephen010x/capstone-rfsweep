#ifndef RFSWEEP_HOST_H
#define RFSWEEP_HOST_H



typedef struct {
    uint8_t real;
    uint8_t imag;
} sbin_t;

typedef struct sbins_t {
    int len;
    sbin_t *bins;
} sbins_t;


typedef struct {
    double   srate_hz;
    uint64_t freq_hz;
    uint32_t band_hz;
    uint32_t lna_gain;
    uint32_t vga_gain;
    volatile uint16_t samps;
    uint8_t  amp_enable;

    // backend only
    sbins_t *_sbins;
} hparams_t;




#ifndef MY_HACKRF_SOURCE
typedef void hackrf_device_t;
#endif



int sbins_init(sbins_t *sbins, int len);
void sbins_free(sbins_t sbins);

void hparams_defaults(hparams_t *params);


uint32_t hackrf_real_bandwidth(uint32_t band_hz);
int hackrf_read(hackrf_device_t *device, hparams_t *params, sbins_t *sbins);
int hackrf_stop(hackrf_device_t *device);
int hackrf_open_board(hackrf_device_t **device);
int hackrf_free_board(hackrf_device_t *device);
int hackrf_is_finished(hackrf_device_t *device);
void hackrf_wait_until_finished(hackrf_device_t *device);




#endif
