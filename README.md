# Open Source RF Measurement System

A senior capstone project by Stephen Harris and Jared Bell.

Sponsored by Utah Tech University.




## TODO:
These are the things that need to get done for this makefile
- [ ] (optional) add assembly and print instructions
- [ ] Link to cad readme or files
- [ ] Add images, findings, etc etc
- [ ] Add table of contents



Mention the goal of the project, and that the command line tool is only meant to collect data, while you can use whatever you want to process the data. But that a python file is provided by default. (also mention data format)





## Script Dependancies (optional)

To use the scripts, you must install the script dependancies

### Debian Script Dependancies (optional)

```bash
# Update apt
sudo apt update

# Install python3 and pip
sudo apt-get install python3 pip --no-install-recommends

# Install python dependancies
sudo apt-get install python3-numpy python3-scipy python3-matplotlib --no-install-recommends
```

<!-- pip install numpy scipy matplotlib --break-system-packages -->

### Windows CMD Script Dependancies (optional)

```cmd
# Install python3
winget install Python.Python.3.14

# Install neccessary python libs
pip install numpy scipy matplotlib
```

### Windows Desktop Script Dependancies (optional)

1. Install python from https://www.python.org/downloads/
2. Install neccessary python libs `pip install numpy scipy matplotlib`







## How to Use

`rfsweep` is a command line tool with various options that allows the user to wirelessly send commands to the Raspberry PI controller that manages the motor and antennas. The communication is server/client based over TCP. Both the client and the server must be connected to Wi-Fi.

Packaged alongside the `rfsweep` command are several interactive scripts to allow for less technical and more convenient use of `rfsweep`.


### Connecting to the Rasperry PI controller

Communication is managed wirelessly or through ethernet. To communicate, both the client and server must have some way to communicate. The connection can either be wireless, or through ethernet. In order for communication to happen, the client has to know the IP address of the server.

#### From the PI's Access Point (intended)

The Raspberry PI is configured to broadcast a wireless access point that you can connect to called `raspi`. On the prototype, connecting to the `raspi` network requires no password.

Note that you may **loose connection to the internet** when connecting to `raspi`. Generally it is an accepted risk to loose internet access while taking measurements. However, if you want internet access while connected to `raspi`, you can enable that by connecting the Raspberry PI controller to the internet by plugging in a USB Wi-Fi dongle to the USB hub connected to the Raspberry PI, and then accessing the PI through either `ssh` (See (connecting to the PI through SSH)[ssh-into-the-raspberry-pi-controller]) or through other means. And then connecting to Wi-Fi from a network utility such as `nmtui`.

#### From localhost

If the server is running from the same computer as the client, you can connect them via the localhost IP address `127.0.0.1`.

#### From a local connection

If both the client and the server are connected to the same local network, you can connect to the PI using it's local IP address.

#### From a public connection

Connecting to the Raspberry PI through a public connection can be more tricky. First, you have to know the public IP address of the router that the PI is connected to. Next, you have to make sure the port (default is port 7070) the server is using is exposed on the router. (The default on most routers is to firewall all ports).


### SSH into the Raspberry PI Controller

The Raspberry PI is a headless controller. As a result, the easiest and most convenient way to access it is through SSH. Through SSH, you are able to access the PI directly through the command line.

By default, the PI's wireless access point IP address is `10.42.0.1`. To SSH into the PI make sure you are connected to the `raspi` network broadcasted by the PI. (If you have only just turned on the PI, wait about 60 seconds for the device to boot up and configure the network). Then you can run `ssh user@10.42.0.1:/home/user` with password `pass`.

For the prototype, both the Raspberry PI's root and user password has been set to `pass`.
In the case of the PI's Access Point or Wi-Fi connection malfunctioning, you are able to take out it's SD memory card, and edit the `startup.sh` script in `/home/user/`. The startup script in the home user directory is ran whenever the PI boots up, and requires some technical competency in GNU/Linux command line and Bash scripting. This script is responsible for setting up Wi-Fi and autostarting the `rfsweep server`.

If you wish to modify the Wi-Fi startup script, deleting the `key/` directory in the user home directory will trigger the Wi-Fi script to reconfigure the Access Point.


### Using the interactive and fast scripts

