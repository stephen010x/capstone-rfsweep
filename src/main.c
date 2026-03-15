
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>

#include "toolkit/debug.h"
#include "rfsweep.h"






typedef struct global_state_t {} state;










char helpstr[] = 
    "\n"
    "usage: hyperc -h\n"
    "       hyperc [options] [infile] [outfile]\n"
    "\n"
    "options:\n"
    "    -h        print help options\n"
    "    --test    run tests (only available with debug version)\n"
    "\n";




// char flags_a[] = {
//     [AFLAG_H] = "h",
// };

char *flags_b[] = {
    [BFLAG_TEST] = "test",
};





global_t global = {0};



//int test(void);
int parse_args(int argc, char *argv[]);
int parse_flag(flagid_t flagid);
void print_help(void);




int main(int argc, char *argv[]) {
    int err;
    
    err = parse_args(argc, argv);
    if (err)
        return -1;

    if (global.flags & GFLAG_TEST)
#       ifdef __DEBUG__
        run_tests();
#       else
        printf("--test is not avaliable in the release version\n");
#       endif
    
    return 0;
}



// Alright, the arguments for this will be simple
// hyperc [flags] [infile] [outfile]
int parse_args(int argc, char *argv[]) {
    int index = 1;

    // for (int i = 0; i < argc; i++) {
    //     printf("[arg %d]\t%s\n", i, argv[i]);
    // }

    // loop through options
    for (; index < argc; index++) {
        char *flag = argv[index];
        //flagid_t flagid = FLAGID_INVALID;

        // if not a flag then stop looking for flags and break
        if (flag[0] != '-')
            break;

        // check if flag type 'a' or type 'b'
        
        if (flag[1] == '-') {   // type b

            for (int i = 0; (size_t)i < lenof(flags_b); i++) {

                if (strcmp(flags_b[i], flag+2) == 0) {

                    int err = parse_flag(FLAGID_TYPE_B | i);
                    if (err) {
                        printf("'%s' is not a valid flag\n", flag);
                        return err;
                    }
                    
                    break;
                }
            }
            
        } else {                // type a

            for (int i = 1; flag[i] != '\0'; i++) {

                int err = parse_flag(FLAGID_TYPE_A | (flagid_t)(unsigned)flag[i]);
                if (err) {
                    printf("'-%c' is not a valid flag\n", flag[i]);
                    return err;
                }
                
            }
            
        }
    }

    // if noop specified, then don't look for infile and outfile arguments
    if (global.flags & GFLAG_NOOP)
        return 0;

    if (index >= argc) {
        printf("no input file specified\n");
        return -1;
    }

    global.infile = argv[index++];

        if (index >= argc) {
        printf("no output target specified\n");
        return -1;
    }

    global.outfile = argv[index++];
    
    return 0;
}



int parse_flag(flagid_t flagid) {
    switch (flagid) {
        case FLAGID_TYPE_A | (flagid_t)(unsigned)'h':
            global.flags |= GFLAG_NOOP;
            print_help();
            break;
            
        case FLAGID_TYPE_B | BFLAG_TEST:
            global.flags |= GFLAG_TEST;
            break;
            
        default: 
            return -1;
    }
    return 0;
}





void print_help(void) {
    printf(helpstr);
}














// 
// 
// // void __destruct main_exit(void) {
// // 	debugf("disabling stepper");
// // 	stepper_enable(false);
// // }
// 
// 
// 
// 
// 
// int main(void) {
//     int err;
// 
//     //DEBUG(micros_test();)
// 
//     DEBUG(
//     printf("-----------------------------\n");
//     net_tests();
//     printf("-----------------------------\n");
//     )
// 
//     err = stepper_enable(true);
//     assert(("failed to enable stepper motor", !err), err);
//     
//     micros_block_for(10e5);
// 
// //     err = stepper_mode(STEP_MODE_1_1);
// //     assert(!err, err); 
// // 
// // 	DEBUG(stepper_test();)
// 
//     stepper_mode(STEP_MODE_1_4);
//     assert(!err, err); 
// 
//     stepper_setorigin();
//     int step = 0;
//     for(int i = 0; (step>>4 <= 200) && (step>>4 >= -200); i++) {
//         stepper_step(STEP_DIR_CLOCKWISE);
//         //micros_block_for(10e3);
//         //debugf("step");
//         
//         step = stepper_getsteps();
//         //debugf("step %d %d", step>>4, step&0xF);
//     }
// 
//     micros_block_for(10e5);
// 
//     // take out of microstepping mode
//     stepper_mode(STEP_MODE_1_1);
// 
//     // disable motor
//     stepper_enable(false);
// 
//     #if 0
//     //hackrf_device_t *device;
//     hparams_t params;
//     fbins_t fbins, fbins2, qbins, qbins2, abins, abins2;
// 
//     // setup default params
//     //hparams_default(&params);
// 
//     // open hackrf board
//     //err = hackrf_open_board(&device);
//     //assert(("failed to open board", !err), err);
// 
//     // init hparams
//     err = hparams_init(&params);
//     assert(("failed to init params", !err), err);
// 
//     params.srate_hz = 10e6*2;
//     params.samps = 1000;
//     params.lna_gain = 30;
//     params.vga_gain = 20;
//     params.freq_hz = 2.4501e6; //5.2e6;
//     params.band_hz = hackrf_real_bandwidth(10e6);
//     //params.band_hz = 
// 
//     debugf("srate_hz = %.0f Hz", params.srate_hz);
// 
//     // begin hackrf read
//     err = hackrf_read(&params, &fbins);
//     if (err) hparams_free(&params);
//     assert(("failed to read from hackrf", !err), err);
// 
//     // wait until read finished
//     hackrf_wait_until_finished(&params);
// 
//     // free hparams
//     hparams_free(&params);
// 
// 
//     err = fbins_segment(&fbins, &fbins2, 2048, 64);
//     assert(!err, err);
//     
// 
//     // perform fft on bins
//     err = fbins_fft(&fbins2, &qbins);
//     assert(!err, err);
// 
// 
//     // average out the fft bins
//     err = fbins_average(&qbins, &abins);
//     assert(!err, err);
// 
//     // convert to log domain
//     fbins_log(&qbins, &qbins2);
//     fbins_log(&abins, &abins2);
// 
//     // draw plot
//     plot_fbins(&fbins, 0.0, fbins.flen / (float)params.srate_hz, 2000, false); //2621
//     plot_fbins(&qbins, 0.0, params.srate_hz, 2000, true);
//     plot_fbins(&abins, 0.0, params.srate_hz, 2000, true);
// 
//     // free hparams and fbins
//     //hparams_free(&params);
//     fbins_free(&fbins);
//     fbins_free(&fbins2);
//     fbins_free(&qbins);
//     fbins_free(&abins);
// 
//     // close hackrf board
//     //err = hackrf_free_board(device);
//     //assert(!err, err);
//     
//     #endif
// 
//     debugf("program ran successfully");
//     return 0;
// }
// 
