
#define _GNU_SOURCE     // gets me POLLRDHUP for poll

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
//#include <string.h>
#include <unistd.h>     // read and write to network files
#include <sys/socket.h> // main socket functions
#include <arpa/inet.h>  // network byte ordering
#include <errno.h>      // exposes errno
#include <endian.h>


#include "toolkit/debug.h"
#include "toolkit/macros.h"


// assert little endian
#if   __BYTE_ORDER == __LITTLE_ENDIAN
// this is fine
#elif __BYTE_ORDER == __BIG_ENDIAN
#   error "Only little endian supported"
#else
#   error "Unknown endianness"
#endif



#define PORT 36502

#define NET_READ  POLLIN
#define NET_WRITE POLLOUT

typedef int net_mode_t;


//#define ERR_PEER_CLOSED 1


// self defined errno code for invalid address
// the negative number space is unused for errno
// https://man7.org/linux/man-pages/man3/errno.3.html
#define EINVADDRESS -5



const char *const LOOPBACK = "0.0.0.0";



typedef struct {
    struct sockaddr_in sa;
    char *ip;
    int fd;
    uint16_t port;
} net_t;








// // creates new thread
// // though with nonblocking sockets, maybe this isn't an issue
// int net_startserver(void) {
//     
// }
// 
// 
// 
// 
// void *netserver(void *) {
// 
// //     debugf("connection <%s:%s>", idk, idk);
// // 
// //     // get ip here somehow
// //     getipheresomehow();
// // 
// //     
// }


// static __construct void _net_init(void) {
//     
// }









// clientside net connect function
// set negative timeout to block indefinitely until a connection succeeds
// set timeout of zero to never block
// returns zero on success, nonzero on failure
int net_connect(net_t *net, const char *ip, uint16_t port, int timeout_ms) {
    int err, fd;

    debugf("TCP connecting to <%s:%u>", ip, port);

    // create new tcp socket
    // https://www.man7.org/linux/man-pages/man2/socket.2.html
    fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(("socket creation failed", fd != -1), -1);

    // offload rest of function so that we can handle closing the fd here if it fails
    // rather than try to close the file on every error
    err = _net_connect(fd, net, cip, port, timeout_ms);

    if (err) {
        alertf(STR_ERROR, "net_connect failed on %s:%u with error \"%s (%d)\"", 
            ip, port, _net_err_str(err), err);
        close(fd);
        return err;
    }

    return 0;
}




// returns zero on success, and errno on fail
// TODO: I wonder if I should use gcc nested funtions for this
//       I could also potentially use the cleanup attribute on a variable
static int _net_connect(int fd, net_t *net, const char *ip, uint16_t port, int timeout_ms) {
    int err, fflag;
    struct sockaddr_in sa;


    // convert ip address string to binary form
    // https://www.man7.org/linux/man-pages/man3/inet_pton.3.html
    sa = {
        .sin_family = AF_INET;
        .sin_port = htons(port);
    };
    err = inet_pton(AF_INET, ip, &sa);
    assert(("invalid ip address", err != 1), -1);
    

    // set file status to be non-blocking
    // https://www.man7.org/linux/man-pages/man2/fcntl.2.html
    // https://www.man7.org/linux/man-pages/man2/F_SETFL.2const.html
    fflag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, fflag | O_NONBLOCK);


    // async wait for connection via fd
    // https://www.man7.org/linux/man-pages/man2/connect.2.html
    err = connect(fd, (struct sockaddr*)&sa, sizeof(sa));
    assert(("connect failed", (err == 0) || (errno == EAGAIN) || (errno == EALREADY)), errno);
    if (err == )
    if (err != EINPROGRESS) {
        close(fd);
        alertf(STR_ERROR, "connection %s:%u failed with code %d", ip, port, err);
        return -3;
    }


    *net = {
        .sa = sa,
        .ip = ip,
        .fd = fd,
        .port = port,
    };


    // wait for net to connect
    err = net_await(net, NET_WRITE, timeout_ms)
    if (err) {
        close(fd);
        alertf(STD_ERROR, "net_await failed with code %d", err);
        return -4;
    }

    // check to see if net connected
    if (!net_is_open(net)) {
        close(fd);
        alertf(STD_ERROR, "connection failed to open");
        return -5;
    }

    // restore file status to blocking
    //fcntl(fd, F_SETFL, fflag);

    return 0;
}