See (Script Dependancies)[script-dependancies-optional] for how to install the dependancies required to run these scripts.

Provided with the `rfsweep` binary are four scripts. `run.bat`, `fastrun.bat`, `transmit.bat`, and `process.py` for Windows. Or `run.sh`, `fastrun.sh`, and `process.py` for Linux.

For the interactive transmitter and receiver scripts: If the IP address is left blank, these scripts will test for the known default access point address, followed by the known default ethernet address, followed by the localhost address.


### Interactive Transmitter Script

`transmit.bat` is an interactive script that will prompt you for the transmitter parameters. See (`rfsweep` Parameters)[rfsweep-parameters] for more information on the parameters.

Killing or closing the transmit script will stop the transmission.

For instance:

```


RF ANTENNA TRANSMIT TOOL
========================

For the following options, enter a value and press enter.
(leave blank and hit enter for default value)

Connection Options
------------------
Enter Controller IP [multiple]:
Enter Controller Port [7070]:

HackRF Options
--------------
Enter Center Freq (Hz) [2.4e9]: 5.1e9
Enable Amplifier? (y/n) [y]: y
Enter VGA-Gain (0-47 dB) [20]: 40
Enter Transmitting Amplitude (0-127) [127]:
Enable Transmitter Clock Out? (y/n) [y]: n

Press Enter to run test...

rfsweep transmit enable --ip=10.42.0.1 --port=7070 --freq=5.1e9 --vga-gain=40 --tx-ampl=127 --amplify

Press Enter or close window to stop transmitter...
rfsweep transmit disable --ip=10.42.0.1 --port=7070
```


### Interactive Receiver Script

`run.bat` and `run.sh` act as an interactive script that will prompt you for the receiver parameters. See (`rfsweep` Parameters)[rfsweep-parameters] for more information on the parameters.

Once the script ends, it will run `process.py`, which will process and graph the data received.

For instance:

```

RF ANTENNA MEASURE TOOL
=======================

For the following options, enter a value and press enter.
(leave blank and hit enter for default value)

Connection Options
------------------
Enter Controller IP [multiple]:
Enter Controller Port [7070]:

HackRF Options
--------------
Enter Center Freq (Hz) [2.4e9]: 5.1e9
Enter Sample Rate (Hz) [10e6]:
Enter Band Filter (Hz) [srate*0.75]:
Enter LNA-Gain (0-40 dB) [16]: 20
Rounding LNA-Gain to 24 dB.
Enter VGA-Gain (0-62 dB) [20]: 16
Enable Amplifier? (y/n) [n]: y

Data Options
------------
Enter Total Sample Steps [360]:
Enter Samples per Step [1]: 5
Enter Microstep Mode (1/2/4/8/16) [1]:

Press Enter to run test...

rfsweep measure --ip=10.42.0.1 --port=7070 --steps=360 --samps=5 --stepmode=1 --file=data\\data-1777676846.bin --freq=5.099e+09 --band= --srate=10e6 --lna-gain=24 --vga-gain=16 --samps=5 --binary --amplify && python process.py --freq 5.1e9 data\\data-1777676846.bin
```

### Fastrun Script

The `fastrun.bat` and `fastrun.sh` scripts are non-interactive scripts that are meant to be edited. Inside they have variables that you can modify to allow for a quick, easy, and repeatable way to run tests without having to use the interactive scripts.

These scripts manage both the transmitter and receiver. When finished it runs `process.py`, which will process and graph the data received.

These are the variables intended to be modified within the script:

