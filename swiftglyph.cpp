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
    Vec2() {}
    Vec2(float xIn, float yIn) : x(xIn), y(yIn) {}
    float x;
    float y;
};

Vec2 operator+(const Vec2& a, const Vec2& b)
{
    return Vec2(a.x + b.x, a.y + b.y);
}

Vec2 operator+(const Vec2& a, float scalar)
{
    return Vec2(a.x + scalar, a.y + scalar);
}

Vec2 operator-(const Vec2& a, const Vec2& b)
{
    return Vec2(a.x - b.x, a.y - b.y);
}

Vec2 operator-(const Vec2& a, float scalar)
{
    return Vec2(a.x - scalar, a.y - scalar);
}

Vec2 operator*(const Vec2& a, const Vec2& b)
{
    return Vec2(a.x * b.x, a.y * b.y);
}

Vec2 operator*(const Vec2& a, float scalar)
{
    return Vec2(a.x * scalar, a.y * scalar);
}

Vec2 operator/(const Vec2& a, const Vec2& b)
{
    return Vec2(a.x / b.x, a.y / b.y);
}

Vec2 operator/(const Vec2& a, float scalar)
{
    return Vec2(a.x / scalar, a.y / scalar);
}

struct GlyphInfo
{
    FT_UInt ftGlyphIndex;
    unsigned char ascii_char;
    Vec2 xy_lower_left;
    Vec2 xy_upper_right;
    Vec2 uv_lower_left;
    Vec2 uv_upper_right;
    Vec2 advance;
};

static GlyphInfo* s_glyphInfo;

void ErrorOut()
{
    printf("Generates a texture and font metrics for the specified font.\n");
    printf("    Usage: swiftglyph [options] fontname\n");
    printf("        -width integer   : specify width of the generated texture.\n");
    printf("        -padding integer : specify padding around each glyph. Can help prevent\n");
    printf("                           glyph clipping when rendering at small sizes.\n");
    printf("        -lua             : will output metrics file as a lua table instead of a yaml file.\n");
    printf("        -json            : will output metrics file as a json object file instead of yaml file.\n");
    printf("        -png             : will output texture as a png instead of a raw file.\n");
    printf("        -tga             : will output texture as a tga instead of a raw file.\n");
    printf("        -vflip           : texture coords will be flipped on v-axis LEGACY setting\n");
    exit(1);
}

static void ExportYAMLMetrics(const std::string& fontprefix, const std::string& fontname,
                              int textureWidth, float line_height)
{

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
        fprintf(fp, "  char_index: %u\n", s_glyphInfo[i].ftGlyphIndex);
        fprintf(fp, "  xy_lower_left: [%f, %f]\n", s_glyphInfo[i].xy_lower_left.x, s_glyphInfo[i].xy_lower_left.y);
        fprintf(fp, "  xy_upper_right: [%f, %f]\n", s_glyphInfo[i].xy_upper_right.x, s_glyphInfo[i].xy_upper_right.y);
        fprintf(fp, "  uv_lower_left: [%f, %f]\n", s_glyphInfo[i].uv_lower_left.x, s_glyphInfo[i].uv_lower_left.y);
        fprintf(fp, "  uv_upper_right: [%f, %f]\n", s_glyphInfo[i].uv_upper_right.x, s_glyphInfo[i].uv_upper_right.y);
        fprintf(fp, "  advance: [%f, %f]\n", s_glyphInfo[i].advance.x, s_glyphInfo[i].advance.y);
    }

    // dump kerning table
    fprintf(fp, "kerning:\n");
    bool empty = true;
    for (int i = 0; i < kNumGlyphs; ++i)
    {
        for (int j = 0; j < kNumGlyphs; ++j)
        {
            FT_Vector ftKerning;
            FT_Get_Kerning(s_face, s_glyphInfo[i].ftGlyphIndex, s_glyphInfo[j].ftGlyphIndex,
                           FT_KERNING_UNFITTED, &ftKerning);
            if (ftKerning.x != 0 || ftKerning.y != 0)
            {
                empty = false;
                fprintf(fp, "-\n");
                fprintf(fp, "  first_index: %u\n", s_glyphInfo[i].ftGlyphIndex);
                fprintf(fp, "  second_index: %u\n", s_glyphInfo[j].ftGlyphIndex);
                fprintf(fp, "  kerning: [%f, %f]\n", FIXED_TO_FLOAT(ftKerning.x) / line_height,
                        FIXED_TO_FLOAT(ftKerning.y) / line_height);
            }
        }
    }
    fclose(fp);
}

