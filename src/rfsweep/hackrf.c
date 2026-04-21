#ifdef __LINUX__


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include <time.h>
#include <sched.h>

//#include <libhackrf/hackrf.h>
#include "libhackrf/hackrf.h"
//#include "kissfft/kiss_fft.h"

//#include "hackrf.h"


#define MY_HACKRF_SOURCE
#include "rfsweep.h"

// needs to be below "rfsweep.h" due to cygwin header conflicts
#include "toolkit/debug.h"
#include "toolkit/macros.h"


#else /* #ifdef __LINUX__ */


// for some reason having this here fixes errors in debug
// windows compatability has turned this into an inclusion nightmare
#include <stdio.h>
// to make sure prototypes are correct
#include "rfsweep.h"
#include "toolkit/debug.h"



#endif /* #ifdef __LINUX__ */





#ifndef __LINUX__

#define ADDAPI
#define ADDCALL

typedef struct {
    uint32_t bandwidth_hz;
} max2837_ft_t;

static const max2837_ft_t max2837_ft[] = {
    {1750000},
    {2500000},
    {3500000},
    {5000000},
    {5500000},
    {6000000},
    {7000000},
    {8000000},
    {9000000},
    {10000000},
    {12000000},
    {14000000},
    {15000000},
    {20000000},
    {24000000},
    {28000000},
    {0}};

extern ADDAPI uint32_t ADDCALL hackrf_compute_baseband_filter_bw(const uint32_t bandwidth_hz);

#endif /* #ifndef __LINUX__ */




// simply call free to free this
fbins_t *fbins_new(int32_t bcount) {
    fbins_t *mem;

    //debugf("allocating new fbin of size %d.", bcount);

    mem = malloc(sizeof(fbins_t) + bcount * sizeof(fbin_t));
    assert(mem != NULL, NULL);

    mem->bcount = bcount;
    
    return mem;
}





size_t fbins_sizeof(fbins_t *fbins) {
    return sizeof(fbins_t) + fbins->bcount * sizeof(fbin_t);
}








uint32_t hackrf_real_bandwidth(uint32_t band_hz) {
    uint32_t val = hackrf_compute_baseband_filter_bw(band_hz);
    //msgf("the selected filter bandwidth is %0.3f MHz", val/1e6f);
    return val;
}








// there is no way we are getting libusb to work on windows for now, so 
// we just won't compile with hackrf at all on windows, easy as that.
#ifndef __LINUX__



#define __HRF_RDCT(__retst)                                 \
    alertf(STR_ERROR, "NOT COMPILED FOR HACKRF LIBRARY!");  \
    __retst




// fbins_t *fbins_new(int32_t)                     {__HRF_RDCT(return NULL);}
// size_t fbins_sizeof(fbins_t *)                  {__HRF_RDCT(return -1);  }
void hparams_defaults(hparams_t *)              {__HRF_RDCT()            }
int hparams_init(hparams_t *, const char *)     {__HRF_RDCT(return -1);  }
void hparams_free(hparams_t *)                  {__HRF_RDCT()            }
int hackrf_read(hparams_t *)                    {__HRF_RDCT(return -1);  }
int hackrf_write(hparams_t *)                   {__HRF_RDCT(return -1);  }
int hackrf_stop(hparams_t *)                    {__HRF_RDCT(return -1);  }
int hackrf_is_finished(hparams_t *)             {__HRF_RDCT(return -1);  }
void hackrf_wait_until_finished(hparams_t *)    {__HRF_RDCT()            }
void hackrf_transmit_enable(hparams_t *)        {__HRF_RDCT()            }
void init_libhackrf(void)                       {__HRF_RDCT()            }
void exit_libhackrf(void)                       {__HRF_RDCT()            }





/* Return final bw. */
// From libhackrf source code
uint32_t ADDCALL hackrf_compute_baseband_filter_bw(const uint32_t bandwidth_hz)
{
    const max2837_ft_t* p = max2837_ft;
    while (p->bandwidth_hz != 0) {
        if (p->bandwidth_hz >= bandwidth_hz) {
            break;
        }
        p++;
    }

    /* Round down (if no equal to first entry) and if > bandwidth_hz */
    if (p != max2837_ft) {
        if (p->bandwidth_hz > bandwidth_hz)
            p--;
    }

    return p->bandwidth_hz;
}





