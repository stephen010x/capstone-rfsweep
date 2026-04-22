
https://pysdr.org/content/hackrf.html

For gdb
```sh
export ASAN_OPTIONS=detect_leaks=0
```

### How to Use:

```
USAGE:
        rfsweep -h
        rfsweep --defaults
        rfsweep <command> -h
        rfsweep <command> [options]

DESCRIPTION
        Server/client command line interface for the RF Antenna
        Capstone project.

COMMANDS:
        test        Run tests (debug build only).
        ping        Ping server.
        server      Start server.
        reset       Reset remote server.
        restart     Restart remote server system.
        getlogs     Return remote server error logs.
        transmit    Enable/disable remote server transmitter.
        measure     Run remote server measurements.
        angle       Move motor to indicated angle
        receive     Run measurements without rotation.

OPTIONS
        -h
            Print help message.

        -v
            Verbose. Echos parameters.

        <command> -h
            Print command help message.

        --log=<filepath>
            Path to log file.

        --file=<filepath>
            Output server response to file rather than stdout.

        --port=<port>
            Port to listen to.

        --defaults
            Print out all default values of relevant flags.

        --ip=<ip>
            Server ip address.

        --port=<port>
            Server port number.

        --tserial=<serial>
            Transmission HackRF serial string.

        --rserial=<port>
            Receiver HackRF serial string.

        --angle=<degrees>
            Set motor angle in terms of degrees

        --step=<degrees>
            Set motor angle in terms of steps

        --stepmode=<1/2/4/8/16>
            Set motor microstep resolution.

HACKRF OPTIONS

        --freq=<freq_hz>
            Set center frequency (Hz).

        --band=<band_hz>
            Set bandwidth (Hz). Will round the frequency to the
            nearest supported frequency.

        --srate=<srate_hz>
            Set sample rate (Hz).

        --lna-gain=<gain_db>
            Set lna-gain. Steps of 8 dB. Ranges from 0 to 40 dB

        --vga-gain=<gain_db>
            Set lna-gain. Steps of 8 dB. Ranges from 0 to 40 dB

        --samps=<samples>
            Number of chunk samples to take. (chunks are usually
            around 32,768 bins each)

        --snap=<snap_pow2>
            Power of 2 exponent to snap the step size to. From
            0 to 4

        --amplify
            Enable amplifier.
```
