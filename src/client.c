#include <stdint.h>

#include "rfsweep.h"



// int client_connect(char *ip, uint16_t port);
// int client_send();
// int client_read();
// int client_

CLIENT_TIMEOUT 1000     /* one second */





static const int16_t _msg_reset   = MESSAGE_RESET;
static const int16_t _msg_restart = MESSAGE_RESTART;
static const int16_t _msg_getlogs = MESSAGE_GETLOGS;

#define CONST_MSG_RESET    (*(const message_t*)&_msg_reset)
#define CONST_MSG_RESTART  (*(const message_t*)&_msg_restart)
#define CONST_MSG_GETLOGS  (*(const message_t*)&_msg_getlogs)




int client_request_reset(globalstate_t *state);
int client_request_restart(globalstate_t *state);
int client_request_getlogs(globalstate_t *state);
int client_request_measure(globalstate_t *state);







int client_request_reset(globalstate_t *state) {
    int err;
    net_t client;
    message_t msg;

    err = net_connect(&client, state.ip, state.port, CLIENT_TIMEOUT);
    assert(!err, err);

    msg = message_read(&client, CLIENT_TIMEOUT);
    assert(msg != NULL, -1);

    err = _client_msg_handler(&msg);
    
    free(msg);
    assert(!err, -2);

    return 0;
}




static int _client_msg_handler(message_t *msg) {
    switch (msg->type) {
        case MESSAGE_NULL:
            alertf(STR_ERROR, "received NULL from server");
            return -1;

        case MESSAGE_ERROR:
            alertf(STR_ERROR, "received ERROR from server");
            return -1;

        case MESSAGE_RECEIVED:
            alertf(STR_WARN, "received unexpected RECEIVED from server");
            return 0;

        case MESSAGE_POLL:
            alertf(STR_WARN, "received unexpected POLL from server");
    }

    return 0;
}