#else /* #ifndef __LINUX__ */





// https://github.com/greatscottgadgets/hackrf/blob/main/host/libhackrf/src/hackrf.h
// https://github.com/DevRaf-Per/hackrf/wiki/libHackRF-API


// Reccomended minimum by hackrf docs
#define TRANSMIT_SRATE 8e6


//#define SAFE_LIBHACKRF_VERSION "0.9.1"
#define SAFE_LIBHACKRF_VERSION "unknown"

typedef hackrf_device hackrf_device_t;
typedef hackrf_transfer hackrf_transfer_t;


//#define TRANSFER_BUFFER_SIZE  262144




//#define fatalassert(__val, ...)





// TODO: add more user friendly errors
// perhaps an err function that determines what to print and if it is fatal or not



static int hackrf_open_board(hackrf_device_t **device, const char* serial);
// consider making this a void return
static int hackrf_free_board(hackrf_device_t *device);
static int setup_params(hackrf_device_t *device, hparams_t *params, bool is_transmit);
static int begin_receiver(hackrf_device_t *device, hparams_t *params);
static int begin_transmitter(hackrf_device_t *device, hparams_t *params);
static int stop_receiver(hackrf_device_t *device);
static int stop_transmitter(hackrf_device_t *device);
static int rx_callback(hackrf_transfer_t *transfer);
static int tx_callback(hackrf_transfer_t *transfer);


//typedef struct global_state_t {} state;








void hparams_defaults(hparams_t *params) {
    *params = (hparams_t){
        .srate_hz = 10e6,
        .freq_hz  = 2.4e6,
        //.band_hz  = 10e6,
        .band_hz  = 0, // 0 will just use the default of 0.75*srate
        .lna_gain = 16,
        .vga_gain = 20,
        .samps = 1,
        //.bins = 2048,
        .amp_enable = false,
        .tx_vga_gain = 20,
        .tx_amp = 127,
    };
}



// will open the first board it sees.
// just use hparams_default and explicit board open for more control, I guess.
int hparams_init(hparams_t *params, const char *serial) {
    int err;
    hparams_defaults(params);
    params->serial = serial;
    err = hackrf_open_board(&params->_device, serial);
    assert(("failed to open board", !err), err);
    return 0;
}



void hparams_free(hparams_t *params) {
    hackrf_free_board(params->_device);
    DEBUG(
        //*params = (hparams_t){0};
    )
}





int hackrf_read(hparams_t *params) {
    int err;
    //hackrf_device_t* device;

    //err = open_board(&device);
    // err = open_board(&device);
    // if (err) return err;

    err = setup_params(params->_device, params, false);
    assert(("problem setting up receiver params", !err), err);

    err = begin_receiver(params->_device, params);
    assert(("problem starting receiver", !err), err);

    // loops forever.
    //for(;;) sched_yield();



    // err = free_board(device);
    // if (err) return err;

    return 0;
}





int hackrf_write(hparams_t *params) {
    int err;

    err = setup_params(params->_device, params, true);
    assert(("problem setting up transmitter params", !err), err);

    err = begin_transmitter(params->_device, params);
    assert(("problem starting transmitter", !err), err);

    return 0;
}









int hackrf_stop(hparams_t *params) {
    int err;

    if (params->is_transmit)
        err = stop_transmitter(params->_device);
    else
        err = stop_receiver(params->_device);
        
    assert(!err, err);
    return 0;
}





//static __construct void init_libhackrf(void) {
void init_libhackrf(void) {
    // initilize the hackrf library
    int err = hackrf_init();
    vassert(("hackrf failed to initilize", !err));

    //fassert(("", TRANSFER_BUFFER_SIZE == hackrf_get_transfer_buffer_size(NULL)));
}


