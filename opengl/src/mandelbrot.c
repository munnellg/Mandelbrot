// graphics libraries
#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>

// standard C libraries
#include <stdio.h>  // fprintf
#include <stdlib.h> // malloc, free
#include <memory.h> // memset

#define PROGRAM_NAME          "Mandelbrot"

// constants for framerate regulation
#define FRAMES_PER_SECOND     30
#define MS_PER_FRAME          (1000/FRAMES_PER_SECOND)
#define DEFAULT_SCREEN_WIDTH  800
#define DEFAULT_SCREEN_HEIGHT 600

#define DEFAULT_VERT_SHADER_FILENAME "shaders/vert.glsl"
#define DEFAULT_FRAG_SHADER_FILENAME "shaders/frag.glsl"

#define DEFAULT_THRESHOLD      4.0
#define DEFAULT_MAX_ITERATIONS 255
#define ZOOM_FACTOR            0.75

#define MAX(x,y) (((x)>(y))? (x) : (y))
#define MIN(x,y) (((x)<(y))? (x) : (y))

struct state {
    SDL_Window   *window;
    SDL_GLContext glcontext;

    int quit;
    int fullscreen;
    int screen_width, screen_height;

    // OpenGL constructs
    GLuint vertex_array_id;
    GLuint vertex_buffer;
    GLuint program_id;

    // shader uniforms
    int uniform_window_size;
    int uniform_centre;
    int uniform_curiter;
    int uniform_thresh;
    int uniform_scale;

    // filenames for the shaders this program uses
    char fvert_shader[128];
    char ffrag_shader[128];

    // mandelbrot controls
    float scale;    
    float centre_x, centre_y;

    float thresh;
    int maxiter;
    int curiter;
};

static const GLfloat vertex_buffer_data[] = {
    -1.0f, -1.0f, 0.0f,
     1.0f, -1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,
};

static void
zoom ( struct state *s, int dir ) {
    s->scale *= (dir < 0)? 1.0/ZOOM_FACTOR : ZOOM_FACTOR;
}

static void
scroll ( struct state *s, Sint32 xrel, Sint32 yrel ) {
    float dx = -xrel * s->scale;
    float dy = yrel * s->scale;
    s->centre_x += dx;
    s->centre_y += dy;
}

static int 
load_conf ( struct state *s, const char *fname ) {
    FILE *f;
    f = fopen(fname, "r");
    if (!f) { 
        fprintf(stderr, "fopen : can't open %s\n", fname);
        return 0; 
    }
    
    char line[512];
    for ( int lineno = 1; fgets( line, 512, f ) != NULL; lineno++ ) {        
        char *label, *lend, *val, *vend;
        // find start of label
        for ( label=line; *label!=0 && isspace(*label); label++ );
        // find end of label
        for ( lend=label; *lend!=0 && !isspace(*lend) && *lend!='='; lend++ );
        // find start of value
        for ( val=lend; *val!=0 && (isspace(*val)||*val=='='); val++ );        
        // find end of value
        for ( vend=val; *vend!=0 && !isspace(*vend); vend++ );
        // NULL terminate label and value
        *lend = *vend = 0;

        // error checking to see if we read a value and a label
        if ( label==lend && val==vend ) { continue; }
        if ( label==lend ) {
            fprintf(
                stderr, 
                "load_conf : value with no label - line %d - %s\n",
                lineno, val
            );
            continue;
        }
        if ( val==vend ) {
            fprintf(
                stderr,
                "load_conf : label with no value - line %d - %s\n",
                lineno, label
            );
            continue;
        }

        // interpret the result
        if ( strcmp(label, "screen_width") == 0 ) {
            int width = atoi(val);
            if ( width <= 0 ) {
                fprintf(
                    stderr,
                    "load_conf : invalid screen width - %d\n",
                    width
                );
            } else {
                s->screen_width = width;
            }
        } else if ( strcmp(label, "screen_height") == 0 ) {
            int height = atoi(val);
            if ( height <= 0 ) {
                fprintf(
                    stderr,
                    "load_conf : invalid screen height - %d\n",
                    height
                );
            } else {
                s->screen_height = height;
            }
        } else if ( strcmp(label, "fullscreen") == 0 ) {
            int fullscreen = atoi(val);
            s->fullscreen = (fullscreen)? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
        } else if ( strcmp(label, "vertex_shader") == 0 ) {            
            strncpy(s->fvert_shader, val, 128);
        } else if ( strcmp(label, "fragment_shader") == 0 ) {            
            strncpy(s->ffrag_shader, val, 128);
        }  
    }

    fclose(f);
    return 1;
}

