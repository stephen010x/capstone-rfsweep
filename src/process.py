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
import random
import sys




is_binary = True
filenames = sys.argv[2:]
binsize = 2048
overlap = 0.5
# target = 2.41e9
target = sys.argv[1]
filtband = 2e5


# # gain stuff
# minpow = -63    # min power of receiver in dB
# maxpow = -54    # max power of receiver in dB
# C = 3e8         # speed of light (m/s)
# dist = 1.88     # distance between antennas in meters
# tgain = 9       # gain of transmitter antenna in dB
# tpow = -9.5     # power of transmitter in dB

Gmax = 0
Gmin = -21





def main():

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


    # doing this last so it doesn't affect the other plots
    print("Applying Bandpass Filter to Frames...")
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
    print("Plotting Graphs...")
    rootplot   = Plot(4, 4)
    pmin = min([min(frad1), min(frad2), min(frad3)])
    pmax = max([max(frad1), max(frad2), max(frad3)])
    plot_polar  = rootplot.subplot_polar(       pos=(0,0), span=(2,2), range=(pmin, pmax), title='Radial Gain Chart')
    plot_linear = rootplot.subplot_linlog(      pos=(2,0), span=(2,2), xlabel='Frequency [Hz]', ylabel='Power', title='Fourier Spectrum Analysis')
    plot_spect  = rootplot.subplot_spectrogram( pos=(0,2), span=(4,2), xlabel='Frequency [Hz]', ylabel='Angle [deg]', alabel='dB', title='Frequency Spectrogram')


    # setup polar plots
    plot_polar.add_line(theta, frad3, label='Random',  color='lightblue', line='-')
    plot_polar.add_line(theta, frad2, label='RMS',     color='#d62728',   line='-')
    plot_polar.add_line(theta, frad1, label='Average', color='#1f77b4',   line='--') #'#1f77b4' '#2ca02c'
    
    # setup linear plots
    plot_linear.add_line(frange, flin1, label='Random',  color='lightblue', line='-')
    plot_linear.add_line(frange, flin3, label='RMS',     color='#d62728',   line='-')
    plot_linear.add_line(frange, flin2, label='Average', color='#1f77b4',   line='-') #'#1f77b4' '#2ca02c'

    # setup spectrogram plot
    plot_spect.set_spectro(frange, arange, Sspec, shading='gouraud', cmap='magma');

    # display plots
    rootplot.show()


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
            self.fbins[i] *= (1 if -band < frange[i] - freq < band else n)
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


    def copy(self):
        return copy.copy(self)

    def deepcopy(self):
        return copy.deepcopy(self)


    # def subplot_linlog(self, window=((0,0),(0,0)), title=None, xlabel=None, ylabel=None)
    def subplot_linlog(self, pos=(0,0), span=(0,0), title=None, xlabel=None, ylabel=None):
        new = self.copy()
        new.plot = self._create_subplot(pos, span)
        
        new.plot.set_yscale('log', base=10)
        new.plot.set_title(title)
        new.plot.set_xlabel(xlabel)
        new.plot.set_ylabel(ylabel)

        return new


    def add_line(self, X, Y, label=None, color=None, line='-'):
        if self.is_polar:
            self.plot.plot(np.append(X, X[0]), np.append(Y, Y[0]), linestyle=line, label=label, color=color)

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
        new = self.copy()
        new.plot = self._create_subplot(pos, span, projection='polar')
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
        new = self.copy()
        new.plot = self._create_subplot(pos, span)
        new.is_colorbar = True
        new.alabel = alabel

        new.plot.set_title(title)
        new.plot.set_xlabel(xlabel)
        new.plot.set_ylabel(ylabel)

        return new


    def _create_subplot(self, pos=(0,0), span=(0,0), projection=None):
        xmin, ymin = pos
        # xmax, ymax = window[1]
        # xspan = xax-xmin+1
        # yspan = yax-ymin+1
        xspan, yspan = span
        return pyplot.subplot2grid((self.rows, self.cols), (ymin,xmin), rowspan=yspan, colspan=xspan, projection=projection)


    def show(self):
        pyplot.subplots_adjust(hspace=1, wspace=1)
        pyplot.show()