//static __destruct void exit_libhackrf(void) {
void exit_libhackrf(void) {
    // safely exit hackrf
    int err = hackrf_exit();
    vassert(!err);
}



// initilizes library and opens board
//int open_board(hackrf_device **device) {
static int hackrf_open_board(hackrf_device_t **device, const char* serial) {
    int err;
    //uint16_t version, usb_version;
    DEBUG(read_partid_serialno_t rpisn;)
    //hackrf_device_list_t *devices;
    
    
    // // initilize the hackrf library
    // err = hackrf_init();
    // assert(("hackrf failed to initilize", err == HACKRF_SUCCESS), err);
    //if (err) fatal(err);

    // warn user if a different untargeted hackrf lib version is being used
    if (strcmp(hackrf_library_version(), SAFE_LIBHACKRF_VERSION)) {
        warnf("target hackrf library version is '%s'. Instead, '%s' is being used.",
            SAFE_LIBHACKRF_VERSION, hackrf_library_version());
    }


    // open first available hackrf device
    err = hackrf_open_by_serial(serial, device);
    //err = hackrf_open(device);
    assert(("hackrf device could not be opened", err == HACKRF_SUCCESS), err);


    // print out board info
    ///////////////////////////////

    // get usb api version
//     err = hackrf_usb_api_version_read(device, &usb_version);
//     
//     printf("device %s opened\n", hackrf_board_id_name(hackrf_board_id(device)));
// 
//     printf("Firmware Version: %s (API:%x.%02x)\n", version,
//         (usb_version >> 8) & 0xFF, usb_version & 0xFF);
//     

    DEBUG(
    // get serial number
    err = hackrf_board_partid_serialno_read(*device, &rpisn);
    assert(!err, err);

    // print out serial number
    fprintf(stderr, "[debug] : device opened with serial #");
    for (int i = 0; i < 4; i++)
        fprintf(stderr, "%04x", rpisn.serial_no[i]);
    fprintf(stderr, "\n");
    
    )
    
    return 0;
}





static int hackrf_free_board(hackrf_device_t *device) {
    int err;

    // close opened device
    err = hackrf_close(device);
    assert((err == HACKRF_SUCCESS), err);

    // // free device list
    // hackrf_device_list_free(devices);

    // safely exit hackrf
    // err = hackrf_exit();
    // assert((err == HACKRF_SUCCESS), err);

    return 0;
}







static int setup_params(hackrf_device_t *device, hparams_t *params, bool is_transmit) {
    int err;
    //DEBUG(uint32_t real_band_hz;)
    uint32_t real_band_hz;
    
    //uint8_t enable_amp = false;
    //uint8_t enable_ant = true;
    
    //uint32_t bandwidth_hz = 10e6;   // half above and half below freq_hz
    //uint64_t freq_hz      = 2.4e6;  // center frequency
    //double sample_rate_hz = 10e6;   // should be between 2-20MHz
    
    //uint32_t lna_gain     = 16;
    //uint32_t vga_gain     = 20;


    // calculate actual bandwidth we will be using
    // will be forced to one of these: 1.75, 2.5, 3.5, 5, 5.5, 6, 7, 8, 
    //      9, 10, 12, 14, 15, 20, 24, 28MHz
    //DEBUG(
    if ((!is_transmit) && (params->band_hz != 0)) {
        real_band_hz = hackrf_compute_baseband_filter_bw(params->band_hz);
        assert(("param bandwidth not an accepted bandwidth", 
                real_band_hz == params->band_hz), -1);
    }
    //)

    // set clock_enable
    hackrf_set_clkout_enable(device, params->clockout_enable);
    

    // apparently won't be exact frequency. See the hackrf.h lib header
    // set center frequency
    err = hackrf_set_freq(device, params->freq_hz);
    assert(("hackrf_set_freq(...)", !err), err);

    // set sample rate
    if (is_transmit) {
        err = hackrf_set_sample_rate(device, TRANSMIT_SRATE);
        assert(("hackrf_set_sample_rate(...)", !err), err);
    } else {
        err = hackrf_set_sample_rate(device, params->srate_hz);
        assert(("hackrf_set_sample_rate(...)", !err), err);
    }

    // set vga and lna gain
    if (is_transmit) {
        err = hackrf_set_txvga_gain(device, params->tx_vga_gain);
        assert(("hackrf_set_txvga_gain(...)", !err), err);
    } else {
        err = hackrf_set_lna_gain(device, params->lna_gain);
        assert(("hackrf_set_lna_gain(...)", !err), err);
        err = hackrf_set_vga_gain(device, params->vga_gain);
        assert(("hackrf_set_vga_gain(...)", !err), err);
    }
    

    // debug(
    // printf("bandwidth of %uHz selected based on %uHz\n", 
    //     real_bandwidth_hz, bandwidth_hz);
    // );

    // toggle amplifier
    err = hackrf_set_amp_enable(device, params->amp_enable);
    assert(("hackrf_set_amp_enable(...)", !err), err);

    // set baseband sampling bandwidth
    // resets after sample rate is set, so call this after sample rate
    // if zero, then use default
    if ((!is_transmit) && (params->band_hz != 0)) {
        err = hackrf_set_baseband_filter_bandwidth(device, params->band_hz);
        assert(!err, err);
    }

    return 0;
}



