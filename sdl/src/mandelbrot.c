#include <omp.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define PROGRAM_NAME   "Mandelbrot"

#define DEFAULT_SCREEN_WIDTH   800
#define DEFAULT_SCREEN_HEIGHT  600

#define DEFAULT_THRESHOLD      4.0
#define DEFAULT_MAX_ITERATIONS 255

#define ZOOM_FACTOR 0.75

struct state {
	SDL_Window   *window;
	SDL_Renderer *renderer;
	SDL_Texture  *texture;

	int quit;
	int fullscreen;
	int screen_width, screen_height;
	
	// affects transformations from world space to camera space
	int screen_halfw, screen_halfh;
	double scale;	
	double centre_x, centre_y;

	double thresh;
	int maxiter;
	int curiter;
};

static void
zoom ( struct state *s, int dir ) {
	s->scale *= (dir < 0)? 1.0/ZOOM_FACTOR : ZOOM_FACTOR;
}

static void
scroll ( struct state *s, Sint32 xrel, Sint32 yrel ) {
	double dx = -xrel * s->scale;
	double dy = -yrel * s->scale;
	s->centre_x += dx;
	s->centre_y += dy;
}

static int
mandelbrot ( double re, double im, int maxiter, double threshold ) {	
	int k;

	double u  = 0.0, v  = 0.0;
	double u2 = u*u, v2 = v*v;

	// f_c(n) = z_n^2 + c => z and c have real and complex parts	
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
	// pretty colours...
	static Uint32 colours[] = { 
		0x421E0F, 0x19071A, 0x09012F, 0x040449,
		0x000764, 0x0C2C8A, 0x1852B1, 0x397DD1,
		0x86B5E5, 0xD3ECF8, 0xF1E9BF, 0xF8C95F,
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
	s->screen_halfw  = s->screen_width/2;
	s->screen_halfh  = s->screen_height/2;
	s->fullscreen    = 0;

	// these make the mandelbrot look nice
	s->thresh        = DEFAULT_THRESHOLD;	   // thresh when point escapes set
	s->maxiter       = DEFAULT_MAX_ITERATIONS; // max series length for test
	
	// Initial transformations chosen because they will fit full mandelbrot
	// on screen and position in centre(ish) of window
	s->scale         = 4.0/s->screen_height; // scale factor from cam to world
	s->centre_x      = 0.0;                // world space centre x
	s->centre_y      = 0.0;                    // world space centre y

	// Initialize SDL so we can use it
	if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
		printf("SDL_Init : %s\n", SDL_GetError());
		return 0;
	}
	
	// create a new window and renderer according to settings
	int status = SDL_CreateWindowAndRenderer(
			s->screen_width, s->screen_height,              // resolution
			s->fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP:0, // toggle fullscreen
			&s->window, &s->renderer                        // init structs
	);

	// make sure we were able to create a window and renderer
	if ( status < 0 ) {
		printf("SDL_CreateWindowAndRenderer : %s\n", SDL_GetError());
		return 0;
	}
	
	// configure how the renderer handles screen dimensions
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(s->renderer, s->screen_width, s->screen_height);

	SDL_SetWindowTitle(s->window, PROGRAM_NAME);


	// create the texture that we will use for rendering
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
			case SDL_MOUSEWHEEL:
				if ( e.wheel.y != 0 ) {
					int flipped = e.wheel.direction != SDL_MOUSEWHEEL_NORMAL;
					zoom( s, (flipped)? -e.wheel.y : e.wheel.y );
				}
				break;
			case SDL_MOUSEMOTION:
				if ( (e.motion.state & SDL_BUTTON_LMASK) != 0 ) {					
					scroll( s, e.motion.xrel, e.motion.yrel );
				}
				break;
		}
	}
}

static void
update ( struct state *s ) {
	if ( s->curiter < s->maxiter ) { s->curiter++; }
}

static void
render ( struct state *s ) {
	Uint32 pixels[s->screen_width*s->screen_height];

	// do rendering in parallel	
	#pragma omp parallel for
	for ( int i=0; i< s->screen_height*s->screen_width; i++ ) {
		int camera_x = i%s->screen_width - s->screen_halfw;
		int camera_y = i/s->screen_width - s->screen_halfh;		
		double world_x = camera_x*s->scale + s->centre_x;
		double world_y = camera_y*s->scale + s->centre_y;
		int k = mandelbrot( world_x, world_y, s->curiter, s->thresh );		
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
		update(&s);
		render(&s);
	}

	terminate(&s);

	return 0;
}