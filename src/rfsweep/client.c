#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <inttypes.h>

#include "rfsweep.h"
// needs to be below "rfsweep.h" due to cygwin header conflicts
#include "toolkit/debug.h"



// int client_connect(char *ip, uint16_t port);
// int client_send();
// int client_read();
// int client_

#define CLIENT_TIMEOUT 1000     /* one second */





static const int16_t _msg_reset   = MESSAGE_RESET;
static const int16_t _msg_restart = MESSAGE_RESTART;
static const int16_t _msg_getlogs = MESSAGE_GETLOGS;
static const int16_t _msg_ping    = MESSAGE_PING;

#define CONST_MSG_RESET    (*(const message_t*)&_msg_reset)
#define CONST_MSG_RESTART  (*(const message_t*)&_msg_restart)
#define CONST_MSG_GETLOGS  (*(const message_t*)&_msg_getlogs)
#define CONST_MSG_PING     (*(const message_t*)&_msg_ping)






static int _client_request(const globalstate_t *restrict state, const message_t *restrict msg);
static int _handle_measuredata(const globalstate_t *restrict state, const message_t *restrict msg, int i);
static int _client_msg_handler(const message_t *msg);
static int _write_strto_file(const char *restrict path, const char *restrict c, const char *restrict format, va_list vlist);
static int _write_binto_file(const char *restrict path, const char *restrict c, const void *buff, size_t len);
static int dump_strto_file(const char *restrict path, const char *restrict format, ...);
static int append_strto_file(const char *restrict path, const char *restrict format, ...);
static int __unused dump_binto_file(const char *restrict path, const void *buff, size_t len);
static int append_binto_file(const char *restrict path, const void *buff, size_t len);
static int _client_request_data(const globalstate_t *state, int msgtype);




static const char *msgstr[MESSAGE_TYPE_LEN] = {
    [MESSAGE_NULL]              = "NULL",
    [MESSAGE_ERROR]             = "ERROR",
    //[MESSAGE_RECEIVED]          = "RECEIVED",
    [MESSAGE_SUCCESS]           = "SUCCESS",
    [MESSAGE_POLL]              = "POLL",
    [MESSAGE_DATA]              = "DATA",
    [MESSAGE_RESET]             = "RESET",
    [MESSAGE_RESTART]           = "RESTART",
    [MESSAGE_GETLOGS]           = "GETLOGS",
    [MESSAGE_MEASURE]           = "MEASURE",
    [MESSAGE_TRANSMIT_ENABLE]   = "TRANSMIT_ENABLE",
    [MESSAGE_TRANSMIT_DISABLE]  = "TRANSMIT_DISABLE",
    [MESSAGE_UNSUPPORTED]       = "UNSUPPORTED",
    [MESSAGE_END]               = "END",
    [MESSAGE_PING]              = "PING",
    [MESSAGE_RECEIVE]           = "RECEIVE",
    [MESSAGE_ROTATE]            = "ROTATE",
    //[MESSAGE_STOP]              = "STOP",
};



static const char *outformatstr = "timestamp(us) angle(degrees) freq(Hz) bandwidth(Hz) "
                                  "samplerate(Hz) bincount [real imag ...]\n";






static int _client_request(const globalstate_t *restrict state, const message_t *restrict msg) {
    int err;
    net_t net;
    //message_t *msg;

    // connect to server
    err = net_connect(&net, state->ip, state->port, CLIENT_TIMEOUT);
    assert(!err, err);

    // send request to server
    err = message_write(&net, msg, CLIENT_TIMEOUT);
    jassert(!err, _exit_net);

    // we dont need the msg pointer anymore, so we reuse it
    // read response from server
    msg = message_read(&net, CLIENT_TIMEOUT);
    if (msg == NULL) err = -1;
    jassert(!err, _exit_net);

    // handle response from server
    err = _client_msg_handler(msg);
    jassert(!err, _exit_msg);

    _exit_msg:
    // free message
    // discard const qualifier since we reused the msg ptr
    free(*(message_t**)&msg);
    
    _exit_net:
    // free net
    err |= net_close(&net, CLIENT_TIMEOUT);
    
    return err;
}






