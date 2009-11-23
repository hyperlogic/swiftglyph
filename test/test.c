#include <stdio.h>
#include <math.h>
#include "test.h"
#include <SDL/SDL.h>

#ifdef DARWIN
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include "bbq.h"
#include "font.h"

// checker boards are cool!
#define WHITE 0xffffffff
#define BLACK 0xff000000
static unsigned int s_checker_texture_data[] = { 
	WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK,
	WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK,
	WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK,
	WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK,
	BLACK, BLACK, BLACK, BLACK, WHITE, WHITE, WHITE, WHITE, 
	BLACK, BLACK, BLACK, BLACK, WHITE, WHITE, WHITE, WHITE, 
	BLACK, BLACK, BLACK, BLACK, WHITE, WHITE, WHITE, WHITE, 
	BLACK, BLACK, BLACK, BLACK, WHITE, WHITE, WHITE, WHITE 
};

static float s_quad_verts[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f
};

static float s_quad_uvs[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f
};

static GLuint s_checker_texture;
static GLuint s_font_texture;

void RenderInit()
{
	// set up projection matrix
	glMatrixMode(GL_PROJECTION);
	glOrtho(1.0, -1.0, -1.0, 1.0, 1.0, -1.0);
	glMatrixMode(GL_MODELVIEW);

	// setup the checker board texture
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &s_checker_texture);
	glBindTexture(GL_TEXTURE_2D, s_checker_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, 8, 8, 
				 0, GL_RGBA, GL_UNSIGNED_BYTE, s_checker_texture_data);

	// load the font
	struct Font* font = (struct Font*)bbq_load("Inconsolata.bin");

	glGenTextures(1, &s_font_texture);
	glBindTexture(GL_TEXTURE_2D, s_font_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, font->texture_width, font->texture_width, 
				 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, font->texture);

	bbq_free(font);

}

void Render()
{
	glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
	
	glDepthMask(GL_FALSE);


	// display gray checker board
	float* uvs = s_quad_uvs;
	float* verts = s_quad_verts;
	glBindTexture(GL_TEXTURE_2D, s_checker_texture);
	glColor4f(0.2f, 0.2f, 0.2f, 1.0f);
	glBegin(GL_QUADS);
	unsigned int i;
	for (i = 0; i < 4; i++)
	{
		glMultiTexCoord2fv(0, uvs); uvs += 2;
		glVertex3fv(verts); verts += 3;
	}
	glEnd();

	// display font texture
	uvs = s_quad_uvs;
	verts = s_quad_verts;
	glBindTexture(GL_TEXTURE_2D, s_font_texture);
	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
	for (i = 0; i < 4; i++)
	{
		glMultiTexCoord2fv(0, uvs); uvs += 2;
		glVertex3fv(verts); verts += 3;
	}
	glEnd();


	SDL_GL_SwapBuffers();
}

int main(int argc, char* argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		fprintf(stderr, "Couldn't init SDL!\n");

	atexit(SDL_Quit);

	SDL_Surface* screen = SDL_SetVideoMode(800, 600, 32, 
										   SDL_HWSURFACE | SDL_RESIZABLE | SDL_OPENGL);

	if (!screen)
		fprintf(stderr, "Couldn't create SDL screen!\n");

	RenderInit();

	int done = 0;
	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					done = 1;
					break;

				case SDL_VIDEORESIZE:
					screen = SDL_SetVideoMode( event.resize.w, event.resize.h, 32, 
											   SDL_HWSURFACE | SDL_RESIZABLE | SDL_OPENGL );
					break;
			}
		}

		if (!done)
			Render();
	}

	return 0;
}
