#!/bin/bash


def_freq=2.4e9
def_isamp=n
def_vga_gain=20
def_ampl=127


#buffsize=65536
extflags=""



pytest=$(which python)
if [ -n "$pytest" ]; then py=python; fi
pytest=$(which python3)
if [ -n "$pytest" ]; then py=python3; fi
if [ -z "$py" ]; then
    echo "Could not find 'python' command. Check if python3 is installed."
    exit 1
fi


echo
echo "HACKRF TRANSMIT SCRIPT"
echo "======================"
echo


read -p "Enter Transmission Frequency (Hz) [$def_freq]: " freq
read -p "Enable Amplifier? (y/n) [$def_isamp]: " isamp
read -p "Enter TX VGA-Gain (0-47 dB) [$def_vga_gain]: " vga_gain
read -p "Enter Signal Amplitude (0-127) [$def_ampl]: " ampl



vga_gain=${vga_gain:-$def_vga_gain}
freq=${freq:-$def_freq}
isamp=${isamp:-$def_isamp}
ampl=${ampl:-$def_ampl}



realfreq=$($py -c "print(int($freq+0.5))")


if [ "$isamp" == "y" ] || [ "$isamp" == "Y" ]  || [ "$isamp" == "yes" ] || [ "$isamp" == "Yes" ]; then
    extflags="$extflags -a 1"
fi



transtr="hackrf_transfer -c $ampl -p 1 -R -s 2000000 -f $realfreq -x $vga_gain $extflags"


echo
echo $transtr
echo
echo "Transmitting signal at $freq Hz..."
echo "Press Ctrl-C or close the window to stop transmission."
echo


$transtr




# read -r -d '' pystring <<PY | $transtr
# 
#     import sys.stdout.buffer as stdout
# 
#     #buff = bytes([0x80, 0x80]) * (bytelen // 2)
#     buff = bytes([1, 1]) * ($buffsize // 2)
# 
#     try:
#         while True:
#             stdout.write(buff)
#             stdout.flush()
#     except BrokenPipeError:
#         pass
# 
# PY
