#ifndef MEDIA_CHUNK_H
#define MEDIA_CHUNK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"

//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
typedef struct audio_chunk_s
{
	uint8_t   *buf;
	int        bufsize;  //total size

	int        data_len; //valid data len
	uint8_t   *data;    //real data, only pointer to buf
}ChunkData;


ChunkData * chunk_data_init(uint32_t bufsize, int *error);
void chunk_data_clean(ChunkData *chunk);
int chunk_data_reset(ChunkData *chunk, uint32_t datasize);


#ifdef __cplusplus
}
#endif

#endif // MEDIA_CHUNK_H
