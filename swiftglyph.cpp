#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <ft2build.h>
#include <string>
#include FT_FREETYPE_H
#include "tga.h"

static FT_Library s_freeTypeLibrary = 0;
static FT_Face s_face;

// 26.6 Fixed to Float
#define FIXED_TO_FLOAT(x) ((float)(x) / 64.0f)

// TODO: utf8 support
// Assumes ASCII encoding
static const int kStartGlyph = 32;
static const int kEndGlyph = 127;
static const int kNumGlyphs = (kEndGlyph - kStartGlyph);

const int TAB_SIZE = 4;

struct Vec2
{
	float x;
	float y;
};

struct GlyphInfo
{
	FT_UInt ftGlyphIndex;
	Vec2 bearing;
	Vec2 size;
	Vec2 texUpperRight;
	Vec2 texLowerLeft;
	Vec2 advance;
};

static GlyphInfo* s_glyphInfo;

void ErrorOut()
{
	printf("Generates a texture and font metrics for the specified font.\n");
	printf("    Usage: swiftglyph [options] fontname\n");
	printf("        -width number : specify width of the generated texture.\n");
	printf("        -debug        : will output generated texture as temp.tga.\n");
	exit(1);
}

int main(int argc, char** argv)
{
	// check options
	int textureWidth = 512;
	std::string fontname;
	bool foundFile = false;
	bool debug = false;
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-h") == 0)
			ErrorOut();
		else if (strcmp(argv[i], "-width") == 0)
		{
			if ((i + 1) < argc)
			{
				textureWidth = atoi(argv[i+1]);
				// positive and a power of two
				if (textureWidth > 0 && ((textureWidth & (textureWidth - 1)) == 0))
				{
					i++;
					continue;
				}
			}

			printf("Error : -width should be followed by a number which is a power of 2\n");
			return 1;
		}
		else if (strcmp(argv[i], "-debug") == 0)
		{
			debug = true;
		}
		else
		{
			if (!foundFile)
			{
				foundFile = true;
				fontname = argv[i];
			}
			else
				ErrorOut();
		}
	}

	if (!foundFile)
	{
		ErrorOut();
	}

	// strip the extention off of the font filename
	std::string fontprefix = fontname.substr(0, fontname.find_last_of("."));

	// Init FreeType
	FT_Error error = FT_Init_FreeType(&s_freeTypeLibrary);
	assert(!error);

	// Attempt to laod the font.
	error = FT_New_Face(s_freeTypeLibrary, fontname.c_str(), 0, &s_face);
	if (error)
	{
		fprintf(stderr, "Error Loading Font \"%s\"\n", fontname.c_str());
		return 1;
	}

	// allocate gylph metrics
	s_glyphInfo = new GlyphInfo[kNumGlyphs];
	assert(s_glyphInfo);

	const int kNumGlyphsPerRow = ceil(sqrt(kNumGlyphs));
	const int kGlyphTextureWidth = textureWidth;
	const int kGlyphWidth = kGlyphTextureWidth / kNumGlyphsPerRow;
	const int kBufferSize = kGlyphTextureWidth * kGlyphTextureWidth;

	// allocate & clear the render buffer
	unsigned char buffer[kBufferSize];
	memset(buffer, 0, kBufferSize);

	int pixels = kGlyphTextureWidth / kNumGlyphsPerRow;
	error = FT_Set_Char_Size(s_face, pixels << 6, 0, 72, 0);
	assert(!error);

	printf("Generating glyphs\n");
	
	// render each glyph into the buffer
	for (int i = 0; i < kNumGlyphs; ++i)
	{

		// load glyph into s_face->glyph
		FT_UInt glyph_index = FT_Get_Char_Index(s_face, kStartGlyph + i);
		error = FT_Load_Glyph(s_face, glyph_index, FT_LOAD_DEFAULT);
		assert(!error);

		// render glyph into s_face->glyph->bitmap
		error = FT_Render_Glyph(s_face->glyph, FT_RENDER_MODE_NORMAL);
		assert(!error);
		
		int r = i / kNumGlyphsPerRow;
		int c = i % kNumGlyphsPerRow;
		int x = c * kGlyphWidth;
		int y = r * kGlyphWidth;
		unsigned char* dest = buffer + (y * kGlyphTextureWidth) + x;
				
		// copy glyph bitmap into buffer, scanline by scanline.
		for (int j = 0; j < s_face->glyph->bitmap.rows; ++j)
		{
			memcpy(dest + (j * kGlyphTextureWidth), 
				   s_face->glyph->bitmap.buffer + (s_face->glyph->bitmap.pitch * j), 
				   s_face->glyph->bitmap.width);		
		}

		// store metrics.
		s_glyphInfo[i].ftGlyphIndex = glyph_index;
		s_glyphInfo[i].bearing.x = FIXED_TO_FLOAT(s_face->glyph->metrics.horiBearingX);
		s_glyphInfo[i].bearing.y = FIXED_TO_FLOAT(s_face->glyph->metrics.horiBearingY);
		s_glyphInfo[i].size.x = FIXED_TO_FLOAT(s_face->glyph->metrics.width);
		s_glyphInfo[i].size.y = FIXED_TO_FLOAT(s_face->glyph->metrics.height);

		float top = (float)y / kGlyphTextureWidth;
		float left = (float)x / kGlyphTextureWidth;
		float bottom = (float)(y + s_face->glyph->bitmap.rows) / kGlyphTextureWidth;
		float right = (float)(x + s_face->glyph->bitmap.width) / kGlyphTextureWidth;

		s_glyphInfo[i].texLowerLeft.x = left;
		s_glyphInfo[i].texLowerLeft.x = bottom;
		s_glyphInfo[i].texUpperRight.x = right;
		s_glyphInfo[i].texUpperRight.x = top;

		s_glyphInfo[i].advance.x = FIXED_TO_FLOAT(s_face->glyph->metrics.horiAdvance);
		s_glyphInfo[i].advance.y = 0.0f;
	}

	// NOTE: I could dump out the texture directly, but I don't want to generate
	// the mip-levels manually. so I save the image as a TGA file then shell out to 
	// imagemagick to do the work for me :)
	// Also, for debugging purposes, its handy to have the TGA file
	
	// create rgba bufffer.
	unsigned char* rgbaBuffer = new unsigned char[kBufferSize * 4];
	for (int i = 0; i < kBufferSize; ++i)
	{
		rgbaBuffer[i*4+0] = 255;
		rgbaBuffer[i*4+1] = 255;
		rgbaBuffer[i*4+2] = 255;
		rgbaBuffer[i*4+3] = buffer[i];
	}   

	// save it out as a targa.
	TGA_Save("temp.tga", kGlyphTextureWidth, kGlyphTextureWidth, 32, rgbaBuffer);
	delete [] rgbaBuffer;

	char cmd[512];
	int w = kGlyphTextureWidth;
	int i = 0;
	while (w >= 1)
	{
		printf("processing lod level %d\n", i);

		// scale the image for each mip-level
		sprintf(cmd, "convert -scale %dx%d temp.tga temp2.tga", w, w);
		system(cmd);

		// stream into an intensity alpha format
		system("stream -map ia -storage-type char temp2.tga temp.raw");

		// concat all the mip levels into a single binary file
		sprintf(cmd, "cat temp.raw >> %s.raw", fontprefix.c_str());
		system(cmd);

		w /= 2;
		i += 1;
	}

	// dump out metrics for each glyph into a yaml file
	char yamlFilename[512];
	sprintf(yamlFilename, "%s.yaml", fontprefix.c_str());
	FILE* fp = fopen(yamlFilename, "w");
	fprintf(fp, "# Font Metrics for %s\n", fontname.c_str());
	fprintf(fp, "texture_width: %d\n", textureWidth);
	fprintf(fp, "glyph_metrics:\n");
	for (int i = 0; i < kNumGlyphs; ++i)
	{
		fprintf(fp, "-\n");
		fprintf(fp, "  index: %u\n", s_glyphInfo[i].ftGlyphIndex);
		fprintf(fp, "  bearing: [%f, %f]\n", s_glyphInfo[i].bearing.x, s_glyphInfo[i].bearing.y);
		fprintf(fp, "  size: [%f, %f]\n", s_glyphInfo[i].size.x, s_glyphInfo[i].size.y);
		fprintf(fp, "  upper_right: [%f, %f]\n", s_glyphInfo[i].texUpperRight.x, 
				                                 s_glyphInfo[i].texUpperRight.y);
		fprintf(fp, "  lower_left: [%f, %f]\n", s_glyphInfo[i].texLowerLeft.x, 
				                                s_glyphInfo[i].texLowerLeft.y);
		fprintf(fp, "  advance: [%f, %f]\n", s_glyphInfo[i].advance.x, 
				                             s_glyphInfo[i].advance.y);
	}

	// dump kerning table
	fprintf(fp, "kerning:\n");
	for (int i = 0; i < kNumGlyphs; ++i)
	{
		for (int j = 0; j < kNumGlyphs; ++j)
		{
			FT_Vector ftKerning;
			FT_Get_Kerning(s_face, s_glyphInfo[i].ftGlyphIndex, s_glyphInfo[j].ftGlyphIndex, 
						   FT_KERNING_UNFITTED, &ftKerning);
			if (ftKerning.x != 0 || ftKerning.y != 0)
			{
				fprintf(fp, "-\n");
				fprintf(fp, "  first_index: %u\n", s_glyphInfo[i].ftGlyphIndex);
				fprintf(fp, "  second_index: %u\n", s_glyphInfo[j].ftGlyphIndex);
				fprintf(fp, "  kerning: [%f, %f]\n", FIXED_TO_FLOAT(ftKerning.x),
						FIXED_TO_FLOAT(ftKerning.y));
			}
		}
	}
	fclose(fp);

	if (!debug)
		remove("temp.tga");
	remove("temp2.tga");
	remove("temp.raw");

	return 0;
}
