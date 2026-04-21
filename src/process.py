#!/bin/python3

# input args:
# [1]  = target bandpass frequency (Hz)
# [2:] = filenames to data


import numpy as np
import scipy as sp
import copy
import math
import matplotlib.pyplot as pyplot
import matplotlib.ticker as ticker
import matplotlib.transforms as trans
import random
import sys
import argparse
import time
import os
import threading as thread




# options
is_binary = True
binsize = 2048
overlap = 0.5
filtband = 4e5
outdir = "images/"



# process arguments
p = argparse.ArgumentParser()
p.add_argument('--freq', type=float, required=False)  # optional frequency flag
p.add_argument('files', nargs='*')                  # collect all non‑flag args
args = p.parse_args()

# frequency to filter for
target = args.freq

# list of files to process
filenames = args.files


# # gain stuff
# minpow = -63    # min power of receiver in dB
# maxpow = -54    # max power of receiver in dB
# C = 3e8         # speed of light (m/s)
# dist = 1.88     # distance between antennas in meters
# tgain = 9       # gain of transmitter antenna in dB
# tpow = -9.5     # power of transmitter in dB

Gmax = 0
Gmin = -21



# output file dir
outfiledir = outdir + "/plots-" + str(hex(int(time.time()*1000)))[2:] + "/"




def main():

    global target

    # load files into samples
    frames = files_to_frames(filenames, is_binary)


    print("Splitting Frames...")
    fsplit = np.array([])
    for f in frames:
        fsplit = np.concatenate((fsplit, f.split(binsize, overlap)))



    print(f"Windowing and Performing FFT on {len(frames)} samples ({len(fsplit)} frames)...")
    # free memory
    frames = None
    satcount = 0
    for f in fsplit:
        satcount += f.satcount()
        # apply window
        f.window()
        # apply fft
        f.fft()
        # hide dc line (for graph aesthetics)
        f.squash_dc()


    # check for any saturated samples
    if satcount > 0:
        print(f"WARNING: {satcount} counts of saturation detected")


    favg = np.array([])
    frms = np.array([])
    frand = np.array([])
    
    print("Averaging Frames...")
    fpack = Frame.package(fsplit).items()
    rindex = random.randint(0, len(list(fpack)[0]) - 1)
    for time, flist in fpack:
        favg  = np.append(favg, Frame.bulk_average(flist))
        frms  = np.append(frms, Frame.bulk_rms(flist))
        frand = np.append(frand, flist[rindex])
    fpack = None


    # free memory
    fsplit = None


    print("Sorting Frames...")
    frand = sorted(frand)
    favg  = sorted(favg)
    frms  = sorted(frms)


    # linear plots
    frange = favg[0].fftrange()
    flin1  = np.abs(random.choice(frand).fbins)**2
    flin2  = np.abs(Frame.bulk_average(favg).fbins)**2
    flin3  = np.abs(Frame.bulk_rms(frms).fbins)**2


    # spectrogram plots
    arange = np.array([f.angle for f in favg])
    Sspec  = np.array([todB(np.abs(f.fbins))*2 for f in favg])


    # calculate target filter frequency
    freqpeak = Frame.get_max_signal_freq(favg)
    print(f"Peak signal estimated to occur at {freqpeak} Hz.")
    target = freqpeak if target is None else target

    # doing this last so it doesn't affect the other plots
    print(f"Applying Bandpass Filter ({target:.3e} \u00B1 {filtband/2:.0e} Hz) to Frames...")
    frand = np.array([f.deepcopy().filter(target, filtband, 1e-10) for f in frand])
    favg  = np.array([f.deepcopy().filter(target, filtband, 1e-10) for f in favg])
    frms  = np.array([f.deepcopy().filter(target, filtband, 1e-10) for f in frms])


    # polar plots
    theta = np.deg2rad(np.array([f.angle for f in favg]))
    frad1 = np.array([f.sumpow() for f in favg])
    frad2 = np.array([f.sumpow() for f in frms])
    frad3 = np.array([f.sumpow() for f in frand])
    
    # free memory
    frand = None
    favg  = None
    frms  = None

    # scale to gain and normalize
    frad1 = dBnorm(rad2gain(frad1, Gmin, Gmax))
    frad2 = dBnorm(rad2gain(frad2, Gmin, Gmax))
    frad3 = dBnorm(rad2gain(frad3, Gmin, Gmax))


    # create graphs
    rootplot   = Plot(4, 4)
    pmin = min([min(frad1), min(frad2), min(frad3)])
    pmax = max([max(frad1), max(frad2), max(frad3)])
    plot_polar  = rootplot.subplot_polar(pos=(0,0), span=(2,2), range=(pmin, pmax), 
            title=f'Polar Gain Chart ({target:.3e} Hz)'.replace('+0', '').replace('+', ''))
    plot_linear = rootplot.subplot_linlog(pos=(2,0), span=(2,2), 
            xlabel='Frequency [Hz]', ylabel='Power', title='Fourier Spectrum Analysis')
    plot_spect  = rootplot.subplot_spectrogram(pos=(0,2), span=(4,2), 
            xlabel='Frequency [Hz]', ylabel='Angle [deg]', alabel='dB', title='Frequency Spectrogram')


    # setup polar plots
    plot_polar.add_line(theta, frad3, label='Random',  color='lightblue', line='-')
    #plot_polar.add_line(theta, frad2, label='RMS',     color='#d62728',   line='-')
    plot_polar.add_line(theta, frad1, label='Average', color='black',     line='-') #'#1f77b4' '#2ca02c'
    
    # setup linear plots
    plot_linear.add_line(frange, flin1, label='Random',  color='lightblue', line='-')
    plot_linear.add_line(frange, flin3, label='RMS',     color='#d62728',   line='-')
    plot_linear.add_line(frange, flin2, label='Average', color='black'  ,   line='-') #'#1f77b4' '#2ca02c'

    # setup spectrogram plot
    plot_spect.set_spectro(frange, arange, Sspec, shading='gouraud', cmap='magma');

    # print output image to file
    def save_graphs():
        rootplot.deepcopy().save(    outfiledir, "plots.png", size=(16,9))
        plot_polar.deepcopy().save(  outfiledir, "fig1.png")
        plot_linear.deepcopy().save( outfiledir, "fig2.png")
        plot_spect.deepcopy().save(  outfiledir, "fig3.png")

    t1 = thread.Thread(target=save_graphs, args=[])
    t1.start()

    # display plots
    print("Plotting Graphs...")
    rootplot.show()

    print("Saving Graphs...")
    t1.join()
    print(f"Saved Graphs to \"{outfiledir}\".")


    print("Done.")












