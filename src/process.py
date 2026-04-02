#!/bin/python3


import numpy as np
import scipy as sp
import copy
import math
import matplotlib.pyplot as pyplot
import random

is_binary = True
filename = 'data.bin'
binsize = 2048
overlap = 0.5





class Sample:
    timestamp = None
    angle     = None
    freq      = None
    band      = None
    samprate  = None
    bins      = []

    # set by class
    #original  = []
    pbins     = []
    pavg      = None
    #freqs     = []
    fbins     = []
    window    = []

    
    def __init__(self):
        pass


    def copy(self):
        return copy.copy(self)


    def split(self, binsize, overlap):
        splits = []
        usize = int(binsize * (1 - overlap))
        count = int(len(self.bins) / binsize)

        #print(binsize, overlap, usize, count, count*usize, len(self.bins))

        for i in range(0, count*usize, usize):
            new = self.copy()
            new.bins = self.bins[i:i+binsize]
            #print(i, i+binsize, len(self.bins), len(new.bins))
            splits.append(new)
            
        return splits


    def window(self, window='hann'):
        self.window = sp.signal.get_window(window, len(self.bins))
        self.bins = self.bins * self.window


    def normalize(self):
        pass


    def fft(self):
        self.fbins = np.fft.fft(self.bins)


    def fftrange(self):
        #print(self.freq, self.band)
        minfreq = self.freq - self.band/2
        maxfreq = self.freq + self.band/2
        # freqbin = self.band / len(self.fbins)
        return np.linspace(minfreq, maxfreq, len(self.fbins), endpoint=False)


    def filter(self, freq, band):
        frange = self.fftrange()
        for i, val in enumerate(self.fbins):
            self.fbins[i] *= 1 if -band < frange[i] - freq < band else 0.001
        return self


    def average(samples):
        refangle = samples[0].angle

        sout = samples[0].copy()
        sout.fbins = np.array([0] * len(sout.fbins))
        
        for s in samples:
            if s.angle != refangle:
                Exception("angles not the same")

            # I guess we are doing a power average
            #sout.fbins = np.sqrt(np.abs(sout.fbins**2) + (s.fbins / len(samples))**2)

            # coherent averaging
            sout.fbins = sout.fbins + s.fbins

        sout.fbins /= len(sout.fbins)

        return sout


    # doesn't guarentee ordered-ness
    def package(samples):
        pack = {}
        
        for s in samples:
            if s.angle in pack:
                pack[s.angle].append(s)
            else:
                pack[s.angle] = [s]
                                
        return pack

    # returns val, freq
    def max(self):
        mval = -1e100
        mfreq = -1
        for freq, val in zip(self.fftrange(), self.fbins):
            if val > mval:
                mfreq = freq
                mval = val
        return mval, mfreq


    # def freq_to_index(self, freq):
    #     pass


    def avgpower(self):
        #return sum((self.fbins/len(self.fbins))**2)
        return sum(np.abs(self.fbins)**2) / len(self.fbins)**2


    # for sorting
    def __lt__(self, other):
        return self.angle < other.angle

    def __eq__(self, other):
        return self.angle == other.angle

        




# FORMAT: <timestamp_micro:f64> <angle:f64> <freq_hz:f64> <band_hz:f64> <samplerate_hz:f64> <bincount:i64> [ <real:i8> <imaginary:i8> ...]

samples = []


# load items
print("Loading Items...")
if is_binary:
    raw = np.fromfile(filename, dtype=np.uint8)
else:
    items = np.loadtxt(filename, dtype=str, comments=None)


# process data
print("Formatting Data...")
while (len(raw if is_binary else items) > 0):

    if is_binary:
        # separate params
        timestamp, angle, freq, band, samprate = np.frombuffer(raw, dtype='f8', count=5)


        # get bins
        bincount = np.frombuffer(raw, dtype='i8', count=1, offset=8*5)[0] * 2
        # print(timestamp, angle, freq, band, samprate, bincount)
        # for i in range(8*6, 8*6+8): print(f'{raw[i]:02x} ', end="")
        # print()
        bins_raw = np.frombuffer(raw, dtype='i1', count=bincount, offset=8*6)

        # 88 de da 71 71 2a 35 8d
        
    else:
        # separate params
        timestamp, angle, freq, band, samprate = np.array(items[:5], dtype='f8')

        # get bins
        bincount = int(items[5]) * 2
        bins = np.array(items[6:bincount+6], dtype='i1')
        

    # format bins
    bins = bins_raw.astype(np.float32).reshape((-1,2))
    bins = bins[:,0] + 1j*bins[:,1]

        
    # create and populate sample
    s = Sample()
    s.timestamp = timestamp
    s.angle     = angle
    s.freq      = freq * 1000
    s.band      = band
    s.samprate  = samprate
    s.bins      = bins
    # s.original  = bins

    # append sample to array of samples
    samples.append(s)


    # check if item buffer is fully consumed
    if is_binary:
        raw = raw[bincount+6*8:]
    else:
        items = items[bincount+8:]



