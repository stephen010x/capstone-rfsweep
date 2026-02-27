
#include <stdio.h>
#include <stlib.h>
#include <stdint.h>
#include <string.h>
//#include <time.h>
#include <sched.h>

//#include <libhackrf/hackrf.h>
#include "libhackrf/hackrf.h"
#include "kissfft/kiss_fft.h"

//#include "hackrf.h"
#include "utils/debug.h"
#include "utils/macros.h"


#define MY_HACKRF_SOURCE
#include "rfsweep-host.h"


// https://github.com/greatscottgadgets/hackrf/blob/main/host/libhackrf/src/hackrf.h
// https://github.com/DevRaf-Per/hackrf/wiki/libHackRF-API


//#define SAFE_LIBHACKRF_VERSION "0.9.1"
#define SAFE_LIBHACKRF_VERSION "unknown"

typedef hackrf_device hackrf_device_t;
typedef hackrf_transfer hackrf_transfer_t;




//#define fatalassert(__val, ...)





// TODO: add more user friendly errors
// perhaps an err function that determines what to print and if it is fatal or not



int hackrf_open_board(hackrf_device_t **device);
int hackrf_free_board(hackrf_device_t *device);

static int setup_receiver_params(hackrf_device_t *device, hparams_t *params);
static int begin_receiver(hackrf_device_t *device, hparams_t *params, sbins_t *sbins);
static int stop_receiver(hackrf_device_t *device);
static int rx_callback(hackrf_transfer_t *transfer);


//typedef struct global_state_t {} state;







void sbins_free(sbins_t sbins) {
    free(sbins->bins);
    debug(
        sbins = (sbins_t){0};
    );
}



int sbins_init(sbins_t *sbins, int len) {
    void *mem = malloc(sizeof(sbin_t) * len);

    if (mem == NULL) {
        debug(
            *sbins = (sbins_t){
                .len = 0,
                .bins = NULL,
            };
        );
        return -1;
    }
    
    *sbins = (sbins_t){
        .len = len,
        .bins = mem,
    };
    
    return 0;
}




void hparams_defaults(hparams_t *params) {
    *params = (hparams_t){
        .srate_hz = 10e6,
        .freq_hz  = 2.4e6,
        .band_hz  = 10e6,
        .lna_gain = 16,
        .vga_gain = 20,
        .samps = 100,
        .amp_enable = false,
    };
}




uint32_t hackrf_real_bandwidth(uint32_t band_hz) {
    return hackrf_compute_baseband_filter_bw(band_hz);
}





int hackrf_read(hackrf_device_t *device, hparams_t *params, sbins_t *sbins) {
    int err;
    //hackrf_device_t* device;

    //err = open_board(&device);
    // err = open_board(&device);
    // if (err) return err;

    err = setup_receiver_params(device, params);
    assert(("problem setting up receiver params", !err), err);

    err = begin_receiver(device, params, sbins);
    assert(("problem starting receiver", !err), err);

    // loops forever.
    //for(;;) sched_yield();



    // err = free_board(device);
    // if (err) return err;

    return 0;
}





static __construct void init_libhackrf(void) {
    // initilize the hackrf library
    int err = hackrf_init();
    assert(("hackrf failed to initilize", err == HACKRF_SUCCESS), err);
}


static __destruct void exit_libhackrf() {
    // safely exit hackrf
    int err = hackrf_exit();
    assert((err == HACKRF_SUCCESS), err)
}



// initilizes library and opens board
//int open_board(hackrf_device **device) {
int hackrf_open_board(hackrf_device_t **device) {
    int err;
    //uint16_t version, usb_version;
    read_partid_serialno_t rpisn;
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
    err = hackrf_open(device);
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

    debug(
    // get serial number
    err = hackrf_board_partid_serialno_read(*device, &rpisn);
    assert(!err, err);

    // print out serial number
    printf("device opened with serial #");
    for (int i = 0; i < 4; i++)
        printf("%04x", rpisn.serial_no[i]);
    printf("\n");
    );
    
    return 0;
}





