# Open Source RF Measurement System

A senior capstone project by Stephen Harris and Jared Bell.

Sponsored by Utah Tech University.


https://pysdr.org/content/hackrf.html


For gdb
```sh

```





## How to Compile




### Compile for Debian/Ubuntu

Tested on Debian (Trixie) and Ubuntu (Noble Numbat)


1. Install dependancies:

`sudo apt update`

`sudo apt-get install git gcc make libusb-1.0-0 libusb-1.0-0-dev`


2. Clone this repository with git. `git clone --recurse-submodules https://github.com/stephen010x/capstone-rfsweep.git`

<!-- If you don't have git, you can install it like so: `sudo apt install git ca-certificates --no-install-recommends` -->

If cloned without submodules, you can update the submodules like so:

`git submodule init`

`git submodule update`


3. Recurse into directory `cd capstone-rfsweep`

4. Run `make`

Once complete, the compiled output can be found in `bin/`



### Compile for Windows

`winget install Git.Git`
`git clone https://github.com/stephen010x/capstone-rfsweep.git`
run `install_cygwin.bat` or from cmd: `call install_cygwin.bat`
`make`



### Cross-Compile for Windows from Debian/Ubuntu

1. Clone this repository. `git clone --recurse-submodules https://github.com/stephen010x/capstone-rfsweep.git`

2. Recurse into directory `cd capstone-rfsweep`

3. Install `wine` with `sudo apt-get install wine`

4. Install `cygwin` with `wine install_cygwin.bat`

5. To compile, run `./winebuild.sh`

Once complete, the compiled output can be found in `bin/`




## How to Use

### Install Script Dependancies (optional)

To use the scripts, you must install the script dependancies

#### For Debian

1. Install `python3` and `pip`

`sudo apt update`

`sudo apt-get install python3 pip --no-install-recommends`

2. Install python dependancies `numpy`, `scipy`, and `matplotlib`

`sudo apt-get install python3-numpy python3-scipy python3-matplotlib --no-install-recommends` 

or...

`pip install numpy scipy matplotlib --break-system-packages`


#### For Windows







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
