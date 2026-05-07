#!/bin/bash

# =================================================
# =================================================
# Edit settings below (avoid whitespace)


# -----------------
# Connection Settings
ip=10.42.0.1
port=7070

# -----------------
# Hackrf Settings
freq=2.4e9      # center frequency
srate=10e6      # sample rate (Hz)
band=           # bandpass filter width. Leave blank to set to 75% of srate
lna_gain=16     # LNA Gain (amplifies small signals) (0-40 dB, step of 8 dB)
vga_gain=20     # VGA Gain (software amplifier) (0-62 dB, step of 2 dB)
is_amp=false    # enable amplifier
steps=360       # how many angles to take samples
samps=1         # how many samples to take at each angle
stepmode=1      # microstep mode (1/2/4/8/16)

tx_vga_gain=20  # Transmitter VGA Gain  (0-47 dB, step of 1 dB)
tx_amp=127      # Transmitter Amplitude (0-127)
tis_amp=false   # Enable transmitter amplifier?

# -----------------
# Output Settings
outdir="data/"
extflags="--binary --clock"
textflags=""







# =================================================
# =================================================
# You don't need to edit anything past this point


offset=0.1      # offsets our center frequency by offset*srate


pytest=$(which python)
if [ -n "$pytest" ]; then py=python; fi
pytest=$(which python3)
if [ -n "$pytest" ]; then py=python3; fi
if [ -z "$py" ]; then
    echo "Could not find 'python' command. Check if python3 is installed."
    exit 1
fi


if [ "$is_amp" == "true" ]; then
    extflags="$extflags --amplify"
fi

if [ "$tis_amp" == "true" ]; then
    textflags="$textflags --amplify"
fi


# add 10% of sample rate to center frequency to prevent DC from overlapping our signal
#realfreq=$(($(printf "%.0f" freq) + $(printf "%.0f" srate) / 10))
#realfreq=$($py -c "print(f\"{int($freq - $srate*$offset):e}\".replace(\"000e+0\", \"e\"))")
realfreq=$($py -c "print(f\"{int($freq - $srate*$offset):g}\")")


mkdir -p $outdir
outfile=$outdir/data-$(printf '%x' $(date +%s)).bin

transon="./rfsweep transmit enable  --ip=$ip --port=$port --freq=$freq --vga-gain=$tx_vga_gain --tx-ampl=$tx_amp $textflags"

transoff="./rfsweep transmit disable  --ip=$ip --port=$port --freq=$freq --vga-gain=$tx_vga_gain --tx-ampl=$tx_amp $textflags"

runstr="./rfsweep measure --ip=$ip --port=$port --steps=$steps --samps=$samps --stepmode=$stepmode --file=$outfile --freq=$realfreq --band=$band --srate=$srate --lna-gain=$lna_gain --vga-gain=$vga_gain --samps=$samps $extflags"

pystr="$py process.py --freq $freq $outfile"

echo "$transon; ($runstr; $transoff) && $pystr"
$transon; ($runstr; $transoff) && $pystr