int client_request_reset(const globalstate_t *state) {
    msgf("Sending RESET request to server");
    return _client_request(state, &CONST_MSG_RESET);
}



int client_request_restart(const globalstate_t *state) {
    msgf("Sending RESTART request to server");
    return _client_request(state, &CONST_MSG_RESTART);
}



int client_request_ping(const globalstate_t *state) {
    msgf("Sending PING to server");
    return _client_request(state, &CONST_MSG_PING);
}









int client_request_getlogs(const globalstate_t *state) {
    int err;
    net_t net;
    message_t *msg;

    msgf("Sending GETLOGS request to server");

    // connect to server
    err = net_connect(&net, state->ip, state->port, CLIENT_TIMEOUT);
    assert(!err, err);

    // send request to server
    err = message_write(&net, &CONST_MSG_GETLOGS, CLIENT_TIMEOUT);
    jassert(!err, _exit_net);

    // read response from server
    msg = message_read(&net, CLIENT_TIMEOUT);
    err = (msg != NULL) ? 0 : -1;
    jassert(!err, _exit_net);

    // handle response from server
    switch (msg->type) {
    
        case MESSAGE_SUCCESS:
            alertf(STR_ERROR, "received unexpected \"SUCCESS\" message from server");
            err = -2;
            goto _exit_msg;
    
        case MESSAGE_DATA:
            // check for null terminator
            jassert(("data is missing null terminator",
                    (msg->data.data[msg->data.size-1] != '\0')), _exit_msg);
            // print data to stdout or file
            if (state->logpath == NULL) {
                printf("%s", (char*)msg->data.data);
            } else {
                err = dump_strto_file(state->logpath, "%s", (char*)msg->data.data);
                jassert(!err, _exit_msg);
            }
            break;

        default:
            err = _client_msg_handler(msg);
            jassert(!err, _exit_msg);
    }

    _exit_msg:
    // free message
    free(msg);
    
    _exit_net:
    // free net
    err |= net_close(&net, CLIENT_TIMEOUT);
    
    return err;
}








static int _client_request_data(const globalstate_t *state, int msgtype) {
    int err;
    net_t net;
    message_t *msg;

    // connect to server
    err = net_connect(&net, state->ip, state->port, CLIENT_TIMEOUT);
    assert(!err, err);

    

    // create and populate message
    msg = message_new(msgtype, 0);
    msg->measure = (typeof(msg->measure)){
        .lna_gain   = state->lna_gain,
        .vga_gain   = state->vga_gain,
        .srate_hz   = state->srate_hz,
        .freq_hz    = state->freq_hz,
        //.band_hz    = state->band_hz,
        .samps      = state->samps,
        .steps      = state->steps,
        .amp_enable = state->amp_enable,
        .snappow    = state->snappow, 
    };



    // some bandwidth checks and defaults
    if (state->band_hz == 0) {
        // we are doing this now ourselves so that we can check if aliasing happens
        msg->measure.band_hz = hackrf_real_bandwidth(state->srate_hz * 3 / 4);
        msgf("Selecting default passband filter bandwidth of %d Hz.", (int)msg->measure.band_hz);
        
    } else {
        msg->measure.band_hz = hackrf_real_bandwidth(state->band_hz);

        if (msg->measure.band_hz != state->band_hz)
            warnf("Rounding filter bandwidth to %d Hz", (int)msg->measure.band_hz);
    }

    if (state->band_hz > state->srate_hz) {
        warnf("The passband filter bandwidth (%f Hz) exceeds the sample rate (%f Hz)! "
              "This will result in aliasing!",
              (double)state->band_hz, (double)state->srate_hz);
    }
    


    // send request to server
    err = message_write(&net, msg, CLIENT_TIMEOUT);
    jassert(!err, _exit_net);

    // free message to be reused for the return message
    free(msg);

    // loop until end message received or error
    for (int i = 0;;) {
        // read response from server
        msg = message_read(&net, CLIENT_TIMEOUT);
        err = (msg != NULL) ? 0 : -1;
        jassert(!err, _exit_net);

        // message throttle
        //micros_block_for(100000);

        // handle response from server
        // err = _client_msg_handler(&msg);
        // jassert(!err, _exit_msg);
        switch (msg->type) {
        
            case MESSAGE_SUCCESS:
                alertf(STR_ERROR, "received unexpected \"SUCCESS\" message from server");
                err = -2;
                goto _exit_msg;


            case MESSAGE_POLL:
                // comment this out later
                //debugf("Server POLLING...");
                break;

        
            case MESSAGE_DATA:
                if (!state->out_binary) {
                    // print format information
                    if (i == 0) {
                        if (state->fpath) {
                            dump_strto_file(state->fpath, outformatstr);
                        } else {
                            printf(outformatstr);
                        }
                    }
                }
                msgf("collecting data [%d/%d]", i+1, state->steps);
                err = _handle_measuredata(state, msg, i);
                i++;
                break;

            case MESSAGE_END:
                msgf("Measurements complete.");
                err = 0;
                free(msg);
                goto _exit_net;


            default:
                err = _client_msg_handler(msg);
                jassert(!err, _exit_msg);
        }

        _exit_msg:
        // free message
        free(msg);

        // if error, then exit loop
        if (err) break;
    }
    
    _exit_net:
    // free net
    err |= net_close(&net, CLIENT_TIMEOUT);
    
    return err;
}




