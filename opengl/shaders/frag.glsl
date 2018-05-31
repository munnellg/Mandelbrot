#version 330 core

uniform vec2  window_size;
uniform vec2  centre;
uniform int   curiter;
uniform float thresh;
uniform float scale;

out vec4 frag_color;

in vec4 vertex_color; 

int
mandelbrot ( in float re, in float im, in float threshold, in int maxiter ) {
	int k;

	float u  = 0.0;
	float v  = 0.0;
	float u2 = u * u;
	float v2 = v * v;

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

vec4
colourize ( in int k ) {
	// pretty colours...
	const int numcolours = 16;
	const vec4 colours[numcolours] = vec4[]( 
		vec4(0.258824, 0.117647, 0.058824, 1.0),
		vec4(0.098039, 0.027451, 0.101961, 1.0),
		vec4(0.035294, 0.003922, 0.184314, 1.0),
		vec4(0.015686, 0.015686, 0.286275, 1.0),
		vec4(0.000000, 0.027451, 0.392157, 1.0),
		vec4(0.047059, 0.172549, 0.541176, 1.0),
		vec4(0.094118, 0.321569, 0.694118, 1.0),
		vec4(0.223529, 0.490196, 0.819608, 1.0),
		vec4(0.525490, 0.709804, 0.898039, 1.0),
		vec4(0.827451, 0.925490, 0.972549, 1.0),
		vec4(0.945098, 0.913725, 0.749020, 1.0),
		vec4(0.972549, 0.788235, 0.372549, 1.0),
		vec4(1.000000, 0.666667, 0.000000, 1.0),
		vec4(0.800000, 0.501961, 0.000000, 1.0),
		vec4(0.600000, 0.341176, 0.000000, 1.0),
		vec4(0.415686, 0.203922, 0.011765, 1.0)
	);	
	return colours[k%numcolours];
}

void
main() {
	int camera_x = int(gl_FragCoord.x - window_size[0]/2);
	int camera_y = int(gl_FragCoord.y - window_size[1]/2);	
	float world_x = float(camera_x*scale + centre[0]);
	float world_y = float(camera_y*scale + centre[1]);
	int k = mandelbrot( world_x, world_y, thresh, curiter );     
    frag_color = (k>=curiter)? vec4(0,0,0,0) : colourize(k);
} 
