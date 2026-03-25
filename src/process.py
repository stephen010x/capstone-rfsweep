#!/bin/python3


import numpy as np



class Sample:
    timestamp = None
    angle     = None
    freq      = None
    band      = None
    samprate  = None
    
    def __init__(self):
        pass



samps = []


while (true):

    # FORMAT: <timestamp_micro:f64> <angle:f64> <freq_hz:f64> <band_hz:f64> <samplerate_hz:f64> <bincount:i64> [ <real:i8> <imaginary:i8> ...]
    raw = np.fromfile('data.raw', dtype=np.uint8)

    # calculat params
    timestamp, angle, freq, band, samprate = np.frombuffer(raw, dtype='f4', count=5)

    # get bins
    bincount = np.frombuffer(raw, dtype='i4', count=1, offset=4*5)
    bins_raw = np.frombuffer(raw, dtype='i1', count=bincount*2, offset=4*6)

    # format bins
    bins = bins_raw.astype(np.float32).reshape((-1,2))
    bins = bins[:,0] + 1j*bins[:,1]

    s = Sample()

    s.timestamp = timestamp
    s.angle     = angle
    s.freq      = freq
    s.band      = band
    s.samprate  = samprate
    s.bins      = bins

    samps.append(s)

    raw = raw[bincount:]

    if (len(raw) == 0):
        break;