int client_request_measure(const globalstate_t *state) {
    msgf("Sending MEASURE request to server");
    return _client_request_data(state, MESSAGE_MEASURE);
}






static int _handle_measuredata(const globalstate_t *restrict state, const message_t *restrict msg, int j) {
    int err, n;
    fbins_t *fbins;
    (void)j;
    

    fbins = (void*)msg->data.data;

    // make sure that sizes check out
    //debugf("msg data size (%d), fbins sizeof (%d)", (int)msg->data.size, (int)fbins_sizeof(fbins));
    assert(((size_t)msg->data.size == fbins_sizeof(fbins)), -1);

    // if outputting binary
    if (state->out_binary) {
        //alertf(STR_ERROR, "binary output not supported yet");
        //return -1;

        //uint8_t data[sizeof(float64_t)];
        uint64_t data;
        //float64_t data;

        // DEBUG(__lsan_disable();) // to ignore memory leaks
        // DEBUG(__lsan_enable();) // to ignore memory leaks

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wstrict-aliasing"

        *(float64_t*)&data = (float64_t)fbins->timestamp_us;
        err = append_binto_file(state->fpath, &data, sizeof(float64_t));
        if (err) error(-1);

        *(float64_t*)&data = (float64_t)fbins->angle;
        err = append_binto_file(state->fpath, &data, sizeof(float64_t));
        if (err) error(-2);

        *(float64_t*)&data = (float64_t)fbins->freq_hz;
        err = append_binto_file(state->fpath, &data, sizeof(float64_t));
        if (err) error(-3);

        *(float64_t*)&data = (float64_t)fbins->band_hz;
        err = append_binto_file(state->fpath, &data, sizeof(float64_t));
        if (err) error(-4);

        *(float64_t*)&data = (float64_t)fbins->srate_hz;
        err = append_binto_file(state->fpath, &data, sizeof(float64_t));
        if (err) error(-5);

        *(int64_t*)&data = (int64_t)fbins->bcount*2;
        err = append_binto_file(state->fpath, &data, sizeof(float64_t));
        if (err) error(-6);

        err = append_binto_file(state->fpath, fbins->bins, 
                        fbins->bcount * sizeof(fbin_t));
        if (err) error(-6);

        #pragma GCC diagnostic pop


    // if ascii output and no output file
    } else if (state->fpath == NULL) {

        // print out params
        printf("%" PRId64 " %f %" PRIu64 " %" PRIu32 " %f %" PRId32,
               fbins->timestamp_us, (double)fbins->angle, fbins->freq_hz, 
               fbins->band_hz, (double)fbins->srate_hz, fbins->bcount*2);

        // print out bins
        for (int i = 0; i < fbins->bcount; i++)
            printf(" %" PRId8 " %" PRId8,
                   fbins->bins[i].real, fbins->bins[i].imag);
            
        printf("\n");


    // if ascii output and output file
    } else {

        // print out params
        err = append_strto_file(state->fpath, 
                "%" PRId64 " %f %" PRIu64 " %" PRIu32 " %f %" PRId32,
                fbins->timestamp_us, fbins->angle, fbins->freq_hz, 
                fbins->band_hz, fbins->srate_hz, fbins->bcount*2);
        assert(!err, -2);

        // print out bins
        #define _BUFFSIZE ((intptr_t)(8*(fbins->bcount)))
        char *const buff = malloc(_BUFFSIZE+1);
        char *buffoff = buff;
        
        for (int i = 0; i < fbins->bcount; i++) {
            //err = append_strto_file(state->fpath, " %" PRIu8 " %" PRIu8,
            // err = append_strto_file(state->fpath, " %hhd %hhd",
            //        (uint8_t)fbins->bins[i].real, (uint8_t)fbins->bins[i].imag);
            // assert(!err, -3);

            if (_BUFFSIZE - (intptr_t)(buffoff - buff) < 0) {
                alertf(STR_FATAL, "character buffer overflow");
                fatal(1);
            }
            
            n = snprintf(buffoff, _BUFFSIZE - (intptr_t)(buffoff - buff), " %hhd %hhd",
                        fbins->bins[i].real, fbins->bins[i].imag);


            buffoff += n;
        }
        buff[_BUFFSIZE] = '\0';
        err = append_strto_file(state->fpath, buff);
        free(buff);
        assert(!err, -3);

            
        err = append_strto_file(state->fpath, "\n");
        assert(!err, -4);
    }

    return 0;
}








static int _client_msg_handler(const message_t *msg) {
    
    switch (msg->type) {
    
        case MESSAGE_NULL:
        case MESSAGE_ERROR:
        case MESSAGE_UNSUPPORTED:
            alertf(STR_ERROR, "received \"%s\" message from server", msgstr[msg->type]);
            return -1;

        case MESSAGE_SUCCESS:
            break;

        default:
            if (msg->type < MESSAGE_TYPE_LEN) {
                DEBUG(fassert(msgstr[msg->type] != NULL);)
                //alertf(STR_WARN, "received unexpected \"%s\" message from server", 
                //          vmsgstr[msg->type]);
                alertf(STR_ERROR, "received unexpected \"%s\" message from server", 
                            msgstr[msg->type]);
                return -2;
            } else {
                alertf(STR_ERROR, "received unknown message \"%d\" from server", 
                            msg->type);
                return -3;
            }
    }

    return 0;
}





static int _write_strto_file(const char *restrict path, const char *restrict c, const char *restrict format, va_list vlist) {
    int err;
    FILE *file;

    // open file
    file = fopen(path, c);
    if (file == NULL) alertf(STR_ERROR, "failed to open file");
    assert(file != NULL, -1);

    // write to file
    //err = fputs(str, file);
    err = vfprintf(file, format, vlist);
    if (err < 0) alertf(STR_ERROR, "failed to write to file")
    jassert(err >= 0, _exit_file);

    _exit_file:
    // close file
    fclose(file);
    return !(err >= 0);
}




static int _write_binto_file(const char *restrict path, const char *restrict c, const void *buff, size_t len) {
    int err;
    size_t n;
    FILE *file;

    err = 0;

    // open file
    file = fopen(path, c);
    if (file == NULL) alertf(STR_ERROR, "failed to open file %s\n.", path);
    assert(file != NULL, -1);

    // write to file
    n = fwrite(buff, 1, len, file);
    if (n != len) {
        alertf(STR_ERROR, "failed to write to file file %s\n.", path);
        err = -2;
    }
    jassert(n == len, _exit_file);

    _exit_file:
    // close file
    fclose(file);
    return err;
}




static int dump_strto_file(const char *restrict path, const char *restrict format, ...) {
    int err;
    va_list vlist;
    va_start(vlist, format);
    err = _write_strto_file(path, "w", format, vlist);
    va_end(vlist);
    return err;
}