# split samples with overlap
print("Splitting Samples...")
samples2 = []
for s in samples:
    samples2 += s.split(binsize, overlap)


print("Windowing and Performing FFT...")
for s in samples2:
    s.window()
    s.fft()


#samples2 = sort(samples2)


samples = Sample.package(samples2)
samples2 = []


print("Averaging Samples...")
for time, slist in samples.items():
    s = Sample.average(slist)
    #s.fbins **= 2
    samples2.append(s)
    # slist[0].fbins = np.abs(slist[0].fbins)
    # samples2.append(slist[0])


print("Sorting Samples...")
samples = sorted(samples2)
samples2 = []


freq = sum([s.max()[1] for s in samples])/len(samples)
print(f"Peak amplitude occurs on average {freq} Hz.")

# print("Filtering Samples...")
# for s in samples:
#     s.filter(freq, 1e5/10)


#theta = np.deg2rad(samples[0].fftrange())
theta = np.deg2rad(np.array([s.angle for s in samples]))
# already average of powers
radius = np.array([s.avgpower() for s in samples])

#x = np.deg2rad(samples[0].fftrange())[5:-5]
#y = np.sqrt(np.real(samples[0].fbins)**2 + np.imag(samples[0].fbins)**2)
#y = np.abs(Sample.average(samples).fbins)[5:-5]
#y = np.real(np.abs(samples[0].fbins)[5:-5])

# frequency
x = samples[0].fftrange()
# angle
y = [s.angle for s in samples]
# signal
S = np.array([np.real(20 * np.log10(np.abs(s.fbins)**2)) for s in samples])

x0 = x[5:-5]
y1 = np.abs(random.choice(samples).fbins)[5:-5]**2
y2 = np.abs(Sample.average(samples).fbins)[5:-5]**2


# free samples
samples = None


# graph
print("Plotting Samples...")
plot = [None]*16
# fig = plt.figure()

# polar graph
# plot[0] = pyplot.subplot(2, 1, 1, projection='polar')
plot[0] = pyplot.subplot2grid((4, 4), (0,0), projection='polar', rowspan=2, colspan=2)
# spectrogram
# plot[1] = pyplot.subplot(2, 1, 2)
plot[1] = pyplot.subplot2grid((4, 4), (2,0), rowspan=2, colspan=4)
# sample graph 1
# plot[2] = pyplot.subplot2grid((4, 4), (1,2), rowspan=1, colspan=2)
plot[2] = pyplot.subplot2grid((4, 4), (0,2), rowspan=2, colspan=2)
# sample graph 2
#plot[3] = pyplot.subplot2grid((4, 4), (0,2), rowspan=1, colspan=2)

# radial graph
plot[0].set_xticks(np.deg2rad(np.arange(0,360,15)))
plot[0].set_xticklabels([f"{d}°" for d in np.arange(0,360,15)])
plot[0].set_rscale('log', base=10)
plot[0].plot(theta, radius, '-')

# sample graph
# plot[1] = pyplot.subplot(2, 1, 2)
# plot[1].set_yscale('log', base=10)
# plot[1].plot(x, y, linestyle='-')

#print(len(samples[0].fbins), binsize)

# spectrogram
# plot[1] = pyplot.subplot(2, 1, 2)
# plot[1].specgram(x, NFFT=binsize, Fs=fs, noverlap=512, cmap='viridis', scale='dB')
# plot[1].xlabel('Time [s]'); plt.ylabel('Frequency [Hz]'); plt.colorbar(label='dB')

# spectrogram
plot[1].pcolormesh(x, y, S, shading='gouraud', cmap='magma')
plot[1].set_xlabel('Frequency [Hz]')
plot[1].set_ylabel('Angle [deg]')
pyplot.colorbar(plot[1].collections[0], ax=plot[1], label='dB')


# sample graph 1
plot[2].set_yscale('log', base=10)
# plot[2].set_title('Random angle')
plot[2].set_xlabel('Frequency [Hz]')
plot[2].set_ylabel('Power')
plot[2].plot(x0, y1, linestyle='-', label='random')
plot[2].plot(x0, y2, linestyle='-', label='average')
plot[2].legend()

# sample graph 2
# plot[3].set_yscale('log', base=10)
# plot[3].set_title('Average over angle')
# plot[3].set_xlabel('Frequency [Hz]')
# plot[3].set_ylabel('Power')
# plot[3].plot(x0, y2, linestyle='-')


#pyplot.tight_layout(pad=0.1)
#pyplot.tight_layout(pad=0.0)
pyplot.subplots_adjust(hspace=1, wspace=1)
pyplot.show()

#print(sum(x), sum(y), sum(theta), sum(radius))


print("Done.")
