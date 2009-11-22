#ifndef BBQ_H
#define BBQ_H

#ifdef __cplusplus
extern "C" {
#endif

struct OpenGLTexture
{
	unsigned int width;
	unsigned int height;
	int internal_format;
	int format;
	int type;
	int num_mips;
	void** pixels;
};

void* bbq_load(const char* filename);
void bbq_free(void* ptr);

#ifdef __cplusplus
}
#endif


#endif