# 
# # frequency
# x = samples[0].fftrange()
# # angle
# y = [s.angle for s in samples]
# # signal
# S = np.array([np.real(20 * np.log10(np.abs(s.fbins)**2)) for s in samples])
# 
# x0 = x[5:-5]
# y1 = np.abs(random.choice(samples).fbins)[5:-5]**2
# y2 = np.abs(Frame.average(samples).fbins)[5:-5]**2
# 
# 
# # free samples
# samples = None
# 
# #exit()
# 
# # graph
# print("Plotting Samples...")
# plot = [None]*16
# # fig = plt.figure()
# 
# # polar graph
# # plot[0] = pyplot.subplot(2, 1, 1, projection='polar')
# plot[0] = pyplot.subplot2grid((4, 4), (0,0), projection='polar', rowspan=2, colspan=2)
# # spectrogram
# # plot[1] = pyplot.subplot(2, 1, 2)
# plot[1] = pyplot.subplot2grid((4, 4), (2,0), rowspan=2, colspan=4)
# # sample graph 1
# # plot[2] = pyplot.subplot2grid((4, 4), (1,2), rowspan=1, colspan=2)
# plot[2] = pyplot.subplot2grid((4, 4), (0,2), rowspan=2, colspan=2)
# # sample graph 2
# #plot[3] = pyplot.subplot2grid((4, 4), (0,2), rowspan=1, colspan=2)
# #plot[3] = pyplot.subplot2grid((4, 4), (0,1), rowspan=2, colspan=1)
# 
# 
# 
# # radial graph
# # plot[0].set_theta_zero_location('N')
# # plot[0].set_theta_direction(-1)
# # plot[0].set_xticks(np.deg2rad(np.arange(0,360,15)))
# # plot[0].set_xticklabels([f"{d}°" for d in np.arange(0,360,15)])
# # plot[0].yaxis.set_major_locator(ticker.MaxNLocator(nbins=6))
# # plot[0].set_rlabel_position(0)
# # #plot[0].set_rscale('log', base=10)
# # plot[0].set_rmax(max(radius))
# # plot[0].set_rmin(min(radius))
# # plot[0].plot(np.append(theta, theta[0]), np.append(radius, radius[0]), '-')
# 
# # sample graph
# # plot[1] = pyplot.subplot(2, 1, 2)
# # plot[1].set_yscale('log', base=10)
# # plot[1].plot(x, y, linestyle='-')
# 
# #print(len(samples[0].fbins), binsize)
# 
# # spectrogram
# # plot[1] = pyplot.subplot(2, 1, 2)
# # plot[1].specgram(x, NFFT=binsize, Fs=fs, noverlap=512, cmap='viridis', scale='dB')
# # plot[1].xlabel('Time [s]'); plt.ylabel('Frequency [Hz]'); plt.colorbar(label='dB')
# 
# # spectrogram
# plot[1].pcolormesh(x, y, S, shading='gouraud', cmap='magma')
# plot[1].set_xlabel('Frequency [Hz]')
# plot[1].set_ylabel('Angle [deg]')
# pyplot.colorbar(plot[1].collections[0], ax=plot[1], label='dB')
# 
# 
# # sample graph 1
# plot[2].set_yscale('log', base=10)
# # plot[2].set_title('Random angle')
# plot[2].set_xlabel('Frequency [Hz]')
# plot[2].set_ylabel('Power')
# plot[2].plot(x0, y1, linestyle='-', label='random')
# plot[2].plot(x0, y2, linestyle='-', label='average')
# plot[2].legend()
# 
# # sample graph 2
# # plot[3].set_yscale('log', base=10)
# # plot[3].set_title('Average over angle')
# # plot[3].set_xlabel('Frequency [Hz]')
# # plot[3].set_ylabel('Power')
# # plot[3].plot(x0, y2, linestyle='-')
# 
# 
# 
# # plot[3].text(0.6, 0.3, f"Gain_min: {Gmin:0.2f} dB;\n\nGain_max: {Gmax:0.2f} dB;\n\n" +
# #                        f"min_angle: {minangle:0.1f}\u00B0;\n\nmax_angle: {maxangle:0.1f}\u00B0;\n\nAngle diff: {diffangle:0.1f}\u00B0;")
# # plot[3].axis('off')
# #plot[3] = pyplot.add_subplot(1, 3, 3); ax.axis('off');
# 
# 
# #pyplot.tight_layout(pad=0.1)
# #pyplot.tight_layout(pad=0.0)
# pyplot.subplots_adjust(hspace=1, wspace=1)
# pyplot.show()
# 
# #print(sum(x), sum(y), sum(theta), sum(radius))





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


