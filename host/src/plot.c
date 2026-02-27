// simple_plplot_live.c
// build: gcc simple_plplot_live.c -o simple_plplot_live -lplplot -lm
#include <plplot/plstream.h>
#include <math.h>
#include <unistd.h>   // usleep

int main(void)
{
 PLFLT x[200], y[200];
 plstream *pls = new plstream();

 pls->init();                     // initialize PLplot (uses env/plplotrc
for device)
 pls->env(0.0, 199.0, -1.5, 1.5, 0, 0); // set viewport: x1,x2,y1,y2

 for (int frame = 0; frame < 500; ++frame) {
     for (int i = 0; i < 200; ++i) {
         x[i] = i;
         y[i] = sin((i*0.1) + frame*0.05);
     }

     pls->col0(1);                // color index
     pls->pgenv(0.0, 199.0, -1.5, 1.5, 0, 0); // reset coords (optional)
     pls->lab("X","Y","Live sine"); // labels
     pls->line(200, x, y);        // draw polyline
     pls->flush();                // force draw
     usleep(50000);               // 50 ms
     // clear by overplotting background (simple approach)
     pls->col0(0);
     pls->pabsl(0.0, 199.0, -1.5, 1.5); // draw blank axes area
(device-dependent)
 }

 delete pls;
 return 0;
}
