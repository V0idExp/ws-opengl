#include "matlib.h"
#include <GL/glew.h>
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

extern char*
read_file(const char *filename);

/*** DEFINES AND CONSTANTS ***/
#define WIDTH 800
#define HEIGHT 600
#define MOVE_SPEED 100.0f
#define ROT_SPEED M_PI

enum {
	MOVE_UP = 1,
	MOVE_DOWN = 1 << 1,
	MOVE_LEFT = 1 << 2,
	MOVE_RIGHT = 1 << 3,
	MOVE_FORWARD = 1 << 4,
	MOVE_BACKWARD = 1 << 5,
};

/*** GLOBALS ***/
static GLuint shader = 0;
static GLuint vao = 0;
static GLuint vbo = 0;

static int action = 0;

static Mat model;
static Mat view;
static Mat projection;
static Mat mvp;

static void
shutdown(SDL_Window *win, SDL_GLContext *ctx)
{
	if (ctx) {
		SDL_GL_DeleteContext(ctx);
	}

	if (win) {
		SDL_DestroyWindow(win);
	}

	SDL_Quit();
}

static int
init(unsigned width, unsigned height, SDL_Window **win, SDL_GLContext **ctx)
{
	// initialize SDL video subsystem
	if (!SDL_WasInit(SDL_INIT_VIDEO) && SDL_Init(SDL_INIT_VIDEO) != 0) {
		return 0;
	}

	// create window
	*win = SDL_CreateWindow(
		"OpenGL demo",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_OPENGL
	);
	if (!*win) {
		shutdown(*win, *ctx);
		return 0;
	}

	// initialize OpenGL context
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	*ctx = SDL_GL_CreateContext(*win);
	if (!*ctx) {
		shutdown(*win, *ctx);
		return 0;
	}

	// initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != 0) {
		shutdown(*win, *ctx);
		return 0;
	}
	glGetError(); // silence any errors produced during GLEW initialization

	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("GLEW version: %s\n", glewGetString(GLEW_VERSION));

	return 1;
}

static GLuint
compile_shaders(const char *vert_source, const char *frag_source) {
	// prepare and compile shader sources into OpenGL shader objects
	// (similar to compiling C/C++ sources to `.o` object files)
	GLuint src_objects[2] = { 0, 0 };
	GLenum src_types[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
	const GLchar *src_ptrs[2] = { vert_source, frag_source };
	GLint ok = 1;
	for (short i = 0; i < 2; i++) {
		// create an empty shader object
		// NOTE: these should be destroyed after all programs that use
		// them are released, left out for simplicity
		src_objects[i] = glCreateShader(src_types[i]);

		// set its source
		glShaderSource(
			src_objects[i],  // shader source object
			1,              // number of source strings
			&src_ptrs[i],   // source string array pointer
			NULL            // assume strints are NUL-terminated
		);

		// compile
		glCompileShader(src_objects[i]);

		// check compile status and print any errors to stderr
		glGetShaderiv(src_objects[i], GL_COMPILE_STATUS, &ok);
		if (!ok) {
			// retrieve compile log string length
			GLint len = 0;
			glGetShaderiv(src_objects[i], GL_INFO_LOG_LENGTH, &len);

			// retrieve and print compile log
			GLchar log[len];
			glGetShaderInfoLog(src_objects[i], len, NULL, log);
			fprintf(stderr, "%s\n", log);
			break;
		}
	}

	if (!ok) {
		return 0;
	}

	// create a shader program object
	GLuint shader = glCreateProgram();

	// link compiled shaders to it (similar to a C/C++ program `.o` files
	// linkage into final executable)
	for (short i = 0; i < 2; i++) {
		glAttachShader(shader, src_objects[i]);
	}
	glLinkProgram(shader);

	return shader;
}

static int
init_gl(void)
{
	// initialize model-view matrices
	mat_ident(&model);
	mat_ident(&view);

	// create a perspective projection matrix
	mat_persp(
		&projection,
		60,
		HEIGHT / (float)WIDTH,
		100.0f,
		1000.0f
	);

	// one-time OpenGL state machine initializations
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

	// read and compile shaders into a shader program
	char *vert_src = read_file("data/default.vert");
	char *frag_src = read_file("data/default.frag");
	if (vert_src && frag_src) {
		shader = compile_shaders(vert_src, frag_src);
	}
	free(vert_src);
	free(frag_src);

	// create and bind a Vertex Array Object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create, bind and fill with data a Vertex Buffer Object
	GLfloat vertices[] = {
		// vertex         // color
		-0.3, -0.3, 0.0,  1.0, 0.0, 0.0,
		+0.3, -0.3, 0.0,  0.0, 1.0, 0.0,
		 0.0,  0.3, 0.0,  0.0, 0.0, 1.0,
	};
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(
		GL_ARRAY_BUFFER,   // target buffer
		sizeof(vertices),  // size in machine units (bytes)
		vertices,          // data to submit
		GL_STATIC_DRAW     // usage hint
	);

	// specify position attributes array
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                   // vertex attribute index (layout = 0)
		3,                   // number of components, 3 for XYZ
		GL_FLOAT,            // each component is a float
		GL_FALSE,            // do not normalize data
		sizeof(GLfloat) * 6, // stride between attributes
		(void*)0             // vertex positions start at offset 0
	);

	// specify color attributes array
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                           // color attribute index (layout=1)
		3,                           // number of components, 3 for RGB
		GL_FLOAT,                    // each component is a float
		GL_FALSE,                    // do not normalize data
		sizeof(GLfloat) * 6,         // stride between attributes
		(void*)(sizeof(GLfloat) * 3) // vertex colors have an offset!
	);

	// unbind the VAO
	glBindVertexArray(0);

	return (
		shader != 0 &&
		vao != 0 &&
		vbo != 0 &&
		glGetError() == GL_NO_ERROR
	);
}

