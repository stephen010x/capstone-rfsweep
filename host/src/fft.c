
#define kiss_fft_scalar float
#include "kissfft/kiss_fft.h"
#include "rfsweep/host.h"
#include "utils/debug.h"
#include "utils/macros.h"



// typedef struct {
//     float real;
//     float imag;
// } fbin_t;
// 
// // typedef struct sbins_t {
// //     int len;
// //     sbin_t *bins;
// // } sbins_t;
// 
// typedef struct {
//     int flen; // number of fbins
//     int blen; // number of bins per fbin
//     fbin_t **bins;
// } fbins_t;




// Make sure that fbin_t is equivalent to kiss_fft_cpx
_Static_assert(__builtin_types_compatible_p(kiss_fft_scalar, typeof( ((fbin_t*)0)->real )),
    "fbin_t uses a different scalar type than kiss_fft_cpx");
_Static_assert(sizeof(fbin_t) == sizeof(kiss_fft_cpx), "fbin_t is not equivalent to kiss_fft_cpx");





typedef kiss_fft_cfg kiss_fft_cfg_t;
typedef kiss_fft_cpx kiss_fft_cpx_t;




int fbins_fft(fbins_t *fbins_in, fbins_t *fbins_out) {
    kiss_fft_cfg_t cfg;
    //kiss_fft_cpx_t *cpx_in;
    //kiss_fft_cpx_t *cpx_out;
    //fbins_t *fbins_out;

    // allocate populable buffers
    //*fbins_out = fbins_new(fbins_in->flen, fbins_in->blen);
    fbins_init(fbins_out, fbins_in->flen, fbins_in->blen);

    // allocate fft state object
    cfg = kiss_fft_alloc(fbins_in->blen, false, NULL, NULL);

    for (int i = 0; i < fbins_in->flen; i++) {
        kiss_fft(cfg, (kiss_fft_cpx_t*)fbins_in->bins[i], (kiss_fft_cpx*)fbins_out->bins[i]);
    }

    kiss_fft_free(cfg);

    return 0;
}


static __destruct void exit_fft(void) {
    kiss_fft_cleanup();
}




int fbins_average(fbins_t *fbins_in, fbins_t *fbins_out) {

    // init out bins
    fbins_init(fbins_out, 1, fbins_in->blen);

    // populate out with average
    for (int i = 0; i < fbins_in->blen; i++) {
        // zero out output
        fbins_out->bins[0][i] = (fbin_t){0};

        // add inputs to out
        for (int j = 0; j < fbins_in->flen; j++) {
            fbins_out->bins[0][i].real += fbins_in->bins[j][i].real;
            fbins_out->bins[0][i].imag += fbins_in->bins[j][i].imag;
        }

        // divide output to get average
        fbins_out->bins[0][i].real /= fbins_in->flen;
        fbins_out->bins[0][i].imag /= fbins_in->flen;
    }

    return 0;
}