static void ExportLuaMetrics(const std::string& fontprefix, const std::string& fontname,
                             int textureWidth, float line_height)
{

    // dump out metrics for each glyph into a yaml file
    char luaFilename[512];
    sprintf(luaFilename, "%s.lua", fontprefix.c_str());
    FILE* fp = fopen(luaFilename, "w");
    fprintf(fp, "-- Font Metrics for %s\n", fontname.c_str());
    fprintf(fp, "Font {\n");
    fprintf(fp, "    texture_width = %d,\n", textureWidth);
    fprintf(fp, "    glyph_metrics = {\n");
    for (int i = 0; i < kNumGlyphs; ++i)
    {
        fprintf(fp, "        [%u] = { ascii_index = %u,\n", s_glyphInfo[i].ascii_char, s_glyphInfo[i].ascii_char);
        fprintf(fp, "            xy_lower_left = {%f, %f},\n", s_glyphInfo[i].xy_lower_left.x, s_glyphInfo[i].xy_lower_left.y);
        fprintf(fp, "            xy_upper_right = {%f, %f},\n", s_glyphInfo[i].xy_upper_right.x, s_glyphInfo[i].xy_upper_right.y);
        fprintf(fp, "            uv_lower_left = {%f, %f},\n", s_glyphInfo[i].uv_lower_left.x, s_glyphInfo[i].uv_lower_left.y);
        fprintf(fp, "            uv_upper_right = {%f, %f},\n", s_glyphInfo[i].uv_upper_right.x, s_glyphInfo[i].uv_upper_right.y);
        fprintf(fp, "            advance = {%f, %f} },\n", s_glyphInfo[i].advance.x, s_glyphInfo[i].advance.y);
    }
    fprintf(fp, "    },\n");

    // dump kerning table
    fprintf(fp, "    kerning = {\n");
    bool empty = true;
    for (int i = 0; i < kNumGlyphs; ++i)
    {
        for (int j = 0; j < kNumGlyphs; ++j)
        {
            FT_Vector ftKerning;
            FT_Get_Kerning(s_face, s_glyphInfo[i].ftGlyphIndex, s_glyphInfo[j].ftGlyphIndex,
                           FT_KERNING_UNFITTED, &ftKerning);
            if (ftKerning.x != 0 || ftKerning.y != 0)
            {
                empty = false;
                fprintf(fp, "        { first_char = %u,\n", s_glyphInfo[i].ascii_char);
                fprintf(fp, "          second_char = %u,\n", s_glyphInfo[j].ascii_char);
                fprintf(fp, "          kerning = {%f, %f} },\n", FIXED_TO_FLOAT(ftKerning.x) / line_height,
                        FIXED_TO_FLOAT(ftKerning.y) / line_height);
            }
        }
    }
    fprintf(fp, "    }\n");
    fprintf(fp, "}\n");
    fclose(fp);
}

