#include "bbq.h"
#include <stdio.h>
#include <stdlib.h>

// helper
// TODO: use mmap instead?
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

// Memory layout
//
// unsigned int* base;
// base begining of file in memory.
// base + 0                       | num_offsets
// base + 1                       | offset 0
// base + 2                       | offset 1
// ...                         
// base + num_offsets             | offset n-1
// base + num_offsets + 1         | num_offsets   // again so we can figure out what memory to free.
// base + num_offsets + 2         | start of cooked data
//

void* bbq_load(const char* filename)
{
	unsigned char* p;
	int size = LoadFileToMemory(filename, &p);
	if(size <= 0)
		return 0;

	unsigned int* base = (unsigned int*)p;
	unsigned int num_offsets = *base;
	unsigned int* offset = base + 1;

	// get the start of the actual data, (the 2 is to skip past both num_offsets values)
	unsigned char* data = (unsigned char*)(base + num_offsets + 2);

	// fix up the pointers
	while ((unsigned char*)(offset + 1) < data)
	{
		unsigned long* ptr = (unsigned long*)(data + *offset);
		*ptr = (unsigned long)((unsigned char*)ptr + *ptr);
		offset++;
	}
	
	return data;
}

void bbq_free(void* ptr)
{
	unsigned int* p = (unsigned int*)ptr;
	unsigned char* base = (unsigned char*)(p - (*(p-1) + 2));
	free(base);
}
