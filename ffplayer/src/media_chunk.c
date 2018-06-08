#include "media_chunk.h"
#include "media_defs.h"

//
ChunkData * chunk_data_init(uint32_t bufsize, int *error)
{
	ChunkData * chunk = NULL;

	chunk = (ChunkData *)malloc(sizeof(ChunkData));
	if(!chunk) {*error=FF_ERROR_OUTOFMEMORY; return NULL;}

	chunk->buf = (uint8_t *)av_malloc(bufsize);
	if(!chunk->buf)  {*error=FF_ERROR_OUTOFMEMORY; goto eout;}

	chunk->bufsize = bufsize;
	chunk->data = chunk->buf;
	chunk->data_len = bufsize;
	return chunk;

eout:
	if(chunk) {free(chunk); chunk=NULL;}
	return 0;
}

int chunk_data_reset(ChunkData *chunk, uint32_t datasize)
{
	if(!chunk) return -1;
	if(!chunk->buf) return -2;

	chunk->data = chunk->buf;
	chunk->data_len = datasize;
	return 0;
}

void chunk_data_clean(ChunkData *chunk)
{
	if(chunk)
	{
		if(chunk->buf) 
		{
			av_free(chunk->buf);
			chunk->buf = NULL;
			chunk->bufsize = 0;
			chunk->data = NULL;		
			chunk->data_len = 0;
		}

		free(chunk);
		chunk = NULL;
	}
	return;
}