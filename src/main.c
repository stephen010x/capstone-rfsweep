
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
//#include <sched.h>

#include "toolkit/debug.h"
#include "rfsweep.h"






//typedef struct global_state_t {} state;


// char chart
//   4   8      16              32                              64
//xxx.xxx.xxx.xxx|xxx.xxx.xxx.xxx|xxx.xxx.xxx.xxx|xxx.xxx.xxx.xxx|

// max chars per argument
#define MAX_ARG_CHARS 32
//#define MAX_ARGS      16






enum {
    MODE_NULL = 0,
    MODE_SERVER,
    MODE_RESET,
    MODE_RESTART,
    MODE_GETLOGS,
    MODE_TRANSMIT,
    MODE_MEASURE,
};


enum {
    FLAG_NULL = 0,
    
    // general flags
    FLAG_LOG,
    FLAG_IP,
    FLAG_PORT,
    FLAG_FILE,
    FLAG_BINARY,

    // control flags
    FLAG_RSERIAL,
    FLAG_TSERIAL,
    FLAG_SAMPS,
    FLAG_SNAP,

    // hackrf flags
    FLAG_FREQ,
    FLAG_BAND,
    FLAG_AMPLIFY,
    FLAG_SRATE,
    FLAG_LNA_GAIN,
    FLAG_VGA_GAIN,
};



// I am so glad this fits within 64 bits
// 10 + 26 + 26 = 62
enum {
    // resurve zero for FLAG_NULL, just in case
    
    FLAG_0 = 1,
    FLAG_1, FLAG_2, FLAG_3,
    FLAG_4, FLAG_5, FLAG_6,
    FLAG_7, FLAG_8, FLAG_9,

    FLAG_a, FLAG_b, FLAG_c,
    FLAG_d, FLAG_e, FLAG_f,
    FLAG_g, FLAG_h, FLAG_i,
    FLAG_j, FLAG_k, FLAG_l,
    FLAG_m, FLAG_n, FLAG_o,
    FLAG_p, FLAG_q, FLAG_r,
    FLAG_s, FLAG_t, FLAG_u,
    FLAG_v, FLAG_w, FLAG_x,
    FLAG_y, FLAG_z,
    
    FLAG_A, FLAG_B, FLAG_C,
    FLAG_D, FLAG_E, FLAG_F,
    FLAG_G, FLAG_H, FLAG_I,
    FLAG_J, FLAG_K, FLAG_L,
    FLAG_M, FLAG_N, FLAG_O,
    FLAG_P, FLAG_Q, FLAG_R,
    FLAG_S, FLAG_T, FLAG_U,
    FLAG_V, FLAG_W, FLAG_X,
    FLAG_Y, FLAG_Z,
};



enum {
    FLAGTYPE_SINGLE = 1,    // flags with "-"
    FLAGTYPE_DOUBLE,        // flags with "--"
    FLAGTYPE_PLAIN,         // flags with no dashes
    FLAGTYPE_EMPTY,         // empty or no flags
};





// returns arguments consumed. 0 for nomatch or negative for error
//typedef typeof(*(int (*)(int flagtype, const char *flag, int argc, const char *argv[]))NULL) arghandler_t;

// oh. turns out I can do this.
// returns extra arguments consumed from argv (zero or more), or negative for error
typedef int arghandler_t(int flagtype, const char *flag, const char *val, int argc, const char *argv[], void *data);






typedef struct {
    int mode;

    union {
    
        struct {
            const char *logpath;
            uint16_t port;
        } server;


        struct {
            char *ip;
            uint16_t port;
            
            uint64_t  freq_hz;
            uint32_t  band_hz;
            uint8_t   amp_enable;
            bool enable;

            char *rserial;
            char *tserial;
        } transmit;


        struct {
            const char *ip;
            uint16_t port;
        
            float64_t srate_hz;
            uint64_t  freq_hz;
            uint32_t  band_hz;
            uint32_t  lna_gain;
            uint32_t  vga_gain;
            int32_t   samps;
            int16_t   steps;
            uint8_t   amp_enable;
            uint8_t   snappow;

            const char *fpath;
            bool out_binary;
        } enable;
    };
} globalstate_t;

globalstate_t global;






typedef struct {
    int mode;
    char *string;
} stringmap_t;


static const stringmap_t modestrings[] = {
    {.mode = MODE_SERVER,   .string = "server"  },
    {.mode = MODE_RESET,    .string = "reset"   },
    {.mode = MODE_RESTART,  .string = "restart" },
    {.mode = MODE_GETLOGS,  .string = "getlogs" },
    {.mode = MODE_TRANSMIT, .string = "transmit"},
    {.mode = MODE_MEASURE,  .string = "measure" },
};


