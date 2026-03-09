
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>

#include "utils/debug.h"
#include "rfsweep/host.h"
#include "rfsweep/host.h"






typedef struct global_state_t {} state;





// void __destruct main_exit(void) {
// 	debugf("disabling stepper");
// 	stepper_enable(false);
// }





int main(void) {
    int err;

    DEBUG(micros_test();)

    err = stepper_enable(true);
    assert(("failed to enable stepper motor", !err), err);

    err = stepper_mode(STEP_MODE_1_1);
    assert(!err, err); 

	DEBUG(stepper_test();)

	stepper_mode(STEP_MODE_1_4);
	for(int i = 0;; i++) {
		stepper_step(STEP_DIR_COUNTERCLOCK);
		micros_block_for(10e3);
		debugf("step");
	}

    #if 0
    //hackrf_device_t *device;
    hparams_t params;
    fbins_t fbins, fbins2, qbins, qbins2, abins, abins2;

    // setup default params
    //hparams_default(&params);

    // open hackrf board
    //err = hackrf_open_board(&device);
    //assert(("failed to open board", !err), err);

    // init hparams
    err = hparams_init(&params);
    assert(("failed to init params", !err), err);

    params.srate_hz = 10e6*2;
    params.samps = 1000;
    params.lna_gain = 30;
    params.vga_gain = 20;
    params.freq_hz = 2.4501e6; //5.2e6;
    params.band_hz = hackrf_real_bandwidth(10e6);
    //params.band_hz = 

    debugf("srate_hz = %.0f Hz", params.srate_hz);

    // begin hackrf read
    err = hackrf_read(&params, &fbins);
    if (err) hparams_free(&params);
    assert(("failed to read from hackrf", !err), err);

    // wait until read finished
    hackrf_wait_until_finished(&params);

    // free hparams
    hparams_free(&params);


    err = fbins_segment(&fbins, &fbins2, 2048, 64);
    assert(!err, err);
    

    // perform fft on bins
    err = fbins_fft(&fbins2, &qbins);
    assert(!err, err);


    // average out the fft bins
    err = fbins_average(&qbins, &abins);
    assert(!err, err);

    // convert to log domain
    fbins_log(&qbins, &qbins2);
    fbins_log(&abins, &abins2);

    // draw plot
    plot_fbins(&fbins, 0.0, fbins.flen / (float)params.srate_hz, 2000, false); //2621
    plot_fbins(&qbins, 0.0, params.srate_hz, 2000, true);
    plot_fbins(&abins, 0.0, params.srate_hz, 2000, true);

    // free hparams and fbins
    //hparams_free(&params);
    fbins_free(&fbins);
    fbins_free(&fbins2);
    fbins_free(&qbins);
    fbins_free(&abins);

    // close hackrf board
    //err = hackrf_free_board(device);
    //assert(!err, err);
    
    #endif

    debugf("program ran successfully");
    return 0;
}