static void ExportJSONMetrics(const std::string& fontprefix, const std::string& fontname,
                              int textureWidth, float line_height)
{

    // dump out metrics for each glyph into a yaml file
    char luaFilename[512];
    sprintf(luaFilename, "%s.json", fontprefix.c_str());
    FILE* fp = fopen(luaFilename, "w");
    fprintf(fp, "{\n");
    fprintf(fp, "    \"texture_width\": %d,\n", textureWidth);
    fprintf(fp, "    \"glyph_metrics\": {\n");
    for (int i = 0; i < kNumGlyphs; ++i)
    {
        fprintf(fp, "        \"%u\": {\n", s_glyphInfo[i].ascii_char);
        fprintf(fp, "            \"ascii_index\": %u,\n", s_glyphInfo[i].ascii_char);
        fprintf(fp, "            \"xy_lower_left\": [%f, %f],\n", s_glyphInfo[i].xy_lower_left.x, s_glyphInfo[i].xy_lower_left.y);
        fprintf(fp, "            \"xy_upper_right\": [%f, %f],\n", s_glyphInfo[i].xy_upper_right.x, s_glyphInfo[i].xy_upper_right.y);
        fprintf(fp, "            \"uv_lower_left\": [%f, %f],\n", s_glyphInfo[i].uv_lower_left.x, s_glyphInfo[i].uv_lower_left.y);
        fprintf(fp, "            \"uv_upper_right\": [%f, %f],\n", s_glyphInfo[i].uv_upper_right.x, s_glyphInfo[i].uv_upper_right.y);
        fprintf(fp, "            \"advance\": [%f, %f]\n", s_glyphInfo[i].advance.x, s_glyphInfo[i].advance.y);
        fprintf(fp, "        }%s\n", (i == kNumGlyphs - 1) ? "" : ",");
    }
    fprintf(fp, "    },\n");

    // dump kerning table
    fprintf(fp, "    \"kerning\": {\n");
    bool empty = true;
    for (int i = 0; i < kNumGlyphs; ++i)
    {
        for (int j = 0; j < kNumGlyphs; ++j)
        {
            FT_Vector ftKerning;
            FT_Get_Kerning(s_face, s_glyphInfo[i].ftGlyphIndex, s_glyphInfo[j].ftGlyphIndex,
                           FT_KERNING_UNFITTED, &ftKerning);
            if (ftKerning.x != 0 || ftKerning.y != 0)
            {
                empty = false;
                fprintf(fp, "        {\n");
                fprintf(fp, "            \"first_char\" = %u,\n", s_glyphInfo[i].ascii_char);
                fprintf(fp, "            \"second_char\" = %u,\n", s_glyphInfo[j].ascii_char);
                fprintf(fp, "            \"kerning\" = {%f, %f}\n", FIXED_TO_FLOAT(ftKerning.x) / line_height, FIXED_TO_FLOAT(ftKerning.y) / line_height);
                fprintf(fp, "        }%s\n", (i == kNumGlyphs - 1 && j == kNumGlyphs - 1) ? "" : ",");
            }
        }
    }
    fprintf(fp, "    }\n");
    fprintf(fp, "}\n");
    fclose(fp);
}

