// TGA header
// Original version from http://www.alsprogrammingresource.com/

#ifndef TGAH
#define TGAH

enum 
{                            
	TGA_ERROR_FILE_OPEN, 
	TGA_ERROR_READING_FILE, 
	TGA_ERROR_INDEXED_COLOR,
	TGA_ERROR_MEMORY, 
	TGA_ERROR_COMPRESSED_FILE,
	TGA_OK 
};

struct TGA_Info
{
	int status;
	unsigned char type, pixelDepth;
	short int width, height;
	unsigned char* imageData;
};

TGA_Info* TGA_Load(const char *filename);

void TGA_Destroy(TGA_Info *info);

int TGA_Save(const char* filename,
			 short int width,
			 short int height,
			 unsigned char pixelDepth,
			 unsigned char* imageData);
             
#endif
 

