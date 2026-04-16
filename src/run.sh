#!/bin/bash

outfile=data-$(printf '%x' $(date +%s)).bin
extflags="--binary"

def_ip=10.42.0.1
def_port=12346
def_freq=2.4e9
def_srate=10e6
def_band=
def_lna_gain=16
def_vga_gain=20
def_isamp=false
def_steps=360
def_samps=1
def_smode=1


echo
echo "RF ANTENNA MEASURE TOOL"
echo "======================="
echo
echo "For the following options, enter a value and press enter."
echo "(leave blank and hit enter for default value)"

# echo
# read -p "Advanced options? (y/n): " is_advanced
# if [ -z "" ]; then is_advanced='n'; fi

echo
echo "Connection Options"
echo "------------------"

read -p "Enter Controller IP: " ip
read -p "Enter Controller Port: " port

ip=${ip:-$def_ip}
port=${port:-$def_port}

./rfsweep ping --ip=ip >/dev/null 2>&1
if [ "$?" != "0" ]; then
    echo
    echo "Unable to connect to Controller on <$ip:$port>."
    echo "Check if correct IP and Port, and check if Controller is on."
    #exit 1
fi

echo
echo "HackRF Options"
echo "--------------"

read -p "Enter Center Freq (Hz): " freq
read -p "Enter Band Filter (Hz): " band
read -p "Enter Sample Rate (Hz): " srate
read -p "Enter LNA-Gain (0-40 dB): " _lna_gain

_lna_gain=${_lna_gain:-$def_lna_gain}

lna_gain=$(((((_lna_gain+4)/8)*8)))
if [ "$_lna_gain" != "$lna_gain" ]; then
    echo "Rounding LNA-Gain to $lna_gain dB."
fi

read -p "Enter VGA-Gain (0-62 dB): " _vga_gain

_vga_gain=${_vga_gain:-$def_vga_gain}

vga_gain=$(((((_vga_gain+1)/2)*2)))
if [ "$_vga_gain" != "$vga_gain" ]; then
    echo "Rounding VGA-Gain to $vga_gain dB."
fi

read -p "Enable Amplifier? (y/n):  " isamp

echo
echo "Data Options"
echo "------------"

read -p "Enter Total Sample Steps: " steps
read -p "Enter Samples per Step: " samps
read -p "Enter Microstep Mode (1/2/4/8/16): " smode

echo
read -p "Press Enter to run test..."


if [ "$isamp" == "y" ] || [ "$isamp" == "Y" ]  || [ "$isamp" == "yes" ] || [ "$isamp" == "Yes" ]; then
    extflags="$extflags --amplify"
fi


if [ -z "$def_srate" ]; then
    def_band=$(($(printf "%.0f" def_srate)*75/100))
fi



freq=${freq:-$def_freq}
band=${band:-$def_band}
isamp=${isamp:-$def_isamp}
steps=${steps:-$def_steps}
samps=${samps:-$def_samps}
smode=${smode:-$def_smode}
srate=${srate:-$def_srate}



runstr="./rfsweep measure --ip=$ip --port=$port --steps=$steps --samps=$samps --stepmode=$smode --file=$outfile --freq=$freq --band=$band --srate=$srate --lna-gain=$lna_gain --vga-gain=$vga_gain --samps=$samps $extflags"

echo 
echo $runstr
$runstr