int main(int argc, char** argv)
{
    // check options
    int textureWidth = 512;
    int padding = 1;
    bool vflip = false;
    std::string fontname;

    bool foundFile = false;

    enum MetricsFileType {YamlType, LuaType, JsonType};
    MetricsFileType metricsFileType = YamlType;

    enum TextureFileType {RawType, TgaType, PngType};
    TextureFileType textureFileType = RawType;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            ErrorOut();
        }
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
        else if (strcmp(argv[i], "-padding") == 0)
        {
            if ((i + 1) < argc)
            {
                padding = atoi(argv[i+1]);

                // positive and not too large.
                if (padding >= 0 && padding <= 10)
                {
                    i++;
                    continue;
                }
            }

            printf("Error : -padding should be followed by a positive integer less than 11.\n");
            return 1;
        }
        else if (strcmp(argv[i], "-png") == 0)
        {
            textureFileType = PngType;
        }
        else if (strcmp(argv[i], "-tga") == 0)
        {
            textureFileType = TgaType;
        }
        else if (strcmp(argv[i], "-lua") == 0)
        {
            metricsFileType = LuaType;
        }
        else if (strcmp(argv[i], "-json") == 0)
        {
            metricsFileType = JsonType;
        }
        else if (strcmp(argv[i], "-vflip") == 0)
        {
            vflip = true;
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
    const int kGlyphPixelBorder = padding;

    // allocate & clear the render buffer
    unsigned char* buffer = new unsigned char[kBufferSize];
    memset(buffer, 0, kBufferSize);

    int pixels = kGlyphWidth - (2 * kGlyphPixelBorder);
    error = FT_Set_Char_Size(s_face, pixels << 6, 0, 72, 0);
    assert(!error);

    float line_height = FIXED_TO_FLOAT(s_face->size->metrics.height);

    //printf("Generating glyphs\n");

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
            memcpy(dest + ((j + kGlyphPixelBorder) * kGlyphTextureWidth) + kGlyphPixelBorder,
                   s_face->glyph->bitmap.buffer + (s_face->glyph->bitmap.pitch * j),
                   s_face->glyph->bitmap.width);
        }

        // store metrics.
        s_glyphInfo[i].ftGlyphIndex = glyph_index;
        s_glyphInfo[i].ascii_char = kStartGlyph + i;

        Vec2 xy_ll = Vec2(FIXED_TO_FLOAT(s_face->glyph->metrics.horiBearingX),
                          FIXED_TO_FLOAT(s_face->glyph->metrics.horiBearingY - s_face->glyph->metrics.height)) / line_height;
        Vec2 xy_size = Vec2(FIXED_TO_FLOAT(s_face->glyph->metrics.width), FIXED_TO_FLOAT(s_face->glyph->metrics.height)) / line_height;
        const float kXYGlyphPadding = (float)kGlyphPixelBorder / kGlyphWidth;
        s_glyphInfo[i].xy_lower_left = xy_ll - kXYGlyphPadding;
        s_glyphInfo[i].xy_upper_right = s_glyphInfo[i].xy_lower_left + xy_size + 2.0f * kXYGlyphPadding;

        Vec2 glyph_bitmap_size = Vec2(s_face->glyph->bitmap.width, s_face->glyph->bitmap.rows);
        Vec2 uv_size = (glyph_bitmap_size + (2.0f * kGlyphPixelBorder)) / kGlyphTextureWidth;
        Vec2 upper_left = Vec2(x, y) / kGlyphTextureWidth;
        Vec2 lower_right = upper_left + uv_size;

        if (vflip)
        {
            s_glyphInfo[i].uv_lower_left = Vec2(upper_left.x, lower_right.y);
            s_glyphInfo[i].uv_upper_right = Vec2(lower_right.x, upper_left.y);
        }
        else
        {
            s_glyphInfo[i].uv_lower_left = Vec2(upper_left.x, 1.0f - lower_right.y);
            s_glyphInfo[i].uv_upper_right = Vec2(lower_right.x, 1.0f - upper_left.y);
        }

        s_glyphInfo[i].advance.x = FIXED_TO_FLOAT(s_face->glyph->metrics.horiAdvance) / line_height;
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

    delete [] buffer;

    if (textureFileType == TgaType)
    {
        // save it out as a targa.
        std::string fn = fontprefix + std::string(".tga");
        TGA_Save(fn.c_str(), kGlyphTextureWidth, kGlyphTextureWidth, 32, rgbaBuffer);
        delete [] rgbaBuffer;
    }
    else if (textureFileType == PngType)
    {
        // save a temp targa
        TGA_Save("temp.tga", kGlyphTextureWidth, kGlyphTextureWidth, 32, rgbaBuffer);
        delete [] rgbaBuffer;

        // convert the tga to a png
        std::string fn = fontprefix + std::string(".png");
        char cmd[512];
        //sprintf(cmd, "sips -s format png temp.tga --out %s", fn.c_str());
        sprintf(cmd, "magick convert -flip temp.tga %s", fn.c_str());
        system(cmd);

        remove("temp.tga");
    }
    else if (textureFileType == RawType)
    {
        // first save it out as a targa.
        TGA_Save("temp.tga", kGlyphTextureWidth, kGlyphTextureWidth, 32, rgbaBuffer);
        delete [] rgbaBuffer;

        char cmd[512];

        int w = kGlyphTextureWidth;
        int i = 0;
        while (w >= 1)
        {
            printf("processing lod level %d\n", i);

            // scale the image for each mip-level
            sprintf(cmd, "magick convert -scale %dx%d temp.tga temp2.tga", w, w);
            system(cmd);

            // stream into an intensity alpha format
            sprintf(cmd, "magick stream -map ia -storage-type char temp2.tga temp.raw");
            system(cmd);

            // concat all the mip levels into a single binary file
            sprintf(cmd, "cat temp.raw %s %s.raw", i == 0 ? ">" : ">>", fontprefix.c_str());
            system(cmd);

            w /= 2;
            i += 1;
        }

        remove("temp.tga");
        remove("temp2.tga");
        remove("temp.raw");
    }

    if (metricsFileType == LuaType)
    {
        ExportLuaMetrics(fontprefix, fontname, textureWidth, line_height);
    }
    else if (metricsFileType == YamlType)
    {
        ExportYAMLMetrics(fontprefix, fontname, textureWidth, line_height);
    }
    else if (metricsFileType == JsonType)
    {
        ExportJSONMetrics(fontprefix, fontname, textureWidth, line_height);
    }

    return 0;
}
