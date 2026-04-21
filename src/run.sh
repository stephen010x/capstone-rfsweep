#!/bin/bash

outdir="data/"
extflags="--binary"

offset=0.1

def_ip=10.42.0.1
def_port=12346
def_freq=2.4e9
def_srate=10e6
def_band=
def_lna_gain=16
def_vga_gain=20
def_isamp=n
def_steps=360
def_samps=1
def_smode=1



pytest=$(which python)
if [ -n "$pytest" ]; then py=python; fi
pytest=$(which python3)
if [ -n "$pytest" ]; then py=python3; fi
if [ -z "$py" ]; then
    echo "Could not find 'python' command. Check if python3 is installed."
    exit 1
fi


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

read -p "Enter Controller IP [$def_ip]: " ip
read -p "Enter Controller Port [$def_port]: " port

ip=${ip:-$def_ip}
port=${port:-$def_port}

./rfsweep ping --ip=$ip --port=$port >/dev/null 2>&1
if [ "$?" != "0" ]; then
    echo
    echo "Unable to connect to Controller on <$ip:$port>."
    echo "Check if correct IP and Port, and check if Controller is on."
    exit 1
fi

echo
echo "HackRF Options"
echo "--------------"

read -p "Enter Center Freq (Hz) [$def_freq]: " freq
read -p "Enter Sample Rate (Hz) [$def_srate]: " srate
read -p "Enter Band Filter (Hz) [srate*0.75]: " band
read -p "Enter LNA-Gain (0-40 dB) [$def_lna_gain]: " _lna_gain

_lna_gain=${_lna_gain:-$def_lna_gain}

lna_gain=$(((((_lna_gain+4)/8)*8)))
if [ "$_lna_gain" != "$lna_gain" ]; then
    echo "Rounding LNA-Gain to $lna_gain dB."
fi

read -p "Enter VGA-Gain (0-62 dB) [$def_vga_gain]: " _vga_gain

_vga_gain=${_vga_gain:-$def_vga_gain}

vga_gain=$(((((_vga_gain+1)/2)*2)))
if [ "$_vga_gain" != "$vga_gain" ]; then
    echo "Rounding VGA-Gain to $vga_gain dB."
fi

read -p "Enable Amplifier? (y/n) [$def_isamp]: " isamp

echo
echo "Data Options"
echo "------------"

read -p "Enter Total Sample Steps [$def_steps]: " steps
read -p "Enter Samples per Step [$def_samps]: " samps
read -p "Enter Microstep Mode (1/2/4/8/16) [$def_smode]: " smode

echo
read -p "Press Enter to run test..."


if [ "$isamp" == "y" ] || [ "$isamp" == "Y" ]  || [ "$isamp" == "yes" ] || [ "$isamp" == "Yes" ]; then
    extflags="$extflags --amplify"
fi


freq=${freq:-$def_freq}
band=${band:-$def_band}
isamp=${isamp:-$def_isamp}
steps=${steps:-$def_steps}
samps=${samps:-$def_samps}
smode=${smode:-$def_smode}
srate=${srate:-$def_srate}


# if [ -z "$band" ]; then
#     band=$(($(printf "%.0f" srate)*75/100))
# fi



# offset center frequency by 10% of sample rate to prevent DC from overlapping our signal
#realfreq=$(($(printf "%.0f" freq) - $(printf "%.0f" srate) / 10))
realfreq=$($py -c "print(f'{int($freq - $srate*$offset):e}'.replace('000e+0', 'e'))")


mkdir -p $outdir
outfile=$outdir/data-$(printf '%x' $(date +%s)).bin


runstr="./rfsweep measure --ip=$ip --port=$port --steps=$steps --samps=$samps --stepmode=$smode --file=$outfile --freq=$realfreq --band=$band --srate=$srate --lna-gain=$lna_gain --vga-gain=$vga_gain --samps=$samps $extflags"

pystr="$py process.py --freq $freq $outfile"

echo 
echo "$runstr && $pystr"
$runstr && $pystr
