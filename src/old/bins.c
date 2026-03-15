#if 0

#include <stdint.h>
#include <stdlib.h>

#include "toolkit/debug.h"
#include "rfsweep/host.h"


static _bins_data_t *bins_data_new(int flen, int blen);
static fbin_t *bins_data_attach(_bins_data_t *bdata);
static void bins_data_free(_bins_data_t *bdata);

static int _fbins_soft_init(fbins_t *fbins, _bins_data_t *bdata, int flen, int blen);






static _bins_data_t *bins_data_new(int flen, int blen) {
    fbin_t *mem;
    _bins_data_t *bdata;

    // allocate self
    bdata = malloc(sizeof(_bins_data_t));

//     // allocate pointer array for fbins
//     mem = malloc(sizeof(void*) * flen);
//     assert(mem != NULL, NULL);
// 
//     // allocate fbins
//     for (int i = 0; i < flen; i++) {
//         mem[i] = malloc(sizeof(fbin_t) * blen);
//         assert(mem[i] != NULL, NULL);
//     }

    // allocate fbins data
    mem = malloc(sizeof(fbin_t) * flen*blen);

    *bdata = (_bins_data_t){
        .refcount = 0,
        .len = flen*blen,
        .data = mem,
    };

    return bdata;
}



static fbin_t *bins_data_attach(_bins_data_t *bdata) {
    DEBUG(assert(bdata->data != NULL, NULL);)
    bdata->refcount++;
    debugf("bdata (%p) reference increased to %d", bdata, bdata->refcount);
    return bdata->data;
}



// will only truly free after reference count drops to zero
static void bins_data_free(_bins_data_t *bdata) {
    bdata->refcount--;

    debugf("bdata (%p) reference decreased to %d", bdata, bdata->refcount);

    if (bdata->refcount > 0)
        return;

    // // free fbins
    // for (int i = 0; i < flen; i++) {
    //     free(bdata->bins[i]);
    //     DEBUG(bdata->bins[i] = NULL;)
    // }
        
    // free data array for fbins
    free(bdata->data);

    DEBUG(*bdata = (_bins_data_t){0};)

    debugf("bdata (%p) freed", bdata);
    
    // free self
    free(bdata);
}





static int _fbins_soft_init(fbins_t *fbins, _bins_data_t *bdata, int flen, int blen) {
    fbin_t *fdata, **mem;

    // attach bins_data_t
    fdata = bins_data_attach(bdata);
    assert(fdata != NULL, -1);

    // allocate pointer array for fbins
    mem = malloc(sizeof(void*) * flen);
    assert(mem != NULL, -2);

    // // point all of the pointers to their respective data location
    // for (int i = 0; i < flen; i++)
    //     mem[i] = &fdata[i*blen];
    
    *fbins = (fbins_t){
        .flen = flen,
        .blen = blen,
        .bins = mem,
        .ref = bdata,
    };
    
    return 0;
}





int fbins_init(fbins_t *fbins, int flen, int blen) {
    int err;
    _bins_data_t *bdata;
    // fbin_t *fdata, **mem;
    
    // allocate bins_data_t
    bdata = bins_data_new(flen, blen);
    assert(bdata != NULL, -1);

//     // attach bins_data_t
//     fdata = bins_data_attach(bdata);
//     assert(fdata != NULL, -2);
// 
//     // allocate pointer array for fbins
//     mem = malloc(sizeof(void*) * flen);
//     assert(mem != NULL, -1);

    // soft init
    err = _fbins_soft_init(fbins, bdata, flen, blen);
    assert(!err, err);

    // point all of the pointers to their respective data location
    for (int i = 0; i < flen; i++)
        fbins->bins[i] = &bdata->data[i*blen];
    
    // *fbins = (fbins_t){
    //     .flen = flen,
    //     .blen = blen,
    //     .bins = mem,
    //     .ref = bdata,
    // };
    
    return 0;
}



// fbins_t *fbins_new(int flen, int blen) {
//     fbins_t *self = malloc(sizeof(fbins_t));
//     fbins_init(self, flen, blen);
//     return self;
// }



void fbins_free(fbins_t *fbins) {
//     // free fbins
//     for (int i = 0; i < fbins->flen; i++) {
//         free(fbins->bins[i]);
//         DEBUG(fbins->bins[i] = NULL;)
//     }
//         
    // free pointer array for fbins
    free(fbins->bins);
    
    DEBUG(fbins->bins = NULL;)

    bins_data_free(fbins->ref);

    DEBUG(*fbins = (fbins_t){0};)
}




int fbins_segment(fbins_t *fbins_in, fbins_t *fbins_out, int bins, int overlap) {
    int err, jump, segs, tsegs;

    jump = bins - overlap;          // bins between segments
    segs = fbins_in->blen / jump;   // segments per fbin
    tsegs = segs * fbins_in->flen;  // total segments

    // soft init fbins
    // set flen to total segments
    // set blen to bins
    err = _fbins_soft_init(fbins_out, fbins_in->ref, tsegs, bins);
    assert(!err, err);

    // populate fbin pointers
    for (int i = 0; i < fbins_in->flen; i++) {  // iterate over input fbins
    for (int j = 0; j < segs; j++) {            // iterate over segs per fbins
        // output seg index = input fbin index * segs + seg index
        fbins_out->bins[i*segs + j] = &fbins_in->bins[i][j*jump];
    }}

    return 0;
}

#endif
