
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>

#include "utils/debug.h"
#include "rfsweep/host.h"
#include "rfsweep/host.h"






typedef struct global_state_t {} state;







int main(void) {
    int err;
    //hackrf_device_t *device;
    hparams_t params;
    fbins_t fbins, qbins, abins;
    

    // setup default params
    //hparams_default(&params);

    // open hackrf board
    //err = hackrf_open_board(&device);
    //assert(("failed to open board", !err), err);

    // init hparams
    err = hparams_init(&params);
    assert(("failed to init params", !err), err);

    params.srate_hz *= 1;
    params.samps = 100;

    debugf("srate_hz = %f", params.srate_hz);

    // begin hackrf read
    err = hackrf_read(&params, &fbins);
    if (err) hparams_free(&params);
    assert(("failed to read from hackrf", !err), err);

    // wait until read finished
    hackrf_wait_until_finished(&params);



    // perform fft on bins
    fbins_fft(&fbins, &qbins);


    // average out the fft bins
    fbins_average(&qbins, &abins);


    // draw plot
    plot_fbins(&fbins, 0.0, fbins.flen / (float)params.srate_hz, 2000, false); //2621
    plot_fbins(&qbins, 0.0, params.srate_hz/2, 2000, true);
    plot_fbins(&abins, 0.0, params.srate_hz/2, 2000, true);

    // free hparams and fbins
    hparams_free(&params);
    fbins_free(&fbins);
    fbins_free(&qbins);
    fbins_free(&abins);

    // close hackrf board
    //err = hackrf_free_board(device);
    //assert(!err, err);

    debugf("program ran successfully\n");
    return 0;
}

