#include <GL/glew.h>
#include <SDL.h>
#include <SDL_image.h>
#include <assert.h>
#include <stdio.h>

GLuint
load_texture(const char *filename, unsigned *width, unsigned *height)
{
	assert(filename);
	GLuint texture = 0;

	// initialize SDL image library
	static int img_initialized = 0;
	if (!img_initialized && IMG_Init(0) == -1) {
		fprintf(
			stderr,
			"failed to initialize SDL image library: %s\n",
			IMG_GetError()
		);
		return 0;
	}
	img_initialized = 1;

	// create a SDL surface from given image file
	SDL_Surface *surf = IMG_Load(filename);
	if (!surf) {
		fprintf(
			stderr,
			"failed to load image `%s`: %s\n", filename,
			IMG_GetError()
		);
		return 0;
	}

	// convert the surface to RGBA8888 format
#if SDL_COMPILEDVERSION >= SDL_VERSIONNUM(2, 0, 5)
	SDL_Surface *rgba_surf = SDL_ConvertSurfaceFormat(
		surf,
		SDL_PIXELFORMAT_RGBA32,
		0
	);
#else
	SDL_Surface *rgba_surf = SDL_ConvertSurfaceFormat(
		surf,
		SDL_BYTEORDER == SDL_BIG_ENDIAN ?
		SDL_PIXELFORMAT_RGBA8888 :
		SDL_PIXELFORMAT_ARGB8888,
		0
	);
#endif
	if (!rgba_surf) {
		fprintf(
			stderr,
			"image conversion to RGBA32 format failed: %s\n",
			SDL_GetError()
		);
		goto error;
	}

	// create OpenGL texture
	glGenTextures(1, &texture);
	if (!texture) {
		fprintf(
			stderr,
			"OpenGL texture creation failed (error %d)\n",
			glGetError()
		);
		goto error;
	}

	// initialize texture data
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA8,
		rgba_surf->w,
		rgba_surf->h,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		rgba_surf->pixels
	);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		fprintf(
			stderr,
			"OpenGL texture initialization failed (error %d)\n",
			glGetError()
		);
		goto error;
	}

	if (width) {
		*width = rgba_surf->w;
	}
	if (height) {
		*height = rgba_surf->h;
	}

cleanup:
	SDL_FreeSurface(rgba_surf);
	SDL_FreeSurface(surf);
	return texture;

error:
	glDeleteTextures(1, &texture);
	texture = 0;
	goto cleanup;
}