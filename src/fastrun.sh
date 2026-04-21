#!/bin/bash

# =================================================
# =================================================
# Edit settings below


# -----------------
# Connection Settings
ip=10.42.0.1
port=12346

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

# -----------------
# Output Settings
outdir="data/"
extflags="--binary"







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


# add 10% of sample rate to center frequency to prevent DC from overlapping our signal
#realfreq=$(($(printf "%.0f" freq) + $(printf "%.0f" srate) / 10))
realfreq=$($py -c "print(f\"{int($freq - $srate*$offset):e}\".replace(\"000e+0\", \"e\"))")


mkdir -p $outdir
outfile=$outdir/data-$(printf '%x' $(date +%s)).bin


runstr="./rfsweep measure --ip=$ip --port=$port --steps=$steps --samps=$samps --stepmode=$stepmode --file=$outfile --freq=$realfreq --band=$band --srate=$srate --lna-gain=$lna_gain --vga-gain=$vga_gain --samps=$samps $extflags"

pystr="python3 process.py --freq $freq $outfile"

echo "$runstr && $pystr"
$runstr && $pystr
