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
// p begining of file in memory.
// p + 0 : num_pointers
// p + 4 : offset 0
// p + 8 : offset 1
// ...
// p + ((num_pointers - 1) * 4) : offset n-1
// p + (num_pointers * 4) : num_pointers   // again so we can figure out what memory to free.
// p + ((num_pointers + 1) * 4) : start of cooked data
//

void* bbq_load(const char* filename)
{
	unsigned char* p;
	int size = LoadFileToMemory(filename, &p);
	if(size <= 0)
		return 0;

	// get the start of the pointer table
	unsigned int* ptr_table = (unsigned int*)p;
	unsigned int num_ptrs = *ptr_table;
	ptr_table++;

	// get the start of the actual data, (the 2 is to skip past both num_pointer values)
	unsigned char* base = p + ((num_ptrs + 2) * sizeof(unsigned int));

	// fix up the pointers
	while ((ptr_table + 1) < (unsigned int*)base)
	{
		unsigned int* ptr = (unsigned int*)(base + *ptr_table);
		*ptr = (unsigned int)((unsigned char*)ptr + *ptr);
		ptr_table++;
	}
	
	return base;
}

void bbq_free(void* ptr)
{
	unsigned int* p = (unsigned int*)ptr;
	unsigned char* base = (unsigned char*)(p - (*(p-1) + 2));
	free(base);
}