static char *
load_shader_src ( const char *fname ) {
    FILE *f;    

    // boilerplate open file and quit on failure
    f = fopen(fname, "r");
    if (!f) { 
        fprintf(stderr, "fopen : can't open %s\n", fname); 
        return NULL; 
    }

    // get the size of the file in bytes and quit if file has no content
    fseek(f, 0, SEEK_END);
    size_t flen = ftell(f) + 1;
    fseek(f, 0, SEEK_SET);
    if (flen < 2) { 
        fprintf(stderr, "fseek : empty file\n"); 
        return NULL; 
    }

    // allocate enough memory to store complete file contents
    char *c = malloc(sizeof(char)*flen);
    if (!c) {
        fprintf(stderr, "malloc : malloc failed %s\n", fname);
        fclose(f);
        return NULL;
    }

    // load the file into memory
    fread(c, sizeof(char), flen-1, f);
    c[flen-1] = 0; // NULL terminator for string

    fclose(f);
    return c;
}

static int
compile_shader ( GLuint shader_id, const char *shader_src ) {
    // storage for log information reported after compiling shader
    char log[512];
    int info_log_length;

    // compile the shader    
    glShaderSource(shader_id, 1, &shader_src, NULL);
    glCompileShader(shader_id);

    // check result of compilation and print errors on failure
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if ( info_log_length > 0 ) {
        GLsizei loglen;
        glGetShaderInfoLog(shader_id, 512, &loglen, log);
        fprintf(stdout, "%s\n", log);
        return 0; 
    }

    return 1;
}

static int
load_shaders ( struct state *s,  const char *fvertsrc, const char *ffragsrc ) {
    GLuint vert_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    char *vert_src = load_shader_src(fvertsrc);
    if (!vert_src) { return 0; }
    char *frag_src = load_shader_src(ffragsrc);
    if (!frag_src) { free(vert_src); return 0; }
    if (!compile_shader(vert_shader_id, vert_src)) {
        free(vert_src); free(frag_src);
        return 0;
    }
    if (!compile_shader(frag_shader_id, frag_src)) {
        glDeleteShader(vert_shader_id);
        free(vert_src); free(frag_src);
        return 0;
    }
    s->program_id = glCreateProgram();
    glAttachShader(s->program_id, vert_shader_id);
    glAttachShader(s->program_id, frag_shader_id);
    glLinkProgram(s->program_id);

    // uniform arguments that we pass to the shader   
    s->uniform_window_size = glGetUniformLocation(s->program_id, "window_size");
    s->uniform_centre  = glGetUniformLocation(s->program_id, "centre");
    s->uniform_curiter = glGetUniformLocation(s->program_id, "curiter");
    s->uniform_thresh  = glGetUniformLocation(s->program_id, "thresh");
    s->uniform_scale   = glGetUniformLocation(s->program_id, "scale");

    glDetachShader(s->program_id, vert_shader_id);
    glDetachShader(s->program_id, frag_shader_id);
    glDeleteShader(vert_shader_id);
    glDeleteShader(frag_shader_id);
    free(vert_src);
    free(frag_src);
    return 1;
}