static int begin_receiver(hackrf_device_t *device, hparams_t *params) {
    int err;

    if (!hackrf_is_finished(params))
        warnf("running transmitter when previous transmission is not complete");

    // enable antenna
    err = hackrf_set_antenna_enable(device, 1);
    assert(("hackrf_set_antenna_enable(..., 1)", !err), err);

    /*uint16_t frequency_range[2] = {};

    err = hackrf_init_sweep(device,
    const uint16_t* frequency_list,
    const int num_ranges,
    const uint32_t num_bytes,
    const uint32_t step_width,
    const uint32_t offset,
    const enum sweep_style style);*/

    //params->_fbins = fbins;

    // malloc space for fbins
    //fbins_init(fbins, params->samps, (int)hackrf_get_transfer_buffer_size(device)>>1);

    // malloc fbins
    // nevermind
    
    // setup loop params
    //params->_ibin = 0;
    params->_isamp = 0;

    params->is_transmit = false;

    err = hackrf_start_rx(device, (hackrf_sample_block_cb_fn)rx_callback, params);
    assert(("hackrf failed to start receiver loop", !err), err);

    return 0;
}






static int begin_transmitter(hackrf_device_t *device, hparams_t *params) {
    int err;

    if (!hackrf_is_finished(params))
        warnf("running receiver when previous transmission is not complete");

    // enable antenna
    err = hackrf_set_antenna_enable(device, 1);
    assert(("hackrf_set_antenna_enable(..., 1)", !err), err);

    params->is_transmit = true;

    err = hackrf_start_tx(device, (hackrf_sample_block_cb_fn)tx_callback, params);
    assert(("hackrf failed to start receiver loop", !err), err);

    return 0;
}






static int stop_receiver(hackrf_device_t *device) {
    int err;
    
    err = hackrf_stop_rx(device);
    assert(("hackrf failed to stop receiver loop", !err), err);

    // disable antenna
    err = hackrf_set_antenna_enable(device, 0);
    assert(("hackrf_set_antenna_enable(..., 0)", !err), err);

    return 0;
}





static int stop_transmitter(hackrf_device_t *device) {
    int err;
    
    err = hackrf_stop_tx(device);
    assert(("hackrf failed to stop transmitter loop", !err), err);

    // disable antenna
    err = hackrf_set_antenna_enable(device, 0);
    assert(("hackrf_set_antenna_enable(..., 0)", !err), err);

    return 0;
}





__weak_inline int hackrf_is_finished(hparams_t *params) {
    return (hackrf_is_streaming(params->_device) != HACKRF_TRUE);
}



void hackrf_wait_until_finished(hparams_t *params) {
    while (!hackrf_is_finished(params))
        //sched_yield();
        micros_block_for(1);

    // just in case call stop for clean exit
    hackrf_stop(params);
}


// static int rx_callback(hackrf_transfer_t *transfer) {
//     // printf("PRINTING BUFFER (length %d)", transfer->buffer_length);
//     // for (int i = 0; i < transfer->buffer_length; i++)
//     //     printf("%d ", transfer->buffer[i]);
//     // printf("BREAK\n");
//     // printf("\n");
//     // return 0;
// 
//     hparams_t *params = transfer->rx_ctx;
//     fbins_t *fbins = params->_fbins;
// 
//     //params->samps--;
// 
//     //debugf("read %d bytes from hackrf", transfer->buffer_length);
//     
//     // assert that buffer length is a multiple of 2 (even)
//     DEBUG(assert((transfer->buffer_length % 2 == 0), -1);)
//     // assert that fbins length is equal to buffer length
//     DEBUG(assert(("bins are of unequal length", transfer->buffer_length>>1 == fbins->blen), -1);)
//     
// 
//     // assert that the typeof(fbins->bins[0].real) is signed
//     /*_Static_assert((typeof(fbins->bins[0].real))0 - 1 < (typeof(fbins->bins[0].real))0,
//         "typeof(fbins->bins[0].real) must be signed")*/
//     // assert that the typeof(fbins->bins[0].real) is a float
//     _Static_assert(__builtin_types_compatible_p(float, typeof(fbins->bins[0][0].real)),
//         "typeof(fbins->bins[0].real) must be a float");
//     #define _RX_NORMALIZE(__n) (((int8_t)(__n - 128))/256.0f)
// 
//     #if 1
//     for (int i = 0; i < transfer->buffer_length; i += 2) {
//         fbins->bins[params->_i][i>>1].real = (float)_RX_NORMALIZE(transfer->buffer[i+0]);
//         fbins->bins[params->_i][i>>1].imag = (float)_RX_NORMALIZE(transfer->buffer[i+1]);
//     }
//     #endif
// 
//     // this to test speeds
//     // it seems like the bottleneck is libhackrf, which is probably due to usb rates and overhead
//     //memcpy(&fbins->bins[params->_i][0].real, transfer->buffer, transfer->buffer_length);
// 
//     // return zero will continue transfer
//     // whereas return nonzero will stop transfer
//     return (++params->_i >= params->samps);
// }







// static int rx_callback(hackrf_transfer_t *transfer) {
// 
//     hparams_t *params = transfer->rx_ctx;
//     fbins_t *fbins = params->_fbins;
// 
//     // assert that buffer length is a multiple of 2 (even)
//     DEBUG(assert((transfer->buffer_length % 2 == 0), -1);)
//     
//     // assert that the typeof(fbins->bins[0].real) is a float
//     _Static_assert(__builtin_types_compatible_p(float, typeof(fbins->bins[0][0].real)),
//         "typeof(fbins->bins[0].real) must be a float");
// 
//     // #if 1
//     // for (int i = 0; i < transfer->buffer_length; i += 2) {
//     //     fbins->bins[params->_i][i>>1].real = (float)_RX_NORMALIZE(transfer->buffer[i+0]);
//     //     fbins->bins[params->_i][i>>1].imag = (float)_RX_NORMALIZE(transfer->buffer[i+1]);
//     // }
//     // #endif
// 
//     // transfer buffer length rounded down to multiple of bin length
//     int transfer_length = transfer->buffer_length - transfer->buffer_length % params->bins;
// 
//     for (int i = 0; i < transfer_length; i += 2) {
// 
// 
// 
// 
//         // if all bins are filled, then move on to next sample
//         if (params->_ibin > params->bins) {
//             params->_isamp++;
//             params->_ibin = 0;
//         }
// 
//         // if all samples filled, then quit
//         if (params->_isamp > params->samps) {
//             return 1;
//         }
//     }
// 
//     // if we make it here, it means that the samps aren't filled, and we must take
//     // another bulk sample
// 
//     // return zero will continue transfer
//     // return nonzero will stop transfer
//     // continue transfer
//     return 0;
// }







DEBUG(
// https://github.com/llvm/llvm-project/blob/main/compiler-rt/include/sanitizer/lsan_interface.h
extern void __lsan_enable(void);
extern void __lsan_disable(void);
)