```bat
:: fastrun.bat

:: -----------------
:: Connection Settings
:: It will try all three of these IPs
SET "ipA=10.42.0.1" & REM - Wifi IP
SET "ipB=10.42.0.1" & REM - Ethernet IP
SET "ipC=127.0.0.1" & REM - Localhost IP
SET "port=7070"

:: -----------------
:: Hackrf Settings
SET "freq=2.4e9"    & REM - center frequency
SET "srate=10e6"    & REM - sample rate (Hz)
SET "band="         & REM - bandpass filter width. Leave blank to set to 75% of srate
SET "lna_gain=16"   & REM - LNA Gain (amplifies small signals) (0-40 dB, step of 8 dB)
SET "vga_gain=20"   & REM - VGA Gain (software amplifier) (0-62 dB, step of 2 dB)
SET "is_amp=false"  & REM - enable amplifier
SET "steps=360"     & REM - how many angles to take samples
SET "samps=1"       & REM - how many samples to take at each angle
SET "stepmode=1"    & REM - microstep mode (1/2/4/8/16)

:: -----------------
:: Transmitter Settings
SET "tx_enable=true"    & REM - enable transmitter?
SET "tx_vga_gain=20"    & REM - TX VGA Gain (0-47 dB, steps of 1 dB)
SET "tx_ampl=127"       & REM - TX signal amplitude (0-127)
SET "tx_is_amp=true"    & REM - enable TX amplifier?
SET "tx_clock=true"     & REM - enable TX clock out?

:: -----------------
:: Output Settings
SET "outdir=data\"      & REM - output directory
SET "extflags=--binary" & REM - extra flags
```


### How to use `process.py`

`process.py` is a python script packaged with the binaries by default. This script is an optional companion script to the `rfsweep` command, and provides for an easy way to process and visually plot the data received by running `rfsweep measure` or `rfsweep receive`. See (How to use `rfsweep`)[how-to-use-rfsweep] for more information on how to use `rfsweep`.

The `process.py` help manual is available by running:

```cmd
process.py -h
```

To use `process.py`, append one or more file names for it to process. Files can be in binary format, as well as text format. `process.py` detects these formats via file extensions such as `.bin` and `.txt`. See (`rfsweep` Data Formats)[rfsweep-data-formats] for more information on the accepted formats.

Example usage:

```cmd
process.py data1.bin data2.txt
```

`process.py` is able to automatically determine the bandpass filter frequency by looking for the peak signal across all of the data points. However, this may be unreliable with messier signals. If you wish to manually specify a filter frequency, use the `--freq <value>` flag.

```cmd
process.py data.bin --freq 2.41e9
```


### How to use `rfsweep`

`rfsweep` is the core command line executable that allows us to communicate with the server, (as well as acting as the core executable running the server on the Raspbery PI controller).

Within the command line, you can get a help manual by running:

```cmd
rfsweep -h
```

See (`rfsweep` Commands)[rfsweep-commands] or (`rfsweep` Parameters)[rfsweep-parameters] for more detailed usage.






---






## `rfsweep` Commands

`rfsweep` has several commands that can be sent to the server. You can view these commands by running:

```cmd
rfsweep -h
```

Each command also has their own help manual:

```cmd
rfsweep <command> -h
```

The commands are as listed below:

```
COMMANDS:
        ping        Ping server.
        server      Start server.
        reset       Reset remote server.
        restart     Restart remote server system.
        getlogs     Return remote server error logs.
        transmit    Enable/disable remote server transmitter.
        measure     Run remote server measurements.
        receive     Run measurements without rotation.
        rotate      Move motor to indicated angle.

USAGE:
        rfsweep -h
        rfsweep <command> -h
        rfsweep <command> [options]
```



## `rfsweep` Parameters

Parameters are flags that typically follow after a command, that allows us to configure the behavior when the server receives the command. They typically follow the format `--<param>=<value>`

For instance, 

```cmd
rfsweep rotate --angle=360
```

All parameters have a default value. All You can query for the default parameters by running:

```cmd
rfsweep --defaults
```

Note that leaving parameters blank will result in them using their default value. Like so:

```cmd
rfsweep ping --ip= --port=
```

Most parameters are specific to certain commands, as will be explained in the following subsections.


### General Parameters

These are parameters that apply to every command except the `server` command.

```
OPTIONS
        -h
            Print help message.

        -v
            Verbose. Echos parameters.

        --ip=<ip>
            Server ip address.

        --port=<port>
            Server port number.

DEFAULTS:
        --ip=10.42.0.1
        --port=7070

USAGE:
        rfsweep <command> -h
        rfsweep <command> [options]
```

#### NOTES:

The `-v` flag prints out the parameters being used. This tells us exactly what parameters is being sent to the server, and is useful for detecting whether or not we have passed `rfsweep` bad parameters.