static void
shutdown_gl(void)
{
	glDeleteProgram(shader);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

static void
render(void)
{
	// clear color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// activate shader
	// NOTE: there always must be an active program before calling glDraw*()
	glUseProgram(shader);

	// introspect the shader program to find the uniform locations
	GLint mvp_loc = glGetUniformLocation(shader, "mvp");

	// set the uniforms using their location indices
	glUniformMatrix4fv(mvp_loc, 1, GL_TRUE, (GLfloat*)&mvp);

	// bind a VAO
	glBindVertexArray(vao);

	// draw it as a set of triangles
	// try also GL_POINTS, GL_LINES, GL_LINES_STRIP, GL_LINES_LOOP
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// succeed or die!
	assert(glGetError() == GL_NO_ERROR);
}

static void
update(float dt)
{
	static GLfloat dx = 0.0f, dy = 0.0f, dz = 0.0f;
	float dist = dt * MOVE_SPEED;
	if (action & MOVE_LEFT) {
		dx -= dist;
	}
	if (action & MOVE_RIGHT) {
		dx += dist;
	}
	if (action & MOVE_UP) {
		dy += dist;
	}
	if (action & MOVE_DOWN) {
		dy -= dist;
	}
	if (action & MOVE_FORWARD) {
		dz -= dist;
	}
	if (action & MOVE_BACKWARD) {
		dz += dist;
	}

	static GLfloat angle = 0.0f;
	angle += ROT_SPEED * dt;
	if (angle >= 2 * M_PI) {
		angle -= 2 * M_PI;
	}

	// update model matrix
	mat_ident(&model);
	mat_scale(&model, 100, 100, 100);
	mat_rotate(&model, 0.0, 1.0, 0.0, angle);
	mat_translate(&model, dx, dy, dz);

	// update view matrix
	mat_ident(&view);
	mat_translate(&view, 0, 0, -500);

	// chain all transformations into MVP matrix
	Mat modelview;
	mat_mul(&view, &model, &modelview);
	mat_mul(&projection, &modelview, &mvp);
}

static void
handle_keyboard(SDL_KeyboardEvent *key, int is_press)
{
	int bit = 0;
	switch (key->keysym.sym) {
	case SDLK_a:
	case SDLK_LEFT:
		bit = MOVE_LEFT;
		break;
	case SDLK_d:
	case SDLK_RIGHT:
		bit = MOVE_RIGHT;
		break;
	case SDLK_w:
	case SDLK_UP:
		bit = MOVE_UP;
		break;
	case SDLK_s:
	case SDLK_DOWN:
		bit = MOVE_DOWN;
	case SDLK_f:
		bit = MOVE_FORWARD;
		break;
	case SDLK_b:
		bit = MOVE_BACKWARD;
		break;
	}

	if (is_press) {
		action |= bit;
	} else {
		action &= ~bit;
	}
}

int
main(int argc, char *argv[])
{
	SDL_Window *win = NULL;
	SDL_GLContext *ctx = NULL;
	if (!init(WIDTH, HEIGHT, &win, &ctx) || !init_gl()) {
		goto cleanup;
	}

	int run = 1;
	SDL_Event evt;
	Uint32 last_update = SDL_GetTicks();
	float dt = 0.0f;
	while (run) {
		// update the logic
		update(dt);

		while (SDL_PollEvent(&evt)) {
			// quit on app close event or 'esc' key press
			if (evt.type == SDL_QUIT || (
			    evt.type == SDL_KEYDOWN &&
			    evt.key.keysym.sym == SDLK_ESCAPE)) {
				run = 0;
				break;
			}
			if (evt.type == SDL_KEYDOWN) {
				handle_keyboard(&evt.key, 1);
			} else if (evt.type == SDL_KEYUP) {
				handle_keyboard(&evt.key, 0);
			}
		}

		render();
		SDL_GL_SwapWindow(win);

		// compute time delta
		Uint32 now = SDL_GetTicks();
		dt = (now - last_update) / 1000.0f;
		last_update = now;
	}

cleanup:
	shutdown_gl();
	shutdown(win, ctx);
	return EXIT_SUCCESS;
}