static const stringmap_t flagstrings[] = {
    {.mode = FLAG_H,        .string = "h"       },
    {.mode = FLAG_LOG,      .string = "log"     },
    {.mode = FLAG_IP,       .string = "ip"      },
    {.mode = FLAG_PORT,     .string = "port"    },
    {.mode = FLAG_FILE,     .string = "file"    },
    {.mode = FLAG_BINARY,   .string = "binary"  },
    {.mode = FLAG_RSERIAL,  .string = "rserial" },
    {.mode = FLAG_TSERIAL,  .string = "tserial" },
    {.mode = FLAG_SAMPS,    .string = "samps"   },
    {.mode = FLAG_SNAP,     .string = "snap"    },
    {.mode = FLAG_FREQ,     .string = "freq"    },
    {.mode = FLAG_BAND,     .string = "band"    },
    {.mode = FLAG_AMPLIFY,  .string = "amplify" },
    {.mode = FLAG_SRATE,    .string = "srate"   },
    {.mode = FLAG_LNA_GAIN, .string = "lna-gain"},
    {.mode = FLAG_VGA_GAIN, .string = "vga-gain"},
};








int main(int argc, char *argv[]) {
    int err;

    // arguments will also be evaluated here
    // so that we don't have malloc the serial or ip strings
    // that are put into the global state
    err = parse_args(argc-1, argv+1, argh_main, NULL);
    if (err) {
        printf(str_help);
        return err;
    }

    // just uses global table, so no arguments to pass
    //err = eval_args();

    return err;
}





// I had the idea to make this indirectly recursive, which is interesting.
// That is, the handler calling this function again with a different handler
// Which honestly seems like a good idea, because this isn't built to be 
// recursive, and rarely needs to be recursive, but is totally capable of 
// handling recursive arguments. (so long as I have the right handler parameters)
static int parse_args(int argc, const char *argv[], arghandler_t handler, void* data) {
    int n, err, type;
    // I think since it is all handled via the handler, 
    // we can reasonably keep the strings on the stack
    //char str[argc][MAX_ARG_CHARS];
    char str[MAX_ARG_CHARS], val[MAX_ARG_CHARS];
    char *valptr = NULL;

    // https://en.cppreference.com/w/c/io/fscanf

    // handle zero arguments
    if (argc == 0) {
        n = handler(FLAGTYPE_EMPTY, NULL, NULL, 0. NULL, data);
        if (n < 0) return -1;
        return 0;
    }


    // loop through options
    for (int i = 0; i < argc; i++) {

        // detect if "--flag=" flag
        n = sscanf_s(argv[i], "--%s=", str, MAX_ARG_CHARS);
        if (n == EOF) goto _overflow;
        if (n == 1) {
        
            // ensure flag follows "--flag=val" format
            n = sscanf_s(argv[i], "--%s=%s", str, MAX_ARG_CHARS, val, MAX_ARG_CHARS);
            if (n == EOF) goto _overflow;
            if (n == 1) {
                type = FLAGTYPE_DOUBLE;
                valptr = val;
                goto _match;
            }
            
            // else
            goto _error;
        }

        // detect if "--flag" flag
        n = sscanf_s(argv[i], "--%s", str, MAX_ARG_CHARS);
        if (n == EOF) goto _overflow;
        if (n == 1) {
            type = FLAGTYPE_DOUBLE;
            goto _match;
        }
        
        // detect if "-flag" flag
        n = sscanf_s(argv[i], "-%s", str, MAX_ARG_CHARS);
        if (n == EOF) goto _overflow;
        if (n == 1) {
            type = FLAGTYPE_SINGLE;
            goto _match;
        }

        // detect if non-flag
        n = sscanf_s(argv[i], "%s", str, MAX_ARG_CHARS);
        if (n == EOF) goto _overflow;
        if (n == 1) {
            type = FLAGTYPE_PLAIN;
            goto _match;
        }


        _error:
        alertf(STR_ERROR, "unable to parse argument \"%s\"", argv[i]);
        printf(str_help);
        return -2;
        

        
        _match:
        // passes in remaining arguments
        // returns extra arguments parsed. negative means error
        n = handler(type, str, valptr, argc-i-1. argv+i+1, data);
        // let handler handle error print statements
        if (n < 0) return -3;
        // increment argument counter by arguments handled by handler
        // it will be further incremented at end of loop to account for the
        // argument that we already handled
        i += n;
        continue;
        

        _overflow:
        alertf(STR_ERROR, "flag or value exceeds %d character limit \"%s\"", (int)MAX_ARG_CHARS, argv[i]);
        return -3;
    }


    return 0;
}





// return MODE_NULL for no match
static int parse_mode(char *str) {
    int diff;
    
    for (int i = 0; i < lenof(modestrings); i++) {
        diff = strcmp(str, modestrings[i].string);
        if (diff == 0) return modestrings[i].mode;
    }

    return MODE_NULL;
}