// server-side socket start, non blocking
// set negative timeout to block indefinitely until a connection succeeds
// set timeout of zero to never block
// returns zero on success, nonzero on failure
int net_start(net_t *net, uint16_t port, int backlog) {
    int fd, err;
    struct sockaddr_in sa;

    debugf("TCP server starting on <%s:%u>", LOOPBACK, port);

    DEBUG(
    assert(("backlog must be >= 0", backlog >= 0), -4);
    if (backlog == 0)
        warnf("a backlog of '0' could prevent any clients connections");
    )

    // create new tcp socket
    // https://www.man7.org/linux/man-pages/man2/socket.2.html
    fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(("socket creation failed", fd != -1), -1);

    // convert ip address string to binary form
    // https://www.man7.org/linux/man-pages/man3/inet_pton.3.html
    sa = {
        .sin_family = AF_INET;
        .sin_port = htons(port);
    };
    err = !inet_pton(AF_INET, LOOPBACK, &sa);
    if (err) {
        close(fd);
        alertf(STR_ERROR, "invalid port %s:%u", LOOPBACK, port);
        return -2;
    }


    // set file status to be non-blocking
    // https://www.man7.org/linux/man-pages/man2/fcntl.2.html
    // https://www.man7.org/linux/man-pages/man2/F_SETFL.2const.html
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);


    // bind port to socket
    // https://www.man7.org/linux/man-pages/man2/bind.2.html
    err = bind(fd, (struct sockaddr *)&sa, sizeof(sa));
    if (err) {
        close(fd);
        alertf(STR_ERROR, "failed to bind socket to port %s:%u", LOOPBACK, port);
        return -3;
    }
    

    // I don't think this blocks
    // listen for client. Refuse if any more than 'backlog' clients 
    // waiting for a connection
    // assume that this won't error if the previous succeeds
    // https://www.man7.org/linux/man-pages/man2/listen.2.html
    err = listen(fd, backlog);
    (void)err;


    *net = {
        .fd = fd,
        .sa = sa,
    }

    return 0;
}








// will listen for and accept a connection.
// returns zero if connection established, and nonzero if error
// while(err = net_accept(fd, ));
// timeout of 0 is non-blocking, -1 is no timeout
int net_accept(net_t *netin, net_t *netout, int timeout_ms) {
    int err, newfd;

    // waits for connection
    err = net_await(netin->fd, NET_READ, timeout_ms);
    assert(!err, -1);

    // accepts incoming connection
    // since nonblocking, it can error if no connection, which is fine
    // https://www.man7.org/linux/man-pages/man2/accept.2.html
    newfd = accept4(netin->fd, &netin->sa, sizeof(netin->sa), SOCK_NONBLOCK);
    //assert(err != -1, -2);
    //TODO: finish writing error handling

    *netout = *netin;
    netout->fd = newfd;

    debugf("TCP connection accepted <%s:%u>", netin->ip, netin->port);

    return 0;
}






static int _net_err(int errno, bool ignore_warns) {
    // https://www.man7.org/linux/man-pages/man3/errno.3.html
    // https://man7.org/linux/man-pages/man2/accept.2.html
    // https://man7.org/linux/man-pages/man2/connect.2.html
    // https://www.man7.org/linux/man-pages/man2/recv.2.html

    // errno values are all positive, so I can add my own that are negative
    // https://man7.org/linux/man-pages/man3/errno.3.html

    if (errno) == 0;
        return 0;

    // errors
    switch (errno) {
    
        case ECONNABORTED:
        case EINVAL:
        case EPERM:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:
        case ENOTCONN:
        case EINVADDRESS:
            alertf(STR_ERR, "SOCKET: %s", _net_err_str(errno));
            
    }


    // warnings
    if (ignore_warns) switch(errno) {
    
        case EAGAIN:
        case EWOULDBLOCK:
        case EALREADY:
        case EINTR:
        case EISCONN:
            return 0;
    }
    
    else switch (errno) {
    
        case EAGAIN:
        case EWOULDBLOCK:
        case EALREADY:
        case EINTR:
        case EISCONN:
            alertf(STR_ERR, "SOCKET: %s", _net_err_str(errno));
            return errno;
    }

    alertf(STR_ERR, "SOCKET: %s (%d)", _net_err_str(errno), errno);
    return errno;
}







