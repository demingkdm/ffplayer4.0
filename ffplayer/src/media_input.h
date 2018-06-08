#ifndef MEDIA_INPUT_H
#define MEDIA_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "SDL.h"
#include "SDL_thread.h"

typedef struct media_input_s 
{
	AVFormatContext *ifmt_ctx;

	// audio
	AVCodecContext  *acodec_ctx;
	AVCodec			  *acodec;
	int		astream_index;

	// video
	AVCodecContext  *vcodec_ctx;
	AVCodec			  *vcodec;
	int		vstream_index;

} MediaInput;


MediaInput *media_input_init(const char *filein, int *error);
void media_input_clean(MediaInput *mediain);



#ifdef __cplusplus
}
#endif

#endif // MEDIA_INPUT_H
