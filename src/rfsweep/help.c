
#include "rfsweep.h"
// Keeping this under rfsweep.h prevents errors caused by macro redefinitions
#include "toolkit/macros.h"



/*          8      16      24      32      40      48      56      64
     xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-   */

const char *str_help =
    "USAGE:"                                                            "\n"
    "        rfsweep -h"                                                "\n"
    "        rfsweep --defaults"                                        "\n"
    "        rfsweep <command> -h"                                      "\n"
    "        rfsweep <command> [options]"                               "\n"
    ""                                                                  "\n"
    "DESCRIPTION"                                                       "\n"
    "        Server/client command line interface for the RF Antenna"   "\n"
    "        Capstone project."                                         "\n"
    ""                                                                  "\n"
    "COMMANDS:"                                                         "\n"
    "        test        Run tests (debug build only)."                 "\n"
    "        ping        Ping server."                                  "\n"
    "        server      Start server."                                 "\n"
    "        reset       Reset remote server."                          "\n"
    "        restart     Restart remote server system."                 "\n"
    "        getlogs     Return remote server error logs."              "\n"
    "        transmit    Enable/disable remote server transmitter."     "\n"
    "        measure     Run remote server measurements."               "\n"
    "        receive     Run measurements without rotation."            "\n"
    "        rotate      Move motor to indicated angle."                "\n"
    ""                                                                  "\n"
    "OPTIONS"                                                           "\n"
    "        -h"                                                        "\n"
    "            Print help message."                                   "\n"
    ""                                                                  "\n"
    "        <command> -h"                                              "\n"
    "            Print command help message."                           "\n"
    ""                                                                  "\n"
    "        --defaults"                                                "\n"
    "            Print out all default values of relevant flags."       "\n";




/*          8      16      24      32      40      48      56      64
     xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-   */

const char *str_help_server =
    "USAGE:"                                                            "\n"
    "        rfsweep server -h"                                         "\n"
    "        rfsweep server [options]"                                  "\n"
    ""                                                                  "\n"
    "DESCRIPTION"                                                       "\n"
    "        Start server."                                             "\n"
    ""                                                                  "\n"
    "OPTIONS"                                                           "\n"
    "        -h"                                                        "\n"
    "            Print help message."                                   "\n"
    ""                                                                  "\n"
    "        -v"                                                        "\n"
    "            Verbose. Echos parameters."                            "\n"
    ""                                                                  "\n"
    "        --log=<filepath>"                                          "\n"
    "            Path to log file."                                     "\n"
    ""                                                                  "\n"
    "        --port=<port>"                                             "\n"
    "            Port to listen to."                                    "\n"
    ""                                                                  "\n"
    "        --tserial=<serial>"                                        "\n"
    "            Transmission HackRF serial string."                    "\n"
    ""                                                                  "\n"
    "        --rserial=<port>"                                          "\n"
    "            Receiver HackRF serial string."                        "\n";




/*          8      16      24      32      40      48      56      64
     xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-   */

const char *str_help_misc =
    "USAGE:"                                                            "\n"
    "        rfsweep <command> -h"                                      "\n"
    "        rfsweep reset   [options]"                                 "\n"
    "        rfsweep restart [options]"                                 "\n"
    "        rfsweep getlogs [options]"                                 "\n"
    ""                                                                  "\n"
    "DESCRIPTIONS:"                                                     "\n"
    "        reset       Reset remote server."                          "\n"
    "        restart     Restart remote server system."                 "\n"
    "        getlogs     Return remote server error logs."              "\n"
    "        ping        Ping server."                                  "\n"
    ""                                                                  "\n"
    "OPTIONS"                                                           "\n"
    "        -h"                                                        "\n"
    "            Print help message."                                   "\n"
    ""                                                                  "\n"
    "        -v"                                                        "\n"
    "            Verbose. Echos parameters."                            "\n"
    ""                                                                  "\n"
    "        --ip=<ip>"                                                 "\n"
    "            Server ip address."                                    "\n"
    ""                                                                  "\n"
    "        --port=<port>"                                             "\n"
    "            Server port number."                                   "\n";




