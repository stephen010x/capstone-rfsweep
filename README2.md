<!-- 
https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting
https://markdownlivepreview.com/

https://learnbyexample.github.io/customizing-pandoc/
-->



# Open Source RF Measurement System

A senior capstone project by Stephen Harris and Jared Bell.

Sponsored by Utah Tech University.



## Table of Contents
- [How to download](#how-to-download)
- [Connecting to the RFSWEEP server](#connecting-to-the-rfsweep-server)
    - [From the PI's Access Point (intended)](#from-the-pis-access-point)
    - [From `localhost`](#from-localhost)
    - [From a Local Connection](#from-a-local-connection)
    - [From a Public Connection](#from-a-public-connection)
    - [SSH into the Raspberry PI Controller](#ssh-into-the-raspberry-pi-controller)
- [How to Use Scripts](#how-to-use-scripts)
    - [Interactive Receiver Script](#interactive-receiver-script)
    - [Interactive Transmitter Script](#interactive-transmitter-script)
    - [Fastrun Script](#fastrun-script)
    - [How to use `process.py`](#how-to-use-processpy)
- [How to use `rfsweep`](#how-to-use-rfsweep)
    - [`rfsweep` Commands](#rfsweep-commands)
    - [`rfsweep` Parameters](#rfsweep-parameters)
    - [General Parameters](#general-parameters)
    - [`rfsweep ping` Parameters](#rfsweep-ping-parameters)
    - [`rfsweep server` Parameters](#rfsweep-server-parameters)
    - [`rfsweep reset` Parameters](#rfsweep-reset-parameters)
    - [`rfsweep restart` Parameters](#rfsweep-restart-parameters)
    - [`rfsweep getlogs` Parameters](#rfsweep-getlogs-parameters)
    - [`rfsweep transmit` Parameters](#rfsweep-transmit-parameters)
    - [`rfsweep measure` and `rfsweep receive` Parameters](#rfsweep-measure-and-rfsweep-receive-parameters)
- [`rfsweep` Data Formats](#rfsweep-data-formats)
- [Hardware Notices](#hardware-notices)
- [Cad Models](#cad-models)
- [Building `rfsweep`](#building-rfsweep)
    - [Compile for Debian/Ubuntu](#compile-for-debianubuntu)
    - [Compile on Windows CMD](#compile-on-windows-cmd)
    - [Compile on Windows Desktop](#compile-on-windows-desktop)
    - [Cross-Compile for Windows from Debian/Ubuntu](#cross-compile-for-windows-from-debianubuntu)
- [Installing Script Dependancies (optional)](#installing-script-dependancies-optional)
    - [Debian/Ubuntu Script Dependancies (optional)](#debianubuntu-script-depenancies-optional)
    - [Windows CMD Script Dependancies (optional)](#windows-cmd-script-dependancies-optional)
    - [Windows Desktop Script Dependancies (optional)](#windows-desktop-script-dependancies-optional)
- [Setting up the Raspberry PI](#setting-up-the-raspberry-pi)
- [Resources and Documentation](#resources-and-documentation)







# How to download

You can download the binaries and scripts neccessary for using `rfsweep` for both Windows and Debian/Ubuntu in the [releases](https://github.com/stephen010x/capstone-rfsweep/releases) page here.

<divrelesstyle="page-break-after:salways;"></div>

---









# Connecting to the RFSWEEP server

Communication is managed wirelessly or through ethernet. Both the client and server must have some way to connect to each-other. The connection can either be wireless, or through ethernet. In order for communication to happen, the client has to know the IP address of the server.


### From the PI's Access Point (intended)

- The Raspberry PI is configured to broadcast a wireless access point that you can connect to called raspi. On the prototype, connecting to the `raspi` network requires no password.

- Note that you may **__lose connection to the internet when connecting to the Raspberry PI__**. Generally it is an accepted risk to loose internet access while taking measurements. However, if you want internet access while connected to raspi, you can enable that by connecting the Raspberry PI controller to the internet by plugging in a USB Wi-Fi dongle to the USB hub connected to the Raspberry PI, and then accessing the PI through either `ssh` (See [SSH into the Raspberry PI Controller](#ssh-into-the-raspberry-pi-controller)) or through other means. And then connecting the PI to Wi-Fi from a network utility such as `nmtui`.


### From `localhost`

- If the server is running from the same computer as the client, you can connect them via the localhost IP address `127.0.0.1`.


### From a Local Connection

- If both the client and the server are connected to the same local network, you can connect to the PI using it's local IP address.


### From a Public Connection

- Connecting to the Raspberry PI through a public connection can be more tricky. First, you have to know the public IP address of the router that the PI is connected to. Next, you have to make sure the port (default is port `7070`) the server is using is exposed on the router. (The default on most routers is to firewall all ports).


## SSH into the Raspberry PI Controller

- The Raspberry PI is a headless controller. As a result, the easiest and most convenient way to access it is through SSH. Through SSH, you are able to access the PI directly through the command line.

- By default, the PI's wireless access point IP address is `10.42.0.1`. To SSH into the PI make sure you are connected to the raspi network broadcasted by the PI. (If you have only just turned on the PI, wait about 60 seconds for the device to boot up and configure the network). Then you can run `ssh user@10.42.0.1:/home/user` with password `pass`.

- For the prototype, both the Raspberry PI's `root` and `user` password has been set to `pass`.

- In the case of the PI's Access Point or Wi-Fi connection malfunctioning, you are able to take out it's SD memory card, and edit the `startup.sh` script in `/home/user/`. The startup script in the home user directory is ran whenever the PI boots up, and requires some technical competency in GNU/Linux command line and Bash scripting. This script is responsible for setting up Wi-Fi and autostarting the `rfsweep` server.

- If you wish to modify the Wi-Fi startup script, deleting the `key/` directory in the `user` home directory will trigger the Wi-Fi script to reconfigure the Access Point.



<divrelesstyle="page-break-after:salways;"></div>

---








# How to Use Scripts

See [Installing Script Dependancies (optional)](#installing-script-dependancies-optional) for how to install the dependancies required to run these scripts.

Provided with the rfsweep binary are four scripts. `run.bat`, `fastrun.bat`, `transmit.bat`, and `process.py` for Windows. Or `run.sh`, `fastrun.sh`, and `process.py` for Linux.

For the interactive transmitter and receiver scripts: If the IP address is left blank, these scripts will test for the known default access point address, followed by the known default ethernet address, followed by the localhost address.

Also provided is the `processing.py` script, which is the default script used to process and graph the data returned from `rfsweep`.


## Interactive Receiver Script

`run.bat` and `run.sh` act as an interactive script that will prompt you for the receiver parameters. See [`rfsweep measure` and `rfsweep receive` Parameters](#rfsweep-measure-and-rfsweep-receive-parameters) for more information on the measurement parameters.

Once the script ends, it will run process.py, which will process and graph the data received.


## Interactive Transmitter Script

`transmit.bat` is an interactive script that will prompt you for the transmitter parameters. See [`rfsweep transmit` Parameters](#rfsweep-transmit-parameters) for more information on the transmitter parameters.

<details>
<summary>For instance:</summary>
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
</details>


## Fastrun Script


## How to use `process.py`



<divrelesstyle="page-break-after:salways;"></div>

---








- [How to use `rfsweep`](#how-to-use-rfsweep)
    - [`rfsweep` Commands](#rfsweep-commands)
    - [`rfsweep` Parameters](#rfsweep-parameters)
    - [General Parameters](#general-parameters)
    - [`rfsweep ping` Parameters](#rfsweep-ping-parameters)
    - [`rfsweep server` Parameters](#rfsweep-server-parameters)
    - [`rfsweep reset` Parameters](#rfsweep-reset-parameters)
    - [`rfsweep restart` Parameters](#rfsweep-restart-parameters)
    - [`rfsweep getlogs` Parameters](#rfsweep-getlogs-parameters)
    - [`rfsweep transmit` Parameters](#rfsweep-transmit-parameters)
    - [`rfsweep measure` and `rfsweep receive` Parameters](#rfsweep-measure-and-rfsweep-receive-parameters)
- [`rfsweep` Data Formats](#rfsweep-data-formats)
- [Hardware Notices](#hardware-notices)
- [Cad Models](#cad-models)
- [Building `rfsweep`](#building-rfsweep)
    - [Compile for Debian/Ubuntu](#compile-for-debianubuntu)
    - [Compile on Windows CMD](#compile-on-windows-cmd)
    - [Compile on Windows Desktop](#compile-on-windows-desktop)
    - [Cross-Compile for Windows from Debian/Ubuntu](#cross-compile-for-windows-from-debianubuntu)
- [Installing Script Dependancies (optional)](#installing-script-dependancies-optional)
    - [Debian/Ubuntu Script Dependancies (optional)](#debianubuntu-script-depenancies-optional)
    - [Windows CMD Script Dependancies (optional)](#windows-cmd-script-dependancies-optional)
    - [Windows Desktop Script Dependancies (optional)](#windows-desktop-script-dependancies-optional)
- [Setting up the Raspberry PI](#setting-up-the-raspberry-pi)
- [Resources and Documentation](#resources-and-documentation)
