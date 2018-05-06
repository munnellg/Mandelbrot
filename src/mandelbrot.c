#include <omp.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define PROGRAM_NAME   "Mandelbrot"

#define DEFAULT_SCREEN_WIDTH   800
#define DEFAULT_SCREEN_HEIGHT  600

#define DEFAULT_THRESHOLD      4.0
#define DEFAULT_MAX_ITERATIONS 255

#define DEFAULT_MIN_RE -2
#define DEFAULT_MAX_RE  1
#define DEFAULT_MIN_IM -1
#define DEFAULT_MAX_IM  1

struct state {
	SDL_Window   *window;
	SDL_Renderer *renderer;
	SDL_Texture  *texture;

	int quit;
	int fullscreen;
	int screen_width, screen_height;

	float thresh;
	float min_re, max_re;
	float min_im, max_im;

	int maxiter;
	int curiter;
};

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

static Uint32
colourize ( int k ) {
	static Uint32 colours[] = { 
		0x421E0F, 0x19071A, 0x09012F, 0x040449,
		0x000764, 0x0C2C8A,	0x1852B1, 0x397DD1,
		0x86B5E5, 0xD3ECF8,	0xF1E9BF, 0xF8C95F,
		0xFFAA00, 0xCC8000, 0x995700, 0x6A3403,
	};
	static size_t num_colours = sizeof(colours)/sizeof(colours[0]);	
	return colours[k%num_colours];
}

static int
initialize ( struct state *s ) {
	// set up some defaults
	memset( s, 0, sizeof(struct state) );	
	s->screen_width  = DEFAULT_SCREEN_WIDTH;
	s->screen_height = DEFAULT_SCREEN_HEIGHT;
	s->thresh        = DEFAULT_THRESHOLD;
	s->max_im        = DEFAULT_MAX_IM;
	s->min_im        = DEFAULT_MIN_IM;
	s->max_re        = DEFAULT_MAX_RE;
	s->min_re        = DEFAULT_MIN_RE;
	s->maxiter       = DEFAULT_MAX_ITERATIONS;
	s->fullscreen    = 1;

	// Initialize SDL so we can use it
	if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
		printf("SDL_Init : %s\n", SDL_GetError());
		return 0;
	}
	
	int status = SDL_CreateWindowAndRenderer( 
			s->screen_width, s->screen_height,           // resolution
			s->fullscreen*SDL_WINDOW_FULLSCREEN_DESKTOP, // toggle fullscreen
			&s->window, &s->renderer                     // init structs
	);

	// make sure we were able to create a window
	if ( status < 0 ) {
		printf("SDL_CreateWindowAndRenderer : %s\n", SDL_GetError());
		return 0;
	}

	// get screen surface so we can do stuff with it in the render function	
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(s->renderer, s->screen_width, s->screen_height);
	s->texture = SDL_CreateTexture(
		s->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
		s->screen_width, s->screen_height
	);

	return 1;
}

static void
handle_events ( struct state *s ) {
	SDL_Event e;
	while ( SDL_PollEvent(&e) ) {
		switch ( e.type ) {
			case SDL_QUIT:
				s->quit = 1;
				break;
			case SDL_KEYDOWN:
				s->quit = e.key.keysym.sym == SDLK_q;
				break;
		}
	}
}

static void
render ( struct state *s ) {
	Uint32 pixels[s->screen_width*s->screen_height];

	if ( s->curiter < s->maxiter ) { s->curiter++; }

	float dx = (float) abs(s->max_re-s->min_re)/s->screen_width;
	float dy = (float) abs(s->max_im-s->min_im)/s->screen_height;

	// do rendering in parallel
	#pragma omp parallel
	#pragma omp for
	for ( int i=0; i< s->screen_height*s->screen_width; i++ ) {		
		float x = s->min_re + dx*(i%s->screen_width);
		float y = s->min_im + dy*(i/s->screen_width);
		int k = mandelbrot( x, y, s->curiter, s->thresh );		
		pixels[i] = ( k >= s->curiter )? 0 : colourize(k);	
	}

	SDL_UpdateTexture(s->texture, NULL, pixels, s->screen_width*sizeof(Uint32));
	SDL_RenderClear(s->renderer);
	SDL_RenderCopy(s->renderer, s->texture, NULL, NULL);
	SDL_RenderPresent(s->renderer);
}

static void
terminate ( struct state *s ) {
	if ( s->texture )  { SDL_DestroyTexture(s->texture); }
	if ( s->renderer ) { SDL_DestroyRenderer(s->renderer); }
	if ( s->window )   { SDL_DestroyWindow(s->window); }
	SDL_Quit();
}

int
main ( int argc, char *argv[] ) {
	struct state s;
	
	if (!initialize(&s)) {		
		terminate(&s);
		return 1;
	}

	while (!s.quit) {
		handle_events(&s);
		render(&s);
	}

	terminate(&s);

	return 0;
}