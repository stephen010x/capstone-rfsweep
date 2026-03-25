#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <inttypes.h>

#include "toolkit/debug.h"
#include "rfsweep.h"



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
static int dump_strto_file(const char *restrict path, const char *restrict format, ...);
static int append_strto_file(const char *restrict path, const char *restrict format, ...);





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






int client_request_reset(globalstate_t *state) {
    msgf("Sending RESET request to server");
    return _client_request(state, &CONST_MSG_RESET);
}



int client_request_restart(globalstate_t *state) {
    msgf("Sending RESTART request to server");
    return _client_request(state, &CONST_MSG_RESTART);
}



int client_request_ping(globalstate_t *state) {
    msgf("Sending PING to server");
    return _client_request(state, &CONST_MSG_PING);
}









int client_request_getlogs(globalstate_t *state) {
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








int client_request_measure(globalstate_t *state) {
    int err;
    net_t net;
    message_t *msg;

    msgf("Sending MEASURE request to server");

    // connect to server
    err = net_connect(&net, state->ip, state->port, -1);
    assert(!err, err);

    // create and populate message
    msg = message_new(MESSAGE_MEASURE, 0);
    msg->measure = (typeof(msg->measure)){
        .lna_gain   = state->lna_gain,
        .vga_gain   = state->vga_gain,
        .srate_hz   = state->srate_hz,
        .freq_hz    = state->freq_hz,
        .band_hz    = state->band_hz,
        .samps      = state->samps,
        .steps      = state->steps,
        .amp_enable = state->amp_enable,
        .snappow    = state->snappow, 
    };

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
                debugf("Measurements complete.");
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






static int _handle_measuredata(const globalstate_t *restrict state, const message_t *restrict msg, int j) {
    int err;
    fbins_t *fbins;
    (void)j;

    fbins = (void*)msg->data.data;

    // make sure that sizes check out
    //debugf("msg data size (%d), fbins sizeof (%d)", (int)msg->data.size, (int)fbins_sizeof(fbins));
    assert(((size_t)msg->data.size == fbins_sizeof(fbins)), -1);

    // if outputting binary
    if (state->out_binary) {
        alertf(STR_ERROR, "binary output not supported yet");
        return -1;


    // if ascii output and no output file
    } else if (state->fpath == NULL) {

        // print out params
        printf("%" PRId64 " %f %" PRIu64 " %" PRIu32 " %f %" PRId32,
               fbins->timestamp_us, (double)fbins->angle, fbins->freq_hz, 
               fbins->band_hz, (double)fbins->srate_hz, fbins->bcount);

        // print out bins
        for (int i = 0; i < fbins->bcount; i++)
            printf(" %" PRId8 " %" PRId8,
                   fbins->bins[fbins->bcount-1].real,
                   fbins->bins[fbins->bcount-1].imag);
            
        printf("\n");


    // if ascii output and output file
    } else {

        // print out params
        err = append_strto_file(state->fpath, "%" PRId64 " %f %" PRIu64 " %" PRIu32 " %f %" PRId32,
               fbins->timestamp_us, fbins->angle, fbins->freq_hz, 
               fbins->band_hz, fbins->srate_hz, fbins->bcount);
        assert(!err, -2);

        // print out bins
        for (int i = 0; i < fbins->bcount; i++) {
            err = append_strto_file(state->fpath, " %" PRIu8 " %" PRIu8,
                   fbins->bins[fbins->bcount-1].real,
                   fbins->bins[fbins->bcount-1].imag);
            assert(!err, -3);
        }
            
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
                //alertf(STR_WARN, "received unexpected \"%s\" message from server", msgstr[msg->type]);
                alertf(STR_ERROR, "received unexpected \"%s\" message from server", msgstr[msg->type]);
                return -2;
            } else {
                alertf(STR_ERROR, "received unknown message \"%d\" from server", msg->type);
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
    assert(file != NULL, -1);

    // write to file
    //err = fputs(str, file);
    err = vfprintf(file, format, vlist);
    jassert(err >= 0, _exit_file);

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
