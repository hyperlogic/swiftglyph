#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tga.h"

// load the image header fields. We only keep those that matter!
static void tgaLoadHeader(FILE *file, TGA_Info *info)
{
	unsigned char cGarbage;
	short int iGarbage;

	fread(&cGarbage, sizeof(unsigned char), 1, file);
	fread(&cGarbage, sizeof(unsigned char), 1, file);

// type must be 2 or 3
	fread(&info->type, sizeof(unsigned char), 1, file);

	fread(&iGarbage, sizeof(short int), 1, file);
	fread(&iGarbage, sizeof(short int), 1, file);
	fread(&cGarbage, sizeof(unsigned char), 1, file);
	fread(&iGarbage, sizeof(short int), 1, file);
	fread(&iGarbage, sizeof(short int), 1, file);

	fread(&info->width, sizeof(short int), 1, file);
	fread(&info->height, sizeof(short int), 1, file);
	fread(&info->pixelDepth, sizeof(unsigned char), 1, file);

	fread(&cGarbage, sizeof(unsigned char), 1, file);
}

// loads the image pixels. You shouldn't call this function
// directly
static void tgaLoadImageData(FILE *file, TGA_Info *info)
{
	int mode,total,i;
	unsigned char aux;

// mode equal the number of components for each pixel
	mode = info->pixelDepth / 8;
// total is the number of bytes we'll have to read
	total = info->height * info->width * mode;

	fread(info->imageData,sizeof(unsigned char),total,file);

// mode=3 or 4 implies that the image is RGB(A). However TGA
// stores it as BGR(A) so we'll have to swap R and B.
	if (mode >= 3)
		for (i=0; i < total; i+= mode) {
			aux = info->imageData[i];
			info->imageData[i] = info->imageData[i+2];
			info->imageData[i+2] = aux;
		}
}

// this is the function to call when we want to load
// an image
TGA_Info* TGA_Load(const char* filename)
{
	FILE *file;
	TGA_Info *info;
	int mode,total;

// allocate memory for the info struct and check!
	info = (TGA_Info *)malloc(sizeof(TGA_Info));
	if (info == NULL)
		return(NULL);


// open the file for reading (binary mode)
	file = fopen(filename, "rb");
	if (file == NULL) {
		info->status = TGA_ERROR_FILE_OPEN;
		return(info);
	}

// load the header
	tgaLoadHeader(file,info);

// check for errors when loading the header
	if (ferror(file)) {
		info->status = TGA_ERROR_READING_FILE;
		fclose(file);
		return(info);
	}

// check if the image is color indexed
	if (info->type == 1) {
		info->status = TGA_ERROR_INDEXED_COLOR;
		fclose(file);
		return(info);
	}
// check for other types (compressed images)
	if ((info->type != 2) && (info->type !=3)) {
		info->status = TGA_ERROR_COMPRESSED_FILE;
		fclose(file);
		return(info);
	}

// mode equals the number of image components
	mode = info->pixelDepth / 8;
// total is the number of bytes to read
	total = info->height * info->width * mode;
// allocate memory for image pixels
	info->imageData = (unsigned char *)malloc(sizeof(unsigned char) *
											  total);

// check to make sure we have the memory required
	if (info->imageData == NULL) {
		info->status = TGA_ERROR_MEMORY;
		fclose(file);
		return(info);
	}
// finally load the image pixels
	tgaLoadImageData(file,info);

// check for errors when reading the pixels
	if (ferror(file)) {
		info->status = TGA_ERROR_READING_FILE;
		fclose(file);
		return(info);
	}
	fclose(file);
	info->status = TGA_OK;
	return(info);
}

// saves an array of pixels as a TGA image
int TGA_Save(const char* filename,
			 short int width,
			 short int height,
			 unsigned char pixelDepth,
			 unsigned char* imageData)
{
    unsigned char cGarbage = 0, type,mode,aux;
    short int iGarbage = 0;
    int i;
    FILE *file;

// open file and check for errors
    file = fopen(filename, "wb");
    if (file == NULL) {
        return(TGA_ERROR_FILE_OPEN);
    }

// compute image type: 2 for RGB(A), 3 for greyscale
    mode = pixelDepth / 8;
    if ((pixelDepth == 24) || (pixelDepth == 32))
        type = 2;
    else
        type = 3;

// write the header
    fwrite(&cGarbage, sizeof(unsigned char), 1, file);
    fwrite(&cGarbage, sizeof(unsigned char), 1, file);

    fwrite(&type, sizeof(unsigned char), 1, file);

    fwrite(&iGarbage, sizeof(short int), 1, file);
    fwrite(&iGarbage, sizeof(short int), 1, file);
    fwrite(&cGarbage, sizeof(unsigned char), 1, file);
    fwrite(&iGarbage, sizeof(short int), 1, file);
    fwrite(&iGarbage, sizeof(short int), 1, file);

    fwrite(&width, sizeof(short int), 1, file);
    fwrite(&height, sizeof(short int), 1, file);
    fwrite(&pixelDepth, sizeof(unsigned char), 1, file);

    fwrite(&cGarbage, sizeof(unsigned char), 1, file);

// convert the image data from RGB(a) to BGR(A)
    if (mode >= 3)
        for (i=0; i < width * height * mode ; i+= mode) {
            aux = imageData[i];
            imageData[i] = imageData[i+2];
            imageData[i+2] = aux;
        }

// save the image data
    fwrite(imageData, sizeof(unsigned char),
           width * height * mode, file);
    fclose(file);

    return(TGA_OK);
}

// releases the memory used for the image
void TGA_Destroy(TGA_Info *info) {

    if (info != NULL) {
        if (info->imageData != NULL)
            free(info->imageData);
        free(info);
    }
}