class Frame:
    timestamp = None
    angle     = None
    freq      = None
    band      = None
    samprate  = None
    bins      = []

    # Private. Set by class.
    pbins     = []
    fbins     = []
    window    = None

    
    def __init__(self):
        pass


    def copy(self):
        return copy.copy(self)

    def deepcopy(self):
        return copy.deepcopy(self)


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
        # self.window = None


    # def normalize(self):
    #     pass


    def fft(self):
        self.fbins = np.array(np.fft.fftshift(np.fft.fft(self.bins)), dtype=np.complex64)
        self.bins = None


    def fftrange(self):
        #print(self.freq, self.band)
        # minfreq = self.freq - self.band/2
        # maxfreq = self.freq + self.band/2
        minfreq = self.freq - self.samprate/2
        maxfreq = self.freq + self.samprate/2
        # freqbin = self.band / len(self.fbins)
        return np.linspace(minfreq, maxfreq, 
                len(self.bins if self.bins is not None else self.fbins), 
                endpoint=False)


    def filter(self, freq, band, n):
        frange = self.fftrange()
        #ret = []
        for i, val in enumerate(self.fbins):
            #ret.append(self.fbins[i] * (1 if -band < frange[i] - freq < band else n))
            self.fbins[i] *= (1 if -band/2 < frange[i] - freq < band/2 else n)
        #return ret
        return self


    def bulk_average(samples):
        refangle = samples[0].angle

        sout = samples[0].copy()
        sout.fbins = np.array([0] * len(sout.fbins))
        
        for s in samples:
            if s.angle != refangle:
                Exception("angles not the same")
            sout.fbins = sout.fbins + np.abs(s.fbins)

        sout.fbins /= len(samples)
        # sout.fbins /= np.sum(samples[0].window) if samples[0].window is not None else len(sout.fbins)

        return sout


    def bulk_rms(samples):
        refangle = samples[0].angle

        sout = samples[0].copy()
        sout.fbins = np.array([0] * len(sout.fbins))
        
        for s in samples:
            if s.angle != refangle:
                Exception("angles not the same")
            sout.fbins = sout.fbins + np.abs(s.fbins)**2

        # sout.fbins /= np.sum(samples[0].window) if samples[0].window is not None else len(sout.fbins)
        sout.fbins /= len(samples)
        sout.fbins **= 0.5

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


    def squash_dc(self):
        index = len(self.fbins)//2
        # for i in range(index-5, index+5):
        #     print(len(self.fbins), i, self.fbins[i])
        # print("----")
        self.fbins[index-2] = self.fbins[index-4]
        self.fbins[index-1] = self.fbins[index-3]
        self.fbins[index+0] = self.fbins[index-3]
        self.fbins[index+1] = self.fbins[index+3]
        self.fbins[index+2] = self.fbins[index+4]
        #self.bins -= np.mean(self.bins)
        

    def satcount(self):
        return sum([1 if (n == 127 or n == -128) else 0 for n in self.bins])


    # def freq_to_index(self, freq):
    #     pass


    def avgpower(self):
        #return sum((self.fbins/len(self.fbins))**2)
        return sum(np.abs(self.fbins)**2) / len(self.fbins)**2


    def sum(self):
        return sum(np.abs(self.fbins))


    def sumpow(self):
        return sum(np.abs(self.fbins)**2)


    # for sorting
    def __lt__(self, other):
        return self.angle < other.angle

    def __eq__(self, other):
        return self.angle == other.angle


    def get_max_signal_freq(samples):
        # array of indices to max for each sample
        maxi = [np.argmax(s.fbins) for s in samples]
        # get mean
        mean = np.mean(maxi)
        # sort by largest deviations from the mean. largest at the end
        maxi.sort(key=lambda x: abs(x-mean))
        # cut 50% of the most deviant samples to ensure that we get our 
        # signal if it is 50% or more dominant
        maxi = maxi[:len(maxi)//2]
        # Round our new mean to an index
        index = round(np.mean(maxi))
        # return frequency
        return samples[0].fftrange()[index]
        








# FORMAT: <timestamp_micro:f64> <angle:f64> <freq_hz:f64> <band_hz:f64> <samplerate_hz:f64> <bincount:i64> [ <real:i8> <imaginary:i8> ...]

def files_to_frames(filenames, is_binary):
    samples = []

    for filename in filenames:
        # load items
        print(f"Loading Items from {filename}...")
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
                bincount = np.frombuffer(raw, dtype='i8', count=1, offset=8*5)[0]
                # print(timestamp, angle, freq, band, samprate, bincount)
                # for i in range(8*6, 8*6+8): print(f'{raw[i]:02x} ', end="")
                # print()
                bins_raw = np.frombuffer(raw, dtype='i1', count=bincount, offset=8*6)

                # 88 de da 71 71 2a 35 8d
                
            else:
                # separate params
                timestamp, angle, freq, band, samprate = np.array(items[:5], dtype='f8')

                # get bins
                bincount = int(items[5])
                bins = np.array(items[6:bincount+6], dtype='i1')
                

            # format bins
            bins = bins_raw.astype(np.float32).reshape((-1,2))
            bins = bins[:,0] + 1j*bins[:,1]

                
            # create and populate sample
            s = Frame()
            s.timestamp = timestamp
            s.angle     = angle
            s.freq      = freq
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

    return samples










def todB(n):
    return 10*np.log10(n)



def fromdB(n):
    return 10**(n/10)




def rad2gain(array, Gmin, Gmax):
    minval = min(array)
    maxval = max(array)
    return (todB(array) - todB(minval)) * (Gmax - Gmin) / (todB(maxval) - todB(minval)) + Gmin




def dBnorm(array):
    return array - max(array)





# maxidx = max(enumerate(radius), key=lambda x: x[1])[0]
# minidx = min(enumerate(radius), key=lambda x: x[1])[0]
# maxangle = np.rad2deg(theta[maxidx])
# minangle = np.rad2deg(theta[minidx])
# diffangle = maxangle - minangle
# if diffangle < 0:
#     diffangle += 360
#print(f"Gain_min:{Gmin},  Gain_max:{Gmax},  Angle diff:{diffangle}")






class Plot:

    def __init__(self, cols=1, rows=1):
        self.rows = rows
        self.cols = cols
        self.plot = None
        self.is_polar    = False
        self.is_colorbar = False
        # self.plot = pyplot


    def copy(self):
        return copy.copy(self)

    def deepcopy(self):
        return copy.deepcopy(self)


    # def subplot_linlog(self, window=((0,0),(0,0)), title=None, xlabel=None, ylabel=None)
    def subplot_linlog(self, pos=(0,0), span=(0,0), title=None, xlabel=None, ylabel=None):
        #new = self.copy()
        new = self._create_subplot(pos, span)
        
        new.plot.set_yscale('log', base=10)
        new.plot.set_title(title)
        new.plot.set_xlabel(xlabel)
        new.plot.set_ylabel(ylabel)

        return new


    def add_line(self, X, Y, label=None, color=None, line='-', lw=None):
        if self.is_polar:
            self.plot.plot(np.append(X, X[0]), np.append(Y, Y[0]), linestyle=line, label=label, 
                    color=color, lw=lw)

        else:
            self.plot.plot(X, Y, linestyle=line, label=label, color=color)
            
        if label is not None:
            # self.plot.legend(loc='upper right')
            self.plot.legend(loc='upper left', bbox_to_anchor=(1.05, 1), borderaxespad=0.)


    def set_spectro(self, X, Y, S, shading='gouraud', cmap='magma'):
        if self.is_colorbar:
            self.plot.pcolormesh(X, Y, S, shading=shading, cmap=cmap)
            pyplot.colorbar(self.plot.collections[0], ax=self.plot, label=self.alabel)


    def subplot_polar(self, pos=(0,0), span=(0,0), range=None, title=None):
        #new = self.copy()
        new = self._create_subplot(pos, span, projection='polar')
        new.is_polar = True

        new.plot.set_title(title)
        new.plot.set_theta_zero_location('N')
        new.plot.set_theta_direction(-1)
        new.plot.set_xticks(np.deg2rad(np.arange(0,360,15)))
        new.plot.set_xticklabels([f"{d}°" for d in np.arange(0,360,15)])
        new.plot.yaxis.set_major_locator(ticker.MaxNLocator(nbins=6))
        new.plot.set_rlabel_position(0)
        #new.plot.set_rmax(max(radius))
        #new.plot.set_rmin(min(radius))

        if range is not None:
            new.plot.set_rmin(range[0])
            new.plot.set_rmax(range[1])

        return new


    def subplot_spectrogram(self, pos=(0,0), span=(0,0), title=None, xlabel=None, ylabel=None, alabel=None):
        #new = self.copy()
        new = self._create_subplot(pos, span)
        new.is_colorbar = True
        new.alabel = alabel

        new.plot.set_title(title)
        new.plot.set_xlabel(xlabel)
        new.plot.set_ylabel(ylabel)

        return new


    def _create_subplot(self, pos=(0,0), span=(0,0), projection=None):
        new = self.copy()
        new.span = span
        new.pos = pos
        new.plot = pyplot.subplot2grid((self.rows, self.cols), (pos[1], pos[0]), rowspan=span[1], colspan=span[0], projection=projection)
        return new


    def show(self):
        #fig = pyplot.gcf()
        #fig.set_dpi(100)
        pyplot.subplots_adjust(hspace=1, wspace=1)
        pyplot.show()


    def save(self, outdir, name, size=None):
        fig = pyplot.gcf() if self.plot is None else self.plot.figure
        os.makedirs(outdir, exist_ok=True)
        pyplot.subplots_adjust(hspace=1, wspace=1)

        if size is None: size = (16, 12)
        fig.set_size_inches(size[0], size[1], forward=True)

        if self.plot is not None:
            pos  = (self.pos[0] * size[0]/self.cols, self.pos[1] * size[1]/self.rows)
            pos  = (pos[0], size[1]/2 - pos[1])
            span = (self.span[0] * size[0]/self.cols, self.span[1] * size[1]/self.rows)
            bbox = trans.Bbox.from_bounds(pos[0], pos[1], span[0], span[1])
            
            fig.savefig(outdir + "/" + name, dpi=200, bbox_inches=bbox)
            
        else:
            fig.savefig(outdir + "/" + name, dpi=200)






# import gc, sys, types
# 
# def name_from_dict(d, obj):
#     names = []
#     for k, v in d.items():
#         if v is obj:
#             names.append(repr(k))
#     return names
# 
# def describe_ref(ref):
#     t = type(ref)
#     if isinstance(ref, dict):
#         return f"dict (keys: {list(ref.keys())[:10]})"
#     if isinstance(ref, types.FrameType):
#         return f"frame {ref.f_code.co_name} (locals keys: {list(ref.f_locals.keys())[:10]})"
#     return t.__name__
# 
# 
# def getsizeobj(obj):
#     try:
#         return sys.getsizeof(obj)
#     except Exception:
#         return -1
# 
# 
# def show_objects(limit=100):
#     gc.collect()
#     objs = gc.get_objects()
#     objs.sort(key=lambda x: getsizeobj(x), reverse=True)
#     objs = objs[:limit]
#     out = []
#     for i, o in enumerate(objs):
#         try:
#             size = sys.getsizeof(o)
#         except Exception:
#             size = -1
#         refs = gc.get_referrers(o)
#         ref_descs = []
#         names = set()
#         for r in refs:
#             ref_descs.append(describe_ref(r))
#             if isinstance(r, dict):
#                 for n in name_from_dict(r, o):
#                     names.add(n)
#         out.append((size, type(o).__name__, list(names) or None, ref_descs))
#     #out.sort(key=lambda x: x[0] if x[0] is not None else -1, reverse=True)
#     for size, typ, names, ref_descs in out[:limit]:
#         print(f"size={size:8} bytes  type={typ:20} names={names} refs={ref_descs[:6]}")





if __name__ == "__main__":
    main()