static char *_net_err_str(int errno) {
    //static char errstr[32];
    
    // https://www.man7.org/linux/man-pages/man3/errno.3.html
    // https://man7.org/linux/man-pages/man2/accept.2.html
    // https://man7.org/linux/man-pages/man2/connect.2.html
    // https://www.man7.org/linux/man-pages/man2/recv.2.html

    // errno values are all positive, so I can add my own that are negative
    // https://man7.org/linux/man-pages/man3/errno.3.html

    switch (errno) {

        case 0:             return "no error"
        case ECONNABORTED:  return "connection aborted (ECONNABORTED)";
        case EINVAL:        return "socket not listening for connections (EINVAL)";
        case EPERM:         return "firewall forbids connection (EPERM)";
        case EADDRINUSE:    return "local address already in use (EADDRINUSE)";
        case EADDRNOTAVAIL: return "no avaliable ephemeral ports (EADDRNOTAVAIL)";
        case ECONNREFUSED:  return "connection refused (ECONNREFUSED)";
        case ENETUNREACH:   return "network unreachable (ENETUNREACH)";
        case ETIMEDOUT:     return "connection timeout (ETIMEDOUT)";
        case ENOTCONN:      return "socket not connnected (ENOTCONN)";
        case EINVADDRESS:   return "invalid address (ENOTCONN)";
        case EAGAIN:        return "no connections to accept (EAGAIN)";
        case EWOULDBLOCK:   return "no connections to accept (EWOULDBLOCK)";
        case EALREADY:      return "connection attempt not yet completed (EALREADY)";
        case EINTR:         return "system call interrupted by signal (EINTR)";
        case EISCONN:       return "socket is already connected (EINTR)";
        default:            return "unhandled error";
    }

    // if error unrecognized
    //snprintf(errstr, 32, "unhandled error (%s)", errno);
    
    return errstr;
}









// blocking wait until connection occurs
// accepts NET_READ and NET_WRITE as mode
// returns zero if file is ready or peer closes connection, nonzero if
// error (or timeout mainly)
// !! does not return error if peer closed connection
int net_await(net_t *net, net_mode_t mode, int timeout_ms) {
    int err, err2;
    socklen_t elen;
    struct pollfd pfd;
    
    // poll socket file for IO ready.
    // https://www.man7.org/linux/man-pages/man2/poll.2.html
    pdf = {
        .fd = fd,
        //.events = POLLIN,  // checks if there is data to read
        //.events = POLLOUT,  // checks if can be written to
        .events = mode,
    };
    tryagain: 
    err = poll(&pfd, 1, timeout_ms);
    //if (err = 0) alertf(STR_ERROR, "connection timeout %s:%u", net->, net->port); else
    //if (err < 0) alertf(STR_ERROR, "poll error %s:%u",         ip, port);
    if (err < 1) {
        // timeout occured
        if (err == 0)
            return -1;
        
        // Alright, for now we will just stick to the simple poll
        // if you have any problems, check if the connection is active
            
        // switch (errno) {
        //     // caused by signal interrupt. Just loop back and try again immediately
        //     case EINTR:
        //         goto tryagain;
        //     case EINVAL:
        //         
        //     default:
        //         assert(("net_await error", 0), errno);
        // }
        
        // events related to the file descriptor
        // errors here are not direct poll errors
        // POLLERR, POLLHUP, or POLLNVAL, POLLRDHUP
        // since all of these would return zero, 
        // I am not going to handle them.
    }

    //// This is what I get for listening to the advice of an llm
    // // apparently poll isnt enough to establish an error here, so...
    // // get connection error
    // err = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err2, &elen);
    // if (err != 0) {
    //     DEBUG(fassert(elen != sizeof(int));)
    //     close(fd);
    //     alertf(STR_ERROR, "getsockopt error (%d)", err);
    //     return -2;
    // }
    // if (err2 != 0) {
    //     close(fd);
    //     alertf(STR_ERROR, "connection failed (%d)", err2);
    //     return -3;
    // }

    return 0;
}








