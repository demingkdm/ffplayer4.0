#include "media_input.h"
#include "media_defs.h"

int media_init_codec(MediaInput *mediain, int stream_index, int mediatype);


MediaInput *media_input_init(const char *filein, int *error)
{
	AVFormatContext *ifmt_ctx = NULL;
	MediaInput *mediain = NULL;

	int		astream_index = -1;
	int		vstream_index = -1;

	int ret = 0;
	int i = 0;

	if(!filein)  {*error=-1;  return NULL;}

	mediain = malloc(sizeof(MediaInput));
	if(!mediain) {*error=FF_ERROR_OUTOFMEMORY; return NULL;}
	memset(mediain, 0, sizeof(MediaInput));

	 ifmt_ctx = avformat_alloc_context();

	// open input
	if ((ret = avformat_open_input(&ifmt_ctx, filein, 0, 0)) < 0) {
		*error = FF_ERROR_AVFORMAT_OPEN_INPUT; return NULL;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		ret = FF_ERROR_FIND_STREAM_INFO; goto eout;
	}

	av_dump_format(ifmt_ctx, 0, filein, 0);

	for(i=0; i < ifmt_ctx->nb_streams; i++)
	{
		if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
			astream_index=i;
			continue;
		}
		if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			vstream_index=i;
			continue;
		}
	}

	mediain->ifmt_ctx = ifmt_ctx;
	if(astream_index!=-1)
	{
		ret = media_init_codec(mediain, astream_index, AVMEDIA_TYPE_AUDIO);
		if(ret<0) goto eout;
	}
	if(vstream_index!=-1)
	{
		ret = media_init_codec(mediain, vstream_index, AVMEDIA_TYPE_VIDEO);
		if(ret<0) goto eout;
	}

	return mediain;

eout:
	avformat_close_input(ifmt_ctx);
	ifmt_ctx = NULL;
	media_input_clean(mediain);
	*error = ret;
	return NULL;
}

void media_input_clean(MediaInput *mediain)
{
	if(mediain)
	{
		if(mediain->acodec_ctx)
		{
			avcodec_close(mediain->acodec_ctx);
			mediain->acodec_ctx = NULL;
		}
		if(mediain->vcodec_ctx)
		{
			avcodec_close(mediain->vcodec_ctx);
			mediain->vcodec_ctx = NULL;
		}

		if(mediain->ifmt_ctx)
		{
			avformat_close_input(&mediain->ifmt_ctx);
			mediain->ifmt_ctx = NULL;
		}

		free(mediain);
		mediain = NULL;
	}

	return;
}

int media_init_codec(MediaInput *mediain, int stream_index, int mediatype)
{
	AVCodecContext	*codec_ctx=NULL;
	AVCodec			*codec=NULL;
	int ret = 0;

	if(!mediain) return -1;
	if(stream_index<0) return -2;

	codec_ctx = mediain->ifmt_ctx->streams[stream_index]->codec;

	// Find the decoder for the audio stream
	codec=avcodec_find_decoder(codec_ctx->codec_id);
	if(codec==NULL){
		ret =FF_ERROR_FIND_CODEC;
		goto eout;
	}

	if(avcodec_open2(codec_ctx, codec,NULL)<0){
		ret =FF_ERROR_OPEN_CODEC;
		goto eout;
	}

	if(mediatype ==AVMEDIA_TYPE_AUDIO)
	{
		mediain->astream_index = stream_index;
		mediain->acodec_ctx = codec_ctx;
		mediain->acodec = codec;
	}
	else if(mediatype ==AVMEDIA_TYPE_VIDEO)
	{
		mediain->vstream_index = stream_index;
		mediain->vcodec_ctx = codec_ctx;
		mediain->vcodec = codec;
	}
	return 0;

eout:
	avcodec_close(codec_ctx);
	return ret;
}