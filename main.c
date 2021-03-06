#include <GL/glew.h>
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 800
#define HEIGHT 600

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

	// one-time OpenGL state machine initializations
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

	return 1;
}

static void
render(void)
{
	// clear color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int
main(int argc, char *argv[])
{
	SDL_Window *win = NULL;
	SDL_GLContext *ctx = NULL;
	if (!init(WIDTH, HEIGHT, &win, &ctx)) {
		return EXIT_FAILURE;
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

	shutdown(win, ctx);
	return EXIT_SUCCESS;
}