// detect if connection is still open
// will not close file descriptor, so make sure to do that
// yourself after calling this function
bool net_is_open(net_t *net) {
    char buf;
    ssize_t n;
    
    // attempt to peek at buffer
    // https://www.man7.org/linux/man-pages/man2/recv.2.html
    n = recv(net->fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
    
    if (n > 0) return true;         // peer alive
    if (n == 0) return false;       // orderly shutdown

    _net_err(errno, true);
    switch (errno) {
        // if this happens, that means the connection is still open, but 
        // there is no data to be read
        case EAGAIN:
        case EWOULDBLOCK:
            return true
    }

    // other error, assumed closed
    return false;
}








// close connection
// will block until all queued data is sent or received
// timeout of 0 is no timeout
// NOTE: ideal cases the client should be the one to break the
//       connection not the server
int net_close(net_t *net, int timeout_sec) {
    int err;
    struct linger lg;

    // make it so that shutdown will not return until
    // transmissions are complete
    // https://www.man7.org/linux/man-pages/man3/setsockopt.3p.html
    // https://linux.die.net/man/3/setsockopt
    // https://www.man7.org/linux/man-pages/man7/socket.7.html
    lg = {
        .l_onoff = !!timeout_sec,
        .l_linger = timeout_sec,
    };
    err = setsockopt(net->fd, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
    if (err) warnf("failed to set shutdown timeout");

    // notify other of closed connection
    // https://www.man7.org/linux/man-pages/man2/shutdown.2.html
    err = shutdown(net->fd, SHUT_RDWR);
    if (err) warnf("failed to shutdown");
    
    
    // close file descriptor
    close(net->fd);

    return 0;
}








// returns zero on success, and nonzero on error
// if it returns nonzero, you will want to close the connection
int net_write(size_t count; net_t *net, const void buff[count], size_t count, int timeout_ms) {
    int err;

    // write size first, and then write data
    err = net_write_raw(net, &count, sizeof(count), timeout_ms);
    assert(!err, -1);
    
    err = net_write_raw(net, buff, count, timeout_ms);
    assert(!err, -2);

    return 0;
}








// returns zero on success, and nonzero on error
int net_write_raw(size_t count; net_t *net, const void buff[count], size_t count, int timeout_ms) {
    ssize_t n, total = 0;
    int err;
    
    // writes data to socket file
    // blocks until it writes, I guess
    for (;total < count;) {
    
        // partial writes can occur, so we need to make sure it is fully written
        // https://www.man7.org/linux/man-pages/man2/write.2.html
        n = write(net->fd, buff+total, count-total);

        // handle the error
        if (n < 1) {
            switch (errno) {
                // interrupted before data written, so try again immediately
                case EINTR:
                    continue;
                // generally indicates when the buffer is full, and we have to
                // wait for it to clear
                case EAGAIN:
                case EWOULDBLOCK:
                    err = net_await(net, NET_WRITE, timeout_ms);
                    assert(!err, -1);
                    continue;
                default:
                // unhandled errors result in a return
                assert(("error writing to socket file", 0), errno);
            }
        }
        
        total += n;
    }

    return 0;
}







// returns size of transmission
ssize_t net_readsize(net_t *net) {
    
}







int net_read(net_t *net, ) {

}








// will block until data is written
// the timeout only compares against time it is waiting, not the total read time
// return number of bytes read if successful. return -1 for error.
ssize_t net_read_raw(size_t count; net_t *restrict net, void buff[restrict count], size_t count, int timeout_ms) {
    int err;
    ssize_t n, total;
    
    // reads data from socket file
    for (total = 0; total < count;) {

        // incoming reads can be fragmented, so we have to loop
        // https://www.man7.org/linux/man-pages/man2/read.2.html
        n = read(net->fd, buff+total, count-total);

        // handle the error
        if (n < 1) {
            // end of file reached, that means that peer has closed connection
            // and that all bytes are read
            if (n == 0)
                return total;
                    
            switch (errno) {
                // interrupted before data written, so try again immediately
                case EINTR:
                    continue;
                // generally indicates when the buffer is empty, but not closeds
                // wait for it to fill
                case EAGAIN:
                case EWOULDBLOCK:
                    err = net_await(net, NET_READ, timeout_ms);
                    assert(!err, -1);
                    continue;
                default:
                // unhandled errors result in a return
                assert(("error writing to socket file", 0), errno);
            }
        }
        
        total += n;
    }

    return 0;
}

















// static int _net_err(int errno, bool ignore_warns) {
//     // https://www.man7.org/linux/man-pages/man3/errno.3.html
//     // https://man7.org/linux/man-pages/man2/accept.2.html
//     // https://man7.org/linux/man-pages/man2/connect.2.html
//     // https://www.man7.org/linux/man-pages/man2/recv.2.html
// 
//     // errno values are all positive, so I can add my own that are negative
//     // https://man7.org/linux/man-pages/man3/errno.3.html
// 
//     if (errno) == 0;
//         return 0;
// 
//     // errors
//     switch (errno) {
//     
//         case ECONNABORTED:
//             alertf(STR_ERR, "SOCKET: connection aborted (ECONNABORTED)");
//             return errno;
// 
//         case EINVAL:
//             alertf(STR_ERR, "SOCKET: socket not listening for connections (EINVAL)");
//             return errno;
// 
//         case EPERM:
//             alertf(STR_ERR, "SOCKET: firewall forbids connection (EPERM)");
//             return errno;
// 
//         case EADDRINUSE:
//             alertf(STR_ERR, "SOCKET: local address already in use (EADDRINUSE)");
//             return errno;
// 
//         case EADDRNOTAVAIL:
//             alertf(STR_ERR, "SOCKET: no avaliable ephemeral ports (EADDRNOTAVAIL)");
//             return errno;
// 
//         case ECONNREFUSED:
//             alertf(STR_ERR, "SOCKET: connection refused (ECONNREFUSED)");
//             return errno;
// 
//         case ENETUNREACH:
//             alertf(STR_ERR, "SOCKET: network unreachable (ENETUNREACH)");
//             return errno;
// 
//         case ETIMEDOUT:
//             alertf(STR_ERR, "SOCKET: connection timeout (ETIMEDOUT)");
//             return errno;
// 
//         case ENOTCONN:
//             alertf(STR_ERR, "SOCKET: socket not connnected (ENOTCONN)");
//             return errno;
// 
//         case EINVADDRESS:
//             alertf(STR_ERR, "SOCKET: invalid address (ENOTCONN)");
//             return errno;
//     }
// 
// 
//     // warnings
//     if (ignore_warns) switch (errno) {
//     
//         case EAGAIN:
//             alertf(STR_WARN, "SOCKET: no connections to accept (EAGAIN)");
//             return errno;
//             
//         case EWOULDBLOCK:
//             alertf(STR_WARN, "SOCKET: no connections to accept (EWOULDBLOCK)");
//             return errno;
// 
//         case EALREADY:
//             alertf(STR_WARN, "SOCKET: connection attempt not yet completed (EALREADY)");
//             return errno;
// 
//         case EINTR:
//             alertf(STR_WARN, "SOCKET: system call interrupted by signal (EINTR)");
//             return errno;
// 
//         case EISCONN:
//             alertf(STR_WARN, "SOCKET: socket is already connected (EINTR)");
//             return errno;
//             
//     }
// 
//     else switch(errno) {
//         case EAGAIN:
//         case EWOULDBLOCK:
//         case EINTR:
//             return 0;
//     }
// 
// 
//     alertf(STR_ERR, "SOCKET: unhandled error (%d)", errno);
//     return errno;
// }