static int append_strto_file(const char *restrict path, const char *restrict format, ...) {
    int err;
    va_list vlist;
    va_start(vlist, format);
    err = _write_strto_file(path, "a", format, vlist);
    va_end(vlist);
    return err;
}



static int dump_binto_file(const char *restrict path, const void *buff, size_t len) {
    return _write_binto_file(path, "w", buff, len);
}



static int append_binto_file(const char *restrict path, const void *buff, size_t len) {
    return _write_binto_file(path, "a", buff, len);
}









static int _client_request_success(const globalstate_t *state, message_t *msg) {
    int err;
    net_t net;
    //message_t *rmsg;

    // connect to server
    err = net_connect(&net, state->ip, state->port, CLIENT_TIMEOUT);
    assert(!err, err);

    // send request to server
    err = message_write(&net, msg, CLIENT_TIMEOUT);
    jassert(!err, _exit_net);

    // free message to be reused for the return message
    //free(msg);

    // loop until end message received or error
    for (;;) {
        // read response from server
        msg = message_read(&net, CLIENT_TIMEOUT);
        err = (msg != NULL) ? 0 : -1;
        jassert(!err, _exit_net);

        // handle response from server
        // err = _client_msg_handler(&msg);
        // jassert(!err, _exit_msg);
        switch (msg->type) {


            case MESSAGE_POLL:
                // comment this out later
                //debugf("Server POLLING...");
                break;


            case MESSAGE_SUCCESS:
                msgf("Rotation complete.");
                err = 0;
                free(msg);
                goto _exit_net;


            default:
                err = _client_msg_handler(msg);
                jassert(!err, _exit_msg);
        }

        _exit_msg:
        // free message
        free(msg);

        // if error, then exit loop
        if (err) break;
    }
    
    _exit_net:
    // free net
    err |= net_close(&net, CLIENT_TIMEOUT);
    
    return err;
}




int client_request_rotate(const globalstate_t *state) {
    int err;
    message_t *msg;

    msgf("Sending ROTATE request to server");

    // create and populate message
    msg = message_new(MESSAGE_ROTATE, 0);
    if (state->is_angle) {
        msg->rotate = (typeof(msg->rotate)){
            .is_angle = state->is_angle,
            .angle    = state->angle,
        };
    } else {
        msg->rotate = (typeof(msg->rotate)){
            .is_angle = state->is_angle,
            .steps    = state->steps,
        };
    }

    switch (state->stepmode) {
        case 1:  msg->rotate.stepmode = STEP_MODE_1_1;  break;
        case 2:  msg->rotate.stepmode = STEP_MODE_1_2;  break;
        case 4:  msg->rotate.stepmode = STEP_MODE_1_4;  break;
        case 8:  msg->rotate.stepmode = STEP_MODE_1_8;  break;
        case 16: msg->rotate.stepmode = STEP_MODE_1_16; break;
        
        default:
            free(msg);
            alertf(STR_ERROR, "invalid stepmode \"%d\".", state->stepmode);
            return -1;
    }

    err = _client_request_success(state, msg);

    free(msg);

    return err;
}







int client_request_receive(const globalstate_t *state) {
    msgf("Sending RECEIVE request to server");
    return _client_request_data(state, MESSAGE_RECEIVE);
}






int client_request_transmit(const globalstate_t *state) {
    int err;
    message_t *msg;

    msgf("Sending TRANSMIT request to server");

    // create and populate message
    if (state->transmit_enable) {
        msg = message_new(MESSAGE_TRANSMIT_ENABLE, 0);
        msg->transmit_enable = (typeof(msg->transmit_enable)){
                .freq_hz    = state->freq_hz,
                .amp_enable = state->amp_enable,
                .vga_gain   = state->vga_gain,
                .tx_amp     = state->tx_amp,
        };
    } else {
        msg = message_new(MESSAGE_TRANSMIT_DISABLE, 0);
    }

    err = _client_request_success(state, msg);

    free(msg);

    return err;
}