static int
initialize ( struct state *s ) {
    // initialize program to its default state
    memset(s, 0, sizeof(struct state));
    s->screen_width  = DEFAULT_SCREEN_WIDTH;
    s->screen_height = DEFAULT_SCREEN_HEIGHT;
    strncpy(s->fvert_shader, DEFAULT_VERT_SHADER_FILENAME, 128);
    strncpy(s->ffrag_shader, DEFAULT_FRAG_SHADER_FILENAME, 128);
    s->fullscreen    = 0;

    // these make the mandelbrot look nice
    s->thresh        = DEFAULT_THRESHOLD;      // thresh when point escapes set
    s->maxiter       = DEFAULT_MAX_ITERATIONS; // max series length for test
    
    // Initial transformations chosen because they will fit full mandelbrot
    // on screen and position in centre(ish) of window
    s->scale         = 4.0/s->screen_height; // scale factor from cam to world
    s->centre_x      = 0.0;                  // world space centre x
    s->centre_y      = 0.0;                  // world space centre y
    
    load_conf(s, "conf.txt");

    // initialize SDL and quit if something goes wrong
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
        fprintf(stderr, "SDL_Init : %s\n", SDL_GetError());
        return 0;
    }

    // create a new SDL OpenGL window
    s->window = SDL_CreateWindow(PROGRAM_NAME, 
                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                    s->screen_width, s->screen_height, 
                    SDL_WINDOW_OPENGL | s->fullscreen
                );
    // if opening the window failed, then quit the program
    if ( s->window == NULL ) {
        fprintf(stderr, "SDL_CreateWindow : %s\n", SDL_GetError());
        return 0;
    }
    
    // configure the OpenGL context - choose version etc.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    s->glcontext = SDL_GL_CreateContext(s->window);

    // need this in order to use the vertex arrays and buffers
    glewExperimental = GL_TRUE; 
    if ( glewInit() != 0 ) {
        fprintf(stderr, "glewInit : glew failed to initialize\n");
        return 0;
    }

    glGenVertexArrays(1, &s->vertex_array_id);
    glBindVertexArray(s->vertex_array_id);
    glGenBuffers(1, &s->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, s->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), 
            vertex_buffer_data, GL_STATIC_DRAW);

    if (!load_shaders(s, s->fvert_shader, s->ffrag_shader)) {
        fprintf(stderr, "load_shaders : failed to load shaders\n");
        return 0;
    }

    return 1;
}

static void
update ( struct state *s ) {
    if ( s->curiter < s->maxiter ) { s->curiter++; }
}

static void
handle_events ( struct state *s ) {
    SDL_Event e;

    while ( SDL_PollEvent(&e) ) {
        switch (e.type) {
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
render ( struct state *s ) {
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(s->program_id);
    glUniform2f(s->uniform_window_size, s->screen_width, s->screen_height);
    glUniform2f(s->uniform_centre, s->centre_x, s->centre_y);
    glUniform1i(s->uniform_curiter, s->curiter);
    glUniform1f(s->uniform_thresh, s->thresh);
    glUniform1f(s->uniform_scale, s->scale);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, s->vertex_buffer);
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    SDL_GL_SwapWindow(s->window);
}

static void
terminate ( struct state *s ) {
    if ( s->glcontext ) { SDL_GL_DeleteContext(s->glcontext); }
    if ( s->window )    { SDL_DestroyWindow(s->window); }
    SDL_Quit();
}

int
main ( int argc, char *argv[] ) {
    struct state s;

    if ( !initialize(&s) ) {
        terminate(&s);
        return EXIT_FAILURE;
    }
  
    while (!s.quit) {
        Uint32 start = SDL_GetTicks();
        handle_events(&s);
        render(&s);
        update(&s);
        Sint32 delay = MS_PER_FRAME - SDL_GetTicks() + start;
        SDL_Delay(MAX(0, delay));
    }

    terminate(&s);

    return EXIT_SUCCESS;
}