The `--ip` and `--port` flags set the IP address and PORT that points to the server. If these are wrong, then the client will fail to send any messages to the server.


### `rfsweep ping` Parameters

The `ping` command sends a probing signal to the server to determine if the server is reachable. If this command fails, it means that either A) The server is inactive, or B) The server is unreachable, which typically occurs when the server host computer or controller is off, or if your IP or PORT are wrong.

If `ping` fails, check that the Raspberry PI is on, that you are connected to the `raspi` Wi-Fi SSID or through an Ethernet connection, and finally make sure that your IP address and PORT are correct.

See (General Parameters)[general-parameters] for `rfsweep ping` parameters.


### `rfsweep server` Parameters

The `server` command is a special mode that behaves unlike any of the other commands. The other commands are clientside commands being sent to the server. Running `rfsweep server` will start a server instance that is able to receive any commands sent by `rfsweep` client commands.

The prototype is configured to autostart this on the Raspberry PI.

```
DESCRIPTION
        Start server.

OPTIONS
        -h
            Print help message.

        -v
            Verbose. Echos parameters.

        --log=<filepath>
            Path to log file.

        --port=<port>
            Port to listen to.

        --tserial=<serial>
            Transmission HackRF serial string.

        --rserial=<port>
            Receiver HackRF serial string.

DEFAULTS:
        --port=7070
        --log=NULL
        --rserial="0000000000000000d07864dc314d3287"
        --tserial="0000000000000000930c64dc285b0ec3"

USAGE:
        rfsweep server -h
        rfsweep server [options]
```

#### NOTES:

The `--log` flag sets the path to the log file that the server will return when the client calls `rfsweep getlogs`.

The `--tserial` and `--rserial` flags are the serial number strings for identifying the transmitter HackRF, and the receiver HackRF. These serial numbers are unique to each HackRF, and are how the server indentifies which HackRF is the receiver, and which is the transmitter. As a consiquence, using different HackRFs will require you to obtain the serial number, and specifying it in the flags. You can obtain the serial number by running `hackrf_info` from the HackRF Tools library, which can be found in the `hackrf-tools\` directory of the Windows build. Otherwise `hackrf_info` can be obtained through Debian `apt` or by following the instructions on the HackRF Docs: https://hackrf.readthedocs.io/en/latest/installing_hackrf_software.html.


### `rfsweep reset` Parameters

The `rfsweep reset` command will tell the server to restart it's running process. If the `startup.sh` is not running `rfsweep server` on loop, then it will not restart, and will only kill the process.

See (General Parameters)[general-parameters] for `rfsweep reset` parameters.


### `rfsweep restart` Parameters

The `rfsweep restart` command will tell the server to reboot the computer that is running on, which typically should be the Rasberry PI controller.

See (General Parameters)[general-parameters] for `rfsweep restart` parameters.


### `rfsweep getlogs` Parameters

The `rfsweep getlogs` command will request the error logs from the server, and print it to stdout. 

See (General Parameters)[general-parameters] for `rfsweep getlogs` parameters.


### `rfsweep transmit` Parameters

The `rfsweep transmit enable` and `rfsweep transmit disable` command allows you to enable or disable the transmitter. When enabled, the transmitter will generate a sinusoidal wave on the specified frequency.

```
OPTIONS
        -h
            Print help message.

        -v
            Verbose. Echos parameters.

        --ip=<ip>
            Server ip address.

        --port=<port>
            Server port.

HACKRF OPTIONS
        --freq=<freq_hz>
            Set center frequency (hz).

        --vga-gain=<gain_db>
            Variable gain amplifier. Software ampllification of
            the signal.
            Steps of 1 dB. Ranges from 0 to 47 dB

        --tx-ampl=<amplitude>
            Amplitude of transmission relative to gain (0-127)

        --amplify
            Enable amplifier.

        --clock
            Enable transmitter clock out enable.

DEFAULTS
        --ip=10.42.0.1
        --port=7070
        --freq=2.4e9
        --vga-gain=20
        --tx-amp=127

USAGE:
        rfsweep transmit enable  [options]
        rfsweep transmit disable [options]