// this nightmare is what I get for writing the hackrf code before the server code
static int rx_callback(hackrf_transfer_t *transfer) {
    int err;
    fbins_t *fbins;
    hparams_t *params = transfer->rx_ctx;
    //fbins_t *fbins = params->_fbins;

    // assert that buffer length is a multiple of 2 (even)
    DEBUG(assert((transfer->buffer_length % 2 == 0), -1);)
    
    // assert that the typeof(fbins->bins[0].real) is a float
    // _Static_assert(__builtin_types_compatible_p(float, typeof(fbins->bins[0][0].real)),
    //     "typeof(fbins->bins[0].real) must be a float");

    // assert that size of fbin_t is 2 bytes
    _Static_assert(sizeof(fbin_t) == (1<<1), "fbin_t must be 2 bytes in size");

    // we are just going to directly push this into the binqueue
    // as defined in server.c
    // technically anyone can pop it, so it is *fine*
    //DEBUG(__lsan_disable();) // to ignore memory leaks
    fbins = fbins_new(transfer->buffer_length>>1);
    //DEBUG(__lsan_enable();) // to ignore memory leaks
    assert(fbins != NULL, 1);

    // record fbin properties
    *fbins = (fbins_t){
        .angle    = params->angle,
        .band_hz  = params->band_hz,
        .freq_hz  = params->freq_hz,
        .srate_hz = params->srate_hz,
        .timestamp_us = micros(),
        .bcount   = transfer->buffer_length>>1,
    };

    // copy buffer over to fbins
    // debugf("fbins size %d, buffer length %d at %d/%d", fbins_sizeof(fbins), 
    //         transfer->buffer_length, (int)params->_isamp, (int)params->samps);
    memcpy(fbins->bins, transfer->buffer, transfer->buffer_length);

    // push into binqueue
    err = binqueue_push(fbins);
    assert(!err, 1);

    //debugf("i=%d; samps=%d", params->_isamp, params->samps);

    // return zero will continue transfer
    // return nonzero will stop transfer
    return (++params->_isamp >= params->samps);
}





// we will need to set a small sample rate
// to prevent underflow
static int tx_callback(hackrf_transfer_t *transfer) {

    hparams_t *params = transfer->tx_ctx;

    // since this is shifted to the center frequency, all we need to do is 
    // transmit a dc signal
    transfer->valid_length = transfer->buffer_length;
    //transfer->valid_length = 10;

    for (int i = 0; i < transfer->buffer_length; i++) {
    //for (int i = 0; i < 10; i++) {
        ((int8_t*)transfer->buffer)[i] = params->tx_amp;
    }

    return 0;
}



#endif /* #ifndef __LINUX__ */






#ifdef __DEBUG__
int hackrf_run_tests(void) {
    int err;
    //hackrf_device_t *device;
    hparams_t params;
    fbins_t *fbins;

    // init hackrf
    init_libhackrf();

    // init binqueue
    err = binqueue_init();
    assert(!err, err);

    // init hparams
    err = hparams_init(&params, NULL);
    assert(("failed to init params", !err), err);

    params.srate_hz = 10e6*2;
    params.samps = 10;
    params.lna_gain = 30;
    params.vga_gain = 20;
    params.freq_hz = 2.4501e6; //5.2e6;
    params.band_hz = hackrf_real_bandwidth(10e6);
    //params.band_hz = 

    debugf("srate_hz = %.0f Hz", params.srate_hz);

    // begin hackrf read
    err = hackrf_read(&params);
    if (err) hparams_free(&params);
    assert(("failed to read from hackrf", !err), err);

    // wait until read finished
    hackrf_wait_until_finished(&params);

    fbins = binqueue_pop();

    //print out sample of reads
    debugf("printing out hackrf sample:");
    for (int i = 0; i < 20; i++) {
        printf("%d %d ", (int)fbins->bins[i].real, (int)fbins->bins[i].imag);
    }
    printf("\n");

    // free hparams
    hparams_free(&params);
    // free fbins
    free(fbins);

    // free binqueue
    binqueue_free();

    // exit hackrf
    exit_libhackrf();

    return 0;
}
#endif







