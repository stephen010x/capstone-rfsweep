
// simple_plplot_live.c
// build: gcc simple_plplot_live.c -o simple_plplot_live -lplplot -lm
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "toolkit/debug.h"
#include "rfsweep/host.h"
#include "kissfft/kiss_fft.h"

// https://plplot.sourceforge.net/docbook-manual/plplot-html-5.15.0/index.html
// https://plplot.sourceforge.net/docbook-manual/plplot-html-5.15.0/API.html

#ifndef _RASPI
#include <plplot/plplot.h>
#endif






#ifdef _RASPI
typedef float PLFLT;
int plsetopt(char*, char*) {return -1;}
void plinit(void) {}
void plcol0(int) {}
void plenv(float, float, float, float, int, int) {}
void pllab(char*, char*, char*) {}
void plline(int, float[], float[]) {}
void plend(void) {}
#endif



__inline__ float n_to_log(float n) {
    return 20.0*log10(fabs(n));
}


__inline__ float log_to_n(float log) {
    return pow(10.0, log/20.0);
}



// DEBUG(
// static __construct void _plot_init(void) {
// 	warnf("not compiled with plplot library");
// }
// )



int plot_fbins(fbins_t *fbins, float xstart, float xend, int xlen, bool do_average) {
	(void)fbins; (void)xstart; (void)xend; (void)xlen; (void)do_average;
	
    #ifndef _RASPI
    
    int err, xjump;
    float xbin, ymin = 0, ymax = 0;
    PLFLT x[xlen], y[xlen], yd[xlen], y1[xlen], y2[xlen];

    (void)err;


    DEBUG(
    if (fbins->flen != 1) {
        warnf("fbins has %d sample arrays. will only draw the first one", fbins->flen);
    }
    )

    // set options to target qtwidget as output
    plsetopt("dev", "qtwidget");

    plinit();   // returns void apparently

    xbin = (xend - xstart) / (float)(xlen-1);
    xjump = fbins->blen / xlen;

    // populate x and y arrays
    for (int i = 0; i < (int)lenof(x); i++) {
        x[i] = (double)(xstart + xbin*i);
        if (do_average) {
            y1[i] = y2[i] = 0;
            // average values within an xjump
            for (int j = i*xjump; j < (i+1)*xjump; j++) {
                y1[i] += fbins->bins[0][j].real;
                y2[i] += fbins->bins[0][j].imag;
            }
            y1[i] /= xjump; y2[i] /= xjump;
        } else {
            // sample values within an xjump
            y1[i] = fbins->bins[0][i*xjump].real;
            y2[i] = fbins->bins[0][i*xjump].imag;
        }
        float real = y1[i];
        float imag = y2[i];
        // get magnitude of complex
        y[i] = (double)(sqrt(imag*imag + real*real));
        if (y[i] > ymax) ymax = y[i];
        if (y[i] < ymin) ymin = y[i];

        // get log of magnitude
        yd[i] = (double)n_to_log((float)y[i]);
        if (yd[i] > ymax) ymax = yd[i];
        if (yd[i] < ymin) ymin = yd[i];
    }

    ymax *= 2;
    ymin = -ymax;

    // set background color
    plcol0(15);
    
    // setup window xmin, xmax, ymin, ymax, axis scale, draw type
    plenv(xstart, xend, ymin, ymax, 0, 0);


    // xlabel, ylabel, title
    pllab("x", "y", "Test");

    #if 1
    // draw line 1
    plcol0(9);      // set color
    plline(lenof(x), x, y1);

    // draw line 2
    plcol0(8);      // set color
    plline(lenof(x), x, y2);
    #endif

    // draw line 3
    plcol0(7);      // set color
    plline(lenof(x), x, y);

    // draw line 4
    plcol0(10);      // set color
    plline(lenof(x), x, yd);

//     // wait until keypress
//     printf("TEST: Press key to continue...\n");
//     fflush(stdout);
//     getchar();

    // end graph
    plend();

    #else

    assert(("not compiled with plplot library", false), -1);

    #endif

    return 0;
}