```

### `rfsweep measure` and `rfsweep receive` Parameters

The `rfsweep measure` and `rfsweep receive` are the core commands of `rfsweep` that allows us to record measurements to later be processed.

These commands are nearly identical. They both request the server to perform measurements, and return the data back to the client. The only difference between the two is that `rfsweep measure` will take measurements while rotating, whereas `rfsweep receive` will take measurements while remaining stationary.

```
OPTIONS
        -h
            Print help message.

        -v
            Verbose. Echos parameters.

        --ip=<ip>
            Server ip address.

        --port=<port>
            Server port.

        --file=<filepath>
            Output server response to file rather than stdout.

HACKRF OPTIONS
        --freq=<freq_hz>
            Set center frequency (Hz).

        --band=<band_hz>
            Set baseband filter bandwidth (Hz). Will round the
            frequency to the nearest supported frequency.

        --srate=<srate_hz>
            Set sample rate (Hz).

        --lna-gain=<gain_db>
            Low noise amplifier. Hardware amplification of small
            signals. Steps of 8 dB. Ranges from 0 to 40 dB

        --vga-gain=<gain_db>
            Variable gain amplifier. Software ampllification of
            the signal to adjust 8-bit saturation
            Steps of 2 dB. Ranges from 0 to 62 dB

        --steps=<steps>
            Number of angles to take samples (divided across
            360 degrees.

        --samps=<samples>
            Number of chunk samples to take per angle. (chunks
            are usually around 65,536 interleaved bytes of data
            each)

        --stepmode=<1/2/4/8/16>
            Set motor microstep resolution.

        --amplify
            Enable amplifier.

        --binary
            Output results in binary instead of ascii.

        --clock
            Enable receiver clock out enable.

DEFAULTS
        --ip=10.42.0.1
        --port=7070
        --samps=1
        --steps=360
        --freq=2.4e9
        --srate=10e6
        --band=<srate*0.75>
        --lna-gain=16
        --vga-gain=20
        --tx-amp=127
        --stepmode=1

USAGE:
        rfsweep measure [options]
        rfsweep receive [options]
```

#### NOTES:

If `--band=0`, then 


#### Data Formatting:
See something about data formatting


---




## `rfsweep` Data Formats





---





## Setting up the Raspberry PI





---





## How to compile `rfsweep`


### Compile for Debian/Ubuntu

Tested on Debian (Trixie) and Ubuntu (Noble Numbat)

```
# Update apt
sudo apt update

# Install dependancies
sudo apt-get install git gcc make libusb-1.0-0 libusb-1.0-0-dev

# Clone the project repository
git clone https://github.com/stephen010x/capstone-rfsweep.git --recurse-submodules
cd capstone-rfsweep

# If cloned without submodules, you can update the submodules like so:
git submodule init
git submodule update

# Build project
make

# Once complete, the compiled output can be found in 'bin/'
```

<!-- If you don't have git, you can install it like so: `sudo apt install git ca-certificates --no-install-recommends` -->


### Compile for Windows CMD

```cmd
# Open the windows command line

# Install git if not already installed
winget install Git.Git

# Clone the project repository
git clone https://github.com/stephen010x/capstone-rfsweep.git
cd capstone-rfsweep

# Install cygwin
call install_cygwin.bat

# Build project
make

# Once complete, the compiled output can be found in 'bin\'
```

### Compile for Windows Desktop
1. Download repository from https://github.com/stephen010x/capstone-rfsweep
2. Run `install_cygwin.bat` to install Cygwin.
3. From the CMD, run `make`
4. The binaries can found in `bin\`



### Cross-Compile for Windows from Debian/Ubuntu

```
# Update apt
sudo apt update

# Install wine
sudo apt-get install wine

# Clone the project repository
git clone https://github.com/stephen010x/capstone-rfsweep.git --recurse-submodules
cd capstone-rfsweep

# Install Cygwin
wine install_cygwin.bat

# Build project
./winebuild.sh

# Once complete, the compiled output can be found in 'bin/'
```




---





## Resources and Documentation

- https://pysdr.org/content/hackrf.html
- https://github.com/greatscottgadgets/hackrf/
- https://hackrf.readthedocs.io/en/latest/
- https://hackrf.readthedocs.io/en/latest/hackrf_one.html
- https://github.com/cygwin/cygwin/
- https://www.cygwin.com/