/*



hackrf_board_id_name
hackrf_board_id_platform
hackrf_board_id_read
hackrf_board_partid_serialno_read
hackrf_board_rev_name
hackrf_board_rev_read
hackrf_close
hackrf_compute_baseband_filter_bw
hackrf_device_list
hackrf_device_list_free
hackrf_device_list_open
hackrf_error_name
hackrf_exit
hackrf_filter_path_name
hackrf_get_clkin_status
hackrf_get_transfer_buffer_size
hackrf_get_transfer_queue_depth
hackrf_library_release
hackrf_library_version
hackrf_open
hackrf_open_by_serial
hackrf_reset
hackrf_set_amp_enable
hackrf_set_antenna_enable
hackrf_set_baseband_filter_bandwidth
hackrf_set_clkout_enable
hackrf_set_freq
hackrf_set_freq_explicit
hackrf_set_hw_sync_mode
hackrf_set_leds
hackrf_set_lna_gain
hackrf_set_sample_rate
hackrf_set_sample_rate_manual
hackrf_set_txvga_gain
hackrf_set_vga_gain
hackrf_start_rx
hackrf_start_rx_sweep
hackrf_start_tx
hackrf_stop_rx
hackrf_stop_tx
hackrf_supported_platform_read
hackrf_usb_api_version_read
hackrf_usb_board_id_name
hackrf_version_string_read



hackrf_compute_baseband_filter_bw_round_down_lt
hackrf_cpld_checksum
hackrf_cpld_write
hackrf_enable_tx_flush
hackrf_get_m0_state
hackrf_get_operacake_boards
hackrf_get_operacake_mode
hackrf_is_streaming
hackrf_max2837_read
hackrf_max2837_write
hackrf_operacake_gpio_test
hackrf_rffc5071_read
hackrf_rffc5071_write
hackrf_set_operacake_dwell_times
hackrf_set_operacake_freq_ranges
hackrf_set_operacake_mode
hackrf_set_operacake_ports
hackrf_set_operacake_ranges
hackrf_set_rx_overrun_limit
hackrf_set_tx_block_complete_callback
hackrf_set_tx_underrun_limit
hackrf_set_user_bias_t_opts
hackrf_set_ui_enable
hackrf_si5351c_read
hackrf_si5351c_write
hackrf_spiflash_clear_status
hackrf_spiflash_erase
hackrf_spiflash_read
hackrf_spiflash_status
hackrf_spiflash_write





typedef struct {
    hackrf_device* device;
    uint8_t* buffer;
    int buffer_length;
    int valid_length;
    void* rx_ctx;
    void* tx_ctx;
} hackrf_transfer;

typedef struct {
    uint32_t part_id[2];
    uint32_t serial_no[4];
} read_partid_serialno_t;

typedef struct {
    uint32_t dwell;
    uint8_t port;
} hackrf_operacake_dwell_time;

typedef struct {
    uint16_t freq_min;
    uint16_t freq_max;
    uint8_t port;
} hackrf_operacake_freq_range;

typedef struct {
    bool do_update;
    bool change_on_mode_entry;
    bool enabled;
} hackrf_bool_user_settting;

typedef struct {
    hackrf_bool_user_settting tx;
    hackrf_bool_user_settting rx;
    hackrf_bool_user_settting off;
} hackrf_bias_t_user_settting_req;

typedef struct {
    uint16_t requested_mode;
    uint16_t request_flag;
    uint32_t active_mode;
    uint32_t m0_count;
    uint32_t m4_count;
    uint32_t num_shortfalls;
    uint32_t longest_shortfall;
    uint32_t shortfall_limit;
    uint32_t threshold;
    uint32_t next_mode;
    uint32_t error;
} hackrf_m0_state;


struct hackrf_device_list {
    char** serial_numbers;
    enum hackrf_usb_board_id* usb_board_ids;
    int* usb_device_index;
    int devicecount;
    void** usb_devices;
    int usb_devicecount;
};

*/
