#include <stdio.h>
#include <math.h>
#include "test.h"
#include <SDL/SDL.h>
#include <assert.h>

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

struct Font* s_font = 0;

#define TAB_SIZE 4
#define START_GLYPH 32
#define END_GLYPH 127
#define NUM_GLYPHS (END_GLYPH - START_GLYPH)


enum LoadFileToMemoryResult { CouldNotOpenFile = -1, CouldNotReadFile = -2 };
static int LoadFileToMemory(const char *filename, unsigned char **result)
{
	int size = 0;
	FILE *f = fopen(filename, "rb");
	if (f == NULL) 
	{
		*result = NULL;
		return CouldNotOpenFile;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = (unsigned char*)malloc(sizeof(unsigned char) * (size + 1));
	
	int newSize = (int)fread(*result, sizeof(char), size, f);
	if (size != newSize)
	{
		printf("size = %d, newSize = %d\n", size, newSize);
		free(*result);
		return CouldNotReadFile;
	}
	fclose(f);
	(*result)[size] = 0;  // make sure it's null terminated.
	return size;
}

struct Font* LoadFont(const char* font_filename, GLuint* glt)
{
	// load the font
	struct Font* font = (struct Font*)bbq_load(font_filename);

	glGenTextures(1, glt);
	glBindTexture(GL_TEXTURE_2D, *glt);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	unsigned char* texture_data;
	int result = LoadFileToMemory(font->texture_filename, &texture_data);
	if (result < 0)
	{
		printf("error loading %s\n", font->texture_filename);
		exit(1);
	}

	// load all mip levels
	int mip_level = 0;
	int width = font->texture_width;
	unsigned char* ptr = texture_data;
	printf("width = %d\n", width);
	while (width >= 1)
	{
		glTexImage2D(GL_TEXTURE_2D, mip_level, GL_LUMINANCE_ALPHA, width, width, 
				 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, ptr);
		ptr += width * width * 2;
		mip_level++;
		width /= 2;
	}
	free(texture_data);

	return font;
}

void FreeFont(struct Font* font)
{
	bbq_free(font);
}

struct GlyphMetrics* FindGlyphMetrics(struct Font* font, char c)
{
	int i = c - START_GLYPH;

	// if c is non-printable use '?'
	if ((i < 0) || (i >= NUM_GLYPHS))
		i = '?' - START_GLYPH;

	return font->glyph_metrics_array + i;
}

void DrawGlyph(float pen_x, float pen_y, struct GlyphMetrics* glyph)
{
	assert(glyph);

	// vertex coords

	float v0[3], v1[3], v2[3], v3[3];
	v0[0] = pen_x + glyph->xy_lower_left[0];
	v0[1] = pen_y + glyph->xy_lower_left[1];
	v1[0] = pen_x + glyph->xy_upper_right[0];
	v1[1] = pen_y + glyph->xy_lower_left[1];
	v2[0] = pen_x + glyph->xy_upper_right[0];
	v2[1] = pen_y + glyph->xy_upper_right[1];
	v3[0] = pen_x + glyph->xy_lower_left[0];
	v3[1] = pen_y + glyph->xy_upper_right[1];
	v0[2] = v1[2] = v2[2] = v3[2] = 0.0f;

	float uv0[3], uv1[3], uv2[3], uv3[3];
	uv0[0] = glyph->uv_lower_left[0];
	uv0[1] = glyph->uv_lower_left[1];
	uv1[0] = glyph->uv_upper_right[0];
	uv1[1] = glyph->uv_lower_left[1];
	uv2[0] = glyph->uv_upper_right[0];
	uv2[1] = glyph->uv_upper_right[1];
	uv3[0] = glyph->uv_lower_left[0];
	uv3[1] = glyph->uv_upper_right[1];
		
	glMultiTexCoord2fv(0, uv0);
	glVertex3fv(v0);

	glMultiTexCoord2fv(0, uv1);
	glVertex3fv(v1);

	glMultiTexCoord2fv(0, uv2);
	glVertex3fv(v2);

	glMultiTexCoord2fv(0, uv3);
	glVertex3fv(v3);
}

void KerningLookup(struct Font* font, unsigned int curr_index, unsigned int next_index, 
				   float* kerning_x, float* kerning_y)
{
	// TODO: faster search
	unsigned int i;
	for (i = 0; i < font->glyph_kerning_array_size; ++i)
	{
		if ((font->glyph_kerning_array[i].first_index == curr_index) &&
			(font->glyph_kerning_array[i].second_index == next_index))
		{
			*kerning_x = font->glyph_kerning_array[i].kerning[0];
			*kerning_y = font->glyph_kerning_array[i].kerning[1];
			return;
		}
	}
}

void DrawString(struct Font* font, const char* str)
{
	int cursor = 0;
	float pen_x = 0;
	float pen_y = 0;

	glBegin(GL_QUADS);
	const char* p;
	for (p = str; *p; ++p)
	{
		if (*p == '\n')
		{
			// move the pen down by height, and reset x to zero
			pen_x = 0;
			pen_y -= 1;
			cursor = 0;
		}
		else if (*p == 9)  // TAB
		{
			int numSpaces = TAB_SIZE - (cursor % TAB_SIZE);
			struct GlyphMetrics* curr = FindGlyphMetrics(font, ' ');
			int i;
			for (i = 0; i < numSpaces; ++i)
			{				
				DrawGlyph(pen_x, pen_y, curr);
				pen_x += curr->advance[0];
				pen_y += curr->advance[1];
				cursor++;
			}
		}
		else
		{
			struct GlyphMetrics* curr = FindGlyphMetrics(font, *p);
			DrawGlyph(pen_x, pen_y, curr);

			float kerning_x = 0;
			float kerning_y = 0;
			// look up in kerning table
			if (!isspace(*(p+1)))
			{
				struct GlyphMetrics* next = FindGlyphMetrics(font, *(p+1));
				KerningLookup(font, curr->char_index, next->char_index, &kerning_x, &kerning_y);
			}

			// advance the pen
			pen_x += curr->advance[0] + kerning_x;
			pen_y += curr->advance[1] + kerning_y;
			cursor++;
		}
	}
	glEnd();
}

void RenderInit()
{
	// set up projection matrix
	glMatrixMode(GL_PROJECTION);
	glOrtho(3.0, -3.0, -3.0, 3.0, 1.0, -1.0);
	glMatrixMode(GL_MODELVIEW);
	glRotatef(180.0,0,1,0);

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
	glColor4f(0.4,0.4,0.4,1);
	glBegin(GL_QUADS);
	for (i = 0; i < 4; i++)
	{
		glMultiTexCoord2fv(0, uvs); uvs += 2;
		glVertex3fv(verts); verts += 3;
	}
	glEnd();

	glColor4f(1,1,1,1);
	glBindTexture(GL_TEXTURE_2D, s_font_texture);
	DrawString(s_font, "WoWAW\nHello\nWorld!");

	SDL_GL_SwapBuffers();
}

int main(int argc, char* argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		fprintf(stderr, "Couldn't init SDL!\n");

	atexit(SDL_Quit);

	SDL_Surface* screen = SDL_SetVideoMode(512, 512, 32, 
										   SDL_HWSURFACE | SDL_RESIZABLE | SDL_OPENGL);

	if (!screen)
		fprintf(stderr, "Couldn't create SDL screen!\n");

	RenderInit();
	s_font = LoadFont("Inconsolata.bin", &s_font_texture);

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

	FreeFont(s_font);

	return 0;
}