/*          8      16      24      32      40      48      56      64
     xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-   */

const char *str_help_transmit =
    "USAGE:"                                                            "\n"
    "        rfsweep transmit enable  [options]"                        "\n"
    "        rfsweep transmit disable [options]"                        "\n"
    ""                                                                  "\n"
    "DESCRIPTION"                                                       "\n"
    "        Enables or disables the transmitting HackRF controlled"    "\n"
    "        by the server. Emits a constant frequency across the"      "\n"
    "        specified bandwidth."                                      "\n"
    ""                                                                  "\n"
    "OPTIONS"                                                           "\n"
    "        -h"                                                        "\n"
    "            Print help message."                                   "\n"
    ""                                                                  "\n"
    "        -v"                                                        "\n"
    "            Verbose. Echos parameters."                            "\n"
    ""                                                                  "\n"
    "        --ip=<ip>"                                                 "\n"
    "            Server ip address."                                    "\n"
    ""                                                                  "\n"
    "        --port=<port>"                                             "\n"
    "            Server port."                                          "\n"
    ""                                                                  "\n"
    "HACKRF OPTIONS"                                                    "\n"
    "        --freq=<freq_hz>"                                          "\n"
    "            Set center frequency (hz)."                            "\n"
    ""                                                                  "\n"
    "        --vga-gain=<gain_db>"                                      "\n"
    "            Variable gain amplifier. Software ampllification of"   "\n"
    "            the signal."                                           "\n"
    "            Steps of 1 dB. Ranges from 0 to 47 dB"                 "\n"
    ""                                                                  "\n"
    "        --tx-amp=<amplitude>"                                      "\n"
    "            Amplitude of transmission relative to gain (0-127)"    "\n"
    ""                                                                  "\n"
    "        --amplify"                                                 "\n"
    "            Enable amplifier."                                     "\n";





/*          8      16      24      32      40      48      56      64
     xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-xxxxxxx-   */

const char *str_help_measure =
    "USAGE:"                                                            "\n"
    "        rfsweep measure [options]"                                 "\n"
    "        rfsweep receive [options]"                                 "\n"
    ""                                                                  "\n"
    "DESCRIPTION"                                                       "\n"
    "        Request to server to run measurements, returning them"     "\n"
    "        to stdout or to a specified file."                         "\n"
    ""                                                                  "\n"
    "        Data is formatted like so:"                                "\n"
    "        <timestamp_micro:f64> <angle:f64> <freq_hz:f64>"           "\n"
    "        <band_hz:f64> <samplerate_hz:f64> <bytecount:i64> ["       "\n"
    "        <real:i8> <imaginary:i8> ...]"                             "\n"
    ""                                                                  "\n"
    "OPTIONS"                                                           "\n"
    "        -h"                                                        "\n"
    "            Print help message."                                   "\n"
    ""                                                                  "\n"
    "        -v"                                                        "\n"
    "            Verbose. Echos parameters."                            "\n"
    ""                                                                  "\n"
    "        --ip=<ip>"                                                 "\n"
    "            Server ip address."                                    "\n"
    ""                                                                  "\n"
    "        --port=<port>"                                             "\n"
    "            Server port."                                          "\n"
    ""                                                                  "\n"
    "        --file=<filepath>"                                         "\n"
    "            Output server response to file rather than stdout."    "\n"
    ""                                                                  "\n"
    "HACKRF OPTIONS"                                                    "\n"
    "        --freq=<freq_hz>"                                          "\n"
    "            Set center frequency (Hz)."                            "\n"
    ""                                                                  "\n"
    "        --band=<band_hz>"                                          "\n"
    "            Set baseband filter bandwidth (Hz). Will round the"    "\n"
    "            frequency to the nearest supported frequency."         "\n"
    ""                                                                  "\n"
    "        --srate=<srate_hz>"                                        "\n"
    "            Set sample rate (Hz)."                                 "\n"
    ""                                                                  "\n"
    "        --lna-gain=<gain_db>"                                      "\n"
    "            Low noise amplifier. Hardware amplification of small"  "\n"
    "            signals. Steps of 8 dB. Ranges from 0 to 40 dB"        "\n"
    ""                                                                  "\n"
    "        --vga-gain=<gain_db>"                                      "\n"
    "            Variable gain amplifier. Software ampllification of"   "\n"
    "            the signal to adjust 8-bit saturation"                 "\n"
    "            Steps of 2 dB. Ranges from 0 to 62 dB"                 "\n"
    ""                                                                  "\n"
    "        --steps=<steps>"                                           "\n"
    "            Number of angles to take samples (divided across"      "\n"
    "            360 degrees."                                          "\n"
    ""                                                                  "\n"
    "        --samps=<samples>"                                         "\n"
    "            Number of chunk samples to take per angle. (chunks"    "\n"
    "            are usually around 65,536 interleaved bytes of data"   "\n"
    "            each)"                                                 "\n"
    ""                                                                  "\n"
    "        --stepmode=<1/2/4/8/16>"                                   "\n"
    "            Set motor microstep resolution."                       "\n"
    // ""                                                                  "\n"
    // "        --snap=<snap_pow2>"                                        "\n"
    // "            Power of 2 exponent to snap the step size to. From"    "\n"
    // "            0 to 4"                                                "\n"
    ""                                                                  "\n"
    "        --amplify"                                                 "\n"
    "            Enable amplifier."                                     "\n"
    ""                                                                  "\n"
    "        --binary"                                                  "\n"
    "            Output results in binary instead of ascii."            "\n";




const char *str_help_rotate =
    "USAGE:"                                                            "\n"
    "        rfsweep rotate [options]"                                  "\n"
    ""                                                                  "\n"
    "DESCRIPTION"                                                       "\n"
    "        Move motor to indicated angle."                            "\n"
    ""                                                                  "\n"
    "OPTIONS"                                                           "\n"
    "        -h"                                                        "\n"
    "            Print help message."                                   "\n"
    ""                                                                  "\n"
    "        -v"                                                        "\n"
    "            Verbose. Echos parameters."                            "\n"
    ""                                                                  "\n"
    "        --angle=<degrees>"                                         "\n"
    "            Set motor angle in terms of degrees"                   "\n"
    ""                                                                  "\n"
    "        --steps=<degrees>"                                         "\n"
    "            Set motor angle in terms of steps"                     "\n"
    ""                                                                  "\n"
    "        --stepmode=<1/2/4/8/16>"                                   "\n"
    "            Set motor microstep resolution."                       "\n";
    


//const char *str_help_receive = "Help string for \"receive\" not written yet\n";





// TODO: update this
const char *str_help_defaults =
    "DEFAULTS"                                            "\n"
    "        --ip="         STRINGIFY(DEFAULT_IP)         "\n"
    "        --port="       STRINGIFY(DEFAULT_PORT)       "\n"
    "        --rserial="    STRINGIFY(DEFAULT_RSERIAL)    "\n"
    "        --tserial="    STRINGIFY(DEFAULT_TSERIAL)    "\n"
    "        --samps="      STRINGIFY(DEFAULT_SAMPS)      "\n"
    // "        --snap="       STRINGIFY(DEFAULT_SNAP)       "\n"
    "        --steps="      STRINGIFY(DEFAULT_STEPS)      "\n"
    "        --freq="       STRINGIFY(DEFAULT_FREQ)       "\n"
    "        --band="       STRINGIFY(DEFAULT_BAND)       "\n"
    "        --srate="      STRINGIFY(DEFAULT_SRATE)      "\n"
    "        --lna-gain="   STRINGIFY(DEFAULT_LNA)        "\n"
    "        --vga-gain="   STRINGIFY(DEFAULT_VGA)        "\n"
    "        --tx-amp="     STRINGIFY(DEFAULT_TX_AMP)     "\n"
    "        --stepmode="   STRINGIFY(DEFAULT_STEPMODE)   "\n";



