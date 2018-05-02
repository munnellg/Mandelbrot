#include <omp.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define PROGRAM_NAME   "Mandelbrot"

#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600

#define THRESHOLD      4.0
#define MAX_ITERATIONS 255

#define MIN_RE -2
#define MAX_RE  1
#define MIN_IM -1
#define MAX_IM  1

static SDL_Window  *window;
static SDL_Surface *surface;
static int quit;

static int
mandelbrot ( float re, float im, int maxiter, float threshold ) {	
	int k;

	float u  = 0.0;
	float v  = 0.0;
	float u2 = u * u;
	float v2 = v * v;

	for (k = 1; k < maxiter; k++) {
		v = 2 * u * v + im;
		u = u2 - v2 + re;
		u2 = u * u;
		v2 = v * v;
		if ( u2 + v2 >= threshold ) { break; }
	}

	return k;
}

void
escape2colour ( int k, int *r, int *g, int *b ) {
	static unsigned char colours[][3] = { 
		{  66,  30,  15 },
		{  25,   7,  26 },
		{   9,   1,  47 },
		{   4,   4,  73 },
		{   0,   7, 100 },
		{  12,  44, 138 },
		{  24,  82, 177 },
		{  57, 125, 209 },
		{ 134, 181, 229 },
		{ 211, 236, 248 },
		{ 241, 233, 191 },
		{ 248, 201,  95 },
		{ 255, 170,   0 },
		{ 204, 128,   0 },
		{ 153,  87,   0 },
		{ 106,  52,   3 },
	};
	static size_t num_colours = sizeof(colours)/sizeof(colours[0]);	
	unsigned char *c = colours[k%num_colours];
	*r = c[0]; *g = c[1]; *b = c[2];
}

static int
initialize ( void ) {
	window  = NULL; // some default values for our static variables
	surface = NULL;
	quit    = 0;

	// Initialize SDL so we can use it
	if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
		printf("SDL_Init : %s\n", SDL_GetError());
		return 0;
	}

	// create a window
	window = SDL_CreateWindow ( 
				PROGRAM_NAME,            // title at top of window
				SDL_WINDOWPOS_UNDEFINED, // x center on screen
				SDL_WINDOWPOS_UNDEFINED, // y center on screen
				SCREEN_WIDTH,            // width
				SCREEN_HEIGHT,           // height
				0                        // flags for configuring window
			);

	// make sure we were able to create a window
	if ( !window ) {
		printf("SDL_CreateWindow : %s\n", SDL_GetError());
		return 0;
	}

	// get screen surface so we can do stuff with it in the render function
	surface = SDL_GetWindowSurface( window );

	return 1;
}

static void
handle_events ( void ) {
	SDL_Event e;
	while ( SDL_PollEvent(&e) ) {
		switch ( e.type ) {
			case SDL_QUIT: quit = 1;				
		}
	}
}

static void
render ( void ) {
	static int iterations = 0;
	if ( iterations < MAX_ITERATIONS ) { iterations++; }

	float dx = (float) abs(MAX_RE-MIN_RE)/SCREEN_WIDTH;
	float dy = (float) abs(MAX_IM-MIN_IM)/SCREEN_HEIGHT;

	// lock surface so we can directly manipulate pixels
	SDL_LockSurface( surface );
	
	// get pixels and cast to 32 bit ints
	Uint32 *pixels = surface->pixels;

	// do rendering in parallel
	#pragma omp parallel
	#pragma omp for
	for ( int i=0; i< SCREEN_HEIGHT*SCREEN_WIDTH; i++ ) {
		int r, g, b;
		float x = MIN_RE + dx*(i%surface->w);
		float y = MIN_IM + dy*(i/surface->w);
		int k = mandelbrot( x, y, iterations, THRESHOLD );
		
		if ( k >= iterations ) {
			r = g = b = 0;
		} else {
			escape2colour(k, &r, &g, &b );	
		}

		pixels[i] = SDL_MapRGB( surface->format, r, g, b );
	}

	// unlock the surface now that we're done
	SDL_UnlockSurface( surface );

	// flip the screen buffer
	SDL_UpdateWindowSurface( window );
}

static void
terminate ( void ) {
	if ( window ) { SDL_DestroyWindow(window); }
	SDL_Quit();
}

int
main ( int argc, char *argv[] ) {
	
	if (!initialize()) {		
		terminate();
		return 1;
	}

	while (!quit) {
		handle_events();
		render();
	}

	terminate();

	return 0;
}