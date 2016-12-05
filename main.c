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

/*** GLOBALS ***/
static GLuint shader = 0;
static GLuint vao = 0;
static GLuint vbo = 0;

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
	// one-time OpenGL state machine initializations
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

	// read and compile shaders into a shader program
	char *vert_src = read_file("data/simple.vert");
	char *frag_src = read_file("data/simple.frag");
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
		-0.3, -0.3, 0.0,
		+0.3, -0.3, 0.0,
		 0.0,  0.3, 0.0,
	};
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(
		GL_ARRAY_BUFFER,   // target buffer
		sizeof(vertices),  // size in machine units (bytes)
		vertices,          // data to submit
		GL_STATIC_DRAW     // usage hint
	);

	// enable one attribute array, use it for vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,         // attribute index
		3,         // number of attribute components, max is 4
		GL_FLOAT,  // each component is a float
		GL_FALSE,  // do not normalize data
		0,         // stride, 0 stands for tightly packed data
		(void*)0   // offset in machine units (bytes)
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

	// bind a VAO
	glBindVertexArray(vao);

	// draw it as a set of triangles
	// try also GL_POINTS, GL_LINES, GL_LINES_STRIP, GL_LINES_LOOP
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// succeed or die!
	assert(glGetError() == GL_NO_ERROR);
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
	while (run) {
		while (SDL_PollEvent(&evt)) {
			// quit on app close event or 'esc' key press
			if (evt.type == SDL_QUIT || (
			    evt.type == SDL_KEYDOWN &&
			    evt.key.keysym.sym == SDLK_ESCAPE)) {
				run = 0;
				break;
			}
		}

		render();
		SDL_GL_SwapWindow(win);
	}

cleanup:
	shutdown_gl();
	shutdown(win, ctx);
	return EXIT_SUCCESS;
}
