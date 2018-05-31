#include <stdio.h>

#define TERMINAL_WIDTH  40
#define TERMINAL_HEIGHT 22
#define THRESHOLD       4.0f
#define MAX_ITERATIONS  256

int
mandelbrot ( float re, float im, int maxiter, float thresh ) {
	float u = 0, v = 0;
	float u2 = u, v2 = v;

	int k;
	for ( k = 1; k < maxiter; k++ ) {
        v = 2 * u * v + im;		
        u = u2 - v2 + re;
        u2 = u*u;
        v2 = v*v;
        if ( u2 + v2 >= thresh ) { break; }
	}

	return k;
}

int
main ( int argc, char *argv[] ) {
	float scale = 0.07;
	float centerx = -0.5, centery = 0;

	for ( int i=0; i<TERMINAL_HEIGHT*TERMINAL_WIDTH; i++ ) {
        int x = i%TERMINAL_WIDTH;
        int y = i/TERMINAL_WIDTH;
        float re = (x - (float)  TERMINAL_WIDTH/2) * scale + centerx;
        float im = (y - (float) TERMINAL_HEIGHT/2) * scale + centery;
        int k = mandelbrot( re, im, MAX_ITERATIONS, THRESHOLD );
        fprintf(stdout, "%c ", (k>=MAX_ITERATIONS)? 'x' : ' ');
        if ( (i+1)%TERMINAL_WIDTH == 0 ) { fprintf(stdout, "\n"); }
	}
	
	return 0;
}