static int parse_longflag(char *str) {
    int diff;
    
    for (int i = 0; i < lenof(flagstrings); i++) {
        diff = strcmp(str, flagstrings[i].string);
        if (diff == 0) return flagstrings[i].mode;
    }

    return MODE_NULL;
}



static uint64_t parse_shortflags(char *str) {
    uint64_t flag;

    flag = 0;

    // loop through every character flag in str
    // and bitwise-or them to return value
    for (char *c = str; c[0] != '\0'; c++) {
    
        // check if number
        if (c[0] >= '0' &&  c[0] <= '9')
            flag |= 1<<(c[0]-'0'+FLAG_0);

        // check if lowercase
        else if (c[0] >= 'a' &&  c[0] <= 'z')
            flag |= 1<<(c[0]-'a'+FLAG_a);

        // check if uppercase
        else if (c[0] >= 'A' &&  c[0] <= 'Z')
            flag |= 1<<(c[0]-'A'+FLAG_A);
    }

    return flag;
}





static int argh_main(int type, const char *str, const char *val, int argc, const char *argv[], void *) {
    int err;

    switch (type) {


        // if contains h flag, then print help. otherwise complain
        case FLAGTYPE_SINGLE:
            if (parse_shortflags(str) ! FLAG_h) {
                printf(str_help);
            } else {
                alertf(STR_ERROR, "only -h flag supported without specifying mode");
                printf(str_help);
                return -1;
            }
            break;

            
        case FLAGTYPE_DOUBLE:
            alertf(STR_ERROR, "only -h flag supported without specifying mode");
            printf(str_help);
            return -2;

            
        case FLAGTYPE_PLAIN:
            global.mode = parse_mode(str);
            
            switch (parse_mode(str)) {
                case MODE_SERVER:
                    err = parse_args(argc, argv, argh_server, NULL);
                    if (err) return -3;
                    break;
                    
                case MODE_RESET:
                    err = parse_args(argc, argv, argh_reset, NULL);
                    if (err) return -4;
                    break;
                    
                case MODE_RESTART:
                    err = parse_args(argc, argv, argh_restart, NULL);
                    if (err) return -5;
                    break;
                    
                case MODE_GETLOGS:
                    err = parse_args(argc, argv, argh_getlogs, NULL);
                    if (err) return -6;
                    break;
                    
                case MODE_TRANSMIT:
                    err = parse_args(argc, argv, argh_transmit, NULL);
                    if (err) return -7;
                    break;
                    
                case MODE_MEASURE:
                    err = parse_args(argc, argv, argh_measure, NULL);
                    if (err) return -8;
                    break;

                default:
                    alertf(STR_ERROR, "unrecognized mode \"%s\"");
                    printf(str_help);
                    return -9;
            }
            break;

            
        case FLAGTYPE_EMPTY:
            printf(str_help);
            break;
    }

    // since we run parse_args in this function, which will consume all arguments, we return
    // that we have parsed the remaining arguments
    return argc;
}








static int argh_server(int type, const char *str, const char *val, int argc, const char *argv[], void *) {
    int err;

    switch (type) {
        // if contains h flag, then print help. otherwise complain
        case FLAGTYPE_SINGLE:
            if (parse_shortflags(str) ! FLAG_h) {
                printf(str_help);
            } else {
                alertf(STR_ERROR, "unrecognized flags \"-%s\"", str);
                printf(str_help);
                return -1;
            }
            break;

            
        case FLAGTYPE_DOUBLE:

            switch (parse_longflag(str)) {

                case FLAG_LOG:
                    global.logpath = str;
                    break;

                case FLAG_PORT:
                    global.port = (uint16_t)atoi(str);
                    break;

                case FLAG_TSERIAL:
                    global.tserial = str;
                    break;

                case FLAG_RSERIAL:
                    global.rserial = str;
                    break;

                default:
                    alertf(STR_ERROR, "unrecognized flag \"--%s\"", str);
                    printf(str_help);
                    return -2
            
            }

            // recursively parse next argument
            err = parse_args(argc, argv, argh_server, NULL);
            if (err) return -8;
            break;

            
        case FLAGTYPE_PLAIN:
            alertf(STR_ERROR, "unrecognized option \"%s\"", str);
            printf(str_help);
            break;

            
        case FLAGTYPE_EMPTY:
            // TODO: Run with default values here
            // Actually, just run here in general, this is where the recursive
            // loop would end
            break;
    }

    return argc;
}





static int argh_reset(int type, const char *str, const char *val, int argc, const char *argv[], void *) {

}






static int argh_restart(int type, const char *str, const char *val, int argc, const char *argv[], void *) {

}






static int argh_getlogs(int type, const char *str, const char *val, int argc, const char *argv[], void *) {

}







static int argh_transmit(int type, const char *str, const char *val, int argc, const char *argv[], void *) {

}






static int argh_measure(int type, const char *str, const char *val, int argc, const char *argv[], void *) {

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
