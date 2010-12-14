Swiftglyph
==========

Overview
--------

Swiftglyph is a tool for rendering text using OpenGL.
That said, it doesn't actually do the rendering at runtime.
What it does do, is convert a TrueType or OpenType font into a bitmap and a metrics file.

The bitmap is written with a .raw extention.  
It's a cooked Lumanince Alpha texture ready to be streamed in directly to glTexImage2D, including mip levels.
There are also options to output tga and png images.

The metrics file is a .yaml file that includes all the metrics for each glyph in the texture.
It includes the uv-coordinates, bearing, size & advance.  
It also contains a kerning table for pairs of glyphs.
There is an option to output a lua table instead of a yaml file.

Armed with this data, it's easy to render glyphs and strings using texture mapped polygons.

Command line Options
--------------------

Usage: swiftglyph [options] fontname
*   -width integer : specify width of the generated texture.
*   -padding integer : specify padding around each glyph. 
    Can help prevent glyph clipping when rendering at small sizes. 
    Should be small in the 0-10 range.
*   -lua : will output metrics file as a lua table instead of a yaml file.
*   -png : will output texture as a png instead of a raw file.
*   -tga : will output texture as a tga instead of a raw file.

Code Sample
-----------

Here's a sample of reading in the cooked texture image into OpenGL.

	LoadFileToMemory("helvetica.raw", &texture_data);

	// load the raw font texture
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// generate all mip levels
	int mip_level = 0;
	int width = m_font->texture_width;
	unsigned char* ptr = texture_data;
	while (width >= 1)
	{
		glTexImage2D(GL_TEXTURE_2D, mip_level, GL_LUMINANCE_ALPHA, width, width, 
				 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, ptr);
		ptr += width * width * 2;
		mip_level++;
		width /= 2;
	}

	// dump the loaded texture
	free(texture_data);