int hackrf_free_board(hackrf_device_t *device) {
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







static int setup_receiver_params(hackrf_device_t *device, hparams_t *params) {
    int err;
    debug(uint32_t real_band_hz;);
    
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
    debug(
    real_bandwidth_hz = hackrf_compute_baseband_filter_bw(bandwidth_hz);
    assert(("param bandwidth not an accepted bandwidth", real_bandwidth_hz == param->band_hz), -1);
    )
    

    // apparently won't be exact frequency. See the hackrf.h lib header
    // set center frequency
    err = hackrf_set_freq(device, params->freq_hz);
    assert(("hackrf_set_freq(...)", !err), err);

    // set sample rate
    err = hackrf_set_sample_rate(device, params->srate_hz);
    assert(("hackrf_set_sample_rate(...)", !err), err);

    // set vga and lna gain
    err = hackrf_set_lna_gain(device, params->lna_gain);
    assert(("hackrf_set_lna_gain(...)", !err), err);
    err = hackrf_set_vga_gain(device, params->vga_gain);
    assert(("hackrf_set_vga_gain(...)", !err), err);
    

    // debug(
    // printf("bandwidth of %uHz selected based on %uHz\n", 
    //     real_bandwidth_hz, bandwidth_hz);
    // );

    // toggle amplifier
    err = hackrf_set_amp_enable(device, params->amp_enable);
    assert(("hackrf_set_amp_enable(...)", !err), err);

    // set baseband sampling bandwidth
    // resets after sample rate is set, so call this after sample rate
    err = hackrf_set_baseband_filter_bandwidth(device, params->band_hz);
    assert(!err, err);

    return 0;
}



int begin_receiver(hackrf_device_t *device, hparams_t *params, sbins_t *sbins) {
    int err;

    // enable antenna
    err = hackrf_set_antenna_enable(device, HACKRF_TRUE);
    assert(("hackrf_set_antenna_enable(..., HACKRF_TRUE)", !err), err);

    /*uint16_t frequency_range[2] = {};

    err = hackrf_init_sweep(device,
	const uint16_t* frequency_list,
	const int num_ranges,
	const uint32_t num_bytes,
	const uint32_t step_width,
	const uint32_t offset,
	const enum sweep_style style);*/

	params->_sbins = sbins;

    err = hackrf_start_rx(device, (hackrf_sample_block_cb_fn)rx_callback, params);
    assert(("hackrf failed to start receiver loop", !err), err);

    return 0;
}


int stop_receiver(hackrf_device_t *device) {
    int err;
    
    err = hackrf_stop_rx(device);
    assert(("hackrf failed to start receiver loop", !err), err);

    // disable antenna
    err = hackrf_set_antenna_enable(device, HACKRF_FALSE);
    assert(("hackrf_set_antenna_enable(..., HACKRF_FALSE)", !err), err);

    return 0;
}




int hackrf_is_finished(hackrf_device_t *device) {
    return (hackrf_is_streaming(device) != HACKRF_TRUE);
}



void hackrf_wait_until_finished(hackrf_device_t *device) {
    while (!hackrf_is_finished(device))
        sched_yield();
}


static int rx_callback(hackrf_transfer_t *transfer) {
    // printf("PRINTING BUFFER (length %d)", transfer->buffer_length);
    // for (int i = 0; i < transfer->buffer_length; i++)
    //     printf("%d ", transfer->buffer[i]);
    // printf("BREAK\n");
    // printf("\n");
    // return 0;

    hparams_t *params = transfer->rx_ctx;
    sbins_t *sbins = params->sbins;

    params->samps--;

    printf("TRANSFER OF %d BYTES\n", transfer->buffer_length);
    debug(assert(transfer->buffer_length % 2 == 0););

    for (int i = 0; i < (transfer->buffer_length >> 1); i += 2) {
        sbins->bins[i>>1].real = transfer->buffer[i+0];
        sbins->bins[i>>1].imag = transfer->buffer[i+1];
    }

    // return zero will continue transfer
    // whereas return nonzero will stop transfer
    return (params->samps <= 0);
}


// typedef struct {
//     uint8_t real;
//     uint8_t imag;
// } sbin_t;
// 
// typedef struct sbins_t {
//     int len;
//     sbin_t *bins;
// } sbins_t;



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
