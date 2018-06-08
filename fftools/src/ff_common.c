#include "ff_common.h"


int init_input_fmtctx(AVFormatContext **ifmt_ctx, const char *filename_in)
{
    int ret = 0;
    if(!filename_in) return -1;

    if ((ret = avformat_open_input(ifmt_ctx, filename_in, 0, 0)) < 0) {
        ret = FF_ERROR_AVFORMAT_OPEN_INPUT; return ret;
    }
    if ((ret = avformat_find_stream_info(*ifmt_ctx, 0)) < 0) {
        ret = FF_ERROR_FIND_STREAM_INFO; goto eout;
    }
    return 0;

eout:
    avformat_close_input(ifmt_ctx);
    return ret;
}

void clean_avio_buffer(AVIOBuffer *aviobuffer)
{
	if(clean_avio_buffer)
	{
		if(aviobuffer->buf)
		{
			av_free(aviobuffer->buf);
			aviobuffer->buf = NULL;
		}

		if(aviobuffer->avio)
		{
			av_free(aviobuffer->avio);
			aviobuffer->avio = NULL;
		}
	}
}

int init_input_fmtctx_by_avio(AVFormatContext **ifmt_ctx, AVIOBuffer *aviobuffer, int bufsize, void *opaque,
			buffer_read pread, buffer_seek pseek)
{
	int ret = 0;

	if(!aviobuffer) return -1;

	aviobuffer->buf_size = bufsize;
	aviobuffer->buf = (uint8_t *)av_malloc(bufsize);
	if(!aviobuffer->buf) { 
		ret = FF_ERROR_OUTOFMEMORY; goto eout;
	}

	aviobuffer->avio = avio_alloc_context(aviobuffer->buf, bufsize, 0, opaque, pread, NULL, pseek);
	if (!aviobuffer->avio) {
		ret = FF_ERROR_AVIO_ALLOC_CONTEXT; goto eout;
	}

	*ifmt_ctx = avformat_alloc_context();
	(*ifmt_ctx)->pb = aviobuffer->avio;
	(*ifmt_ctx)->flags=AVFMT_FLAG_CUSTOM_IO;

	if (avformat_open_input(ifmt_ctx, "NULL", NULL, NULL) < 0) {
		ret = FF_ERROR_AVFORMAT_OPEN_INPUT; goto eout;
	}

	if ((ret = avformat_find_stream_info(*ifmt_ctx, NULL)) < 0) {
		ret = FF_ERROR_FIND_STREAM_INFO; goto eout;
	}

	return 0;

eout:
	clean_avio_buffer(aviobuffer);
	avformat_close_input(ifmt_ctx);
	return ret;
}


int init_output_fmtctx_by_fmtctx(AVFormatContext **ofmt_ctx, AVFormatContext *ifmt_ctx, const char *filename_out)
{
	AVOutputFormat *ofmt = NULL;
	int ret = 0;
	int i = 0;

	if(!ifmt_ctx) return -1;
	if(!filename_out) return -2;

	avformat_alloc_output_context2(ofmt_ctx, NULL, NULL, filename_out);
	if (!*ofmt_ctx) {
		ret = FF_ERROR_CREATE_OUTPUT_CONTEXT; goto eout;
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++)
	{
		AVStream *in_stream = ifmt_ctx->streams[i];
		AVStream *out_stream = NULL;

		// some container only support the three media_type
		if(in_stream->codec->codec_type!=AVMEDIA_TYPE_VIDEO && 
			in_stream->codec->codec_type!=AVMEDIA_TYPE_AUDIO &&
			in_stream->codec->codec_type!=AVMEDIA_TYPE_SUBTITLE)
			continue;

		out_stream = avformat_new_stream(*ofmt_ctx, in_stream->codec->codec);
		if (!out_stream) {
			ret = FF_ERROR_ALLOC_NEW_STREAM; goto eout;
		}

		if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
			ret = FF_ERROR_CODEC_COPY;	goto eout;
		}
		out_stream->codec->codec_tag = 0; //fourcc (LSB first, so "ABCD" -> ('D'<<24) + ('C'<<16) + ('B'<<8) + 'A').

		if ((*ofmt_ctx)->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

eout:
	return ret;
}

int add_stream_from_fmtctx(AVFormatContext *ofmt_ctx, int *ostream_index, int *istream_index,
                AVFormatContext *ifmt_ctx, int mediatype)
{
    int ret = 0;
    int i = 0;
    if(!ofmt_ctx) return -1;
    if(!ifmt_ctx) return -2;
    if(!ostream_index || !istream_index) return -3;

    for (i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        // Create output AVStream according to input AVStream
        if(ifmt_ctx->streams[i]->codec->codec_type==mediatype)
        {
            AVStream *in_stream = NULL;
            AVStream *out_stream =NULL;

            *istream_index=i;
            in_stream = ifmt_ctx->streams[i];
            out_stream =avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
            if (!out_stream) {
                ret = FF_ERROR_ALLOC_NEW_STREAM; goto eout;
            }
            *ostream_index = out_stream->index;

            // Copy the settings of AVCodecContext
            if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                ret = FF_ERROR_CODEC_COPY; goto eout;
            }
            out_stream->codec->codec_tag = 0;
            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            break;
        }
    }

eout:
    return ret;
}

// mux 
int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index)
{
	int ret;
	int got_frame;
	AVPacket enc_pkt;

	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & AV_CODEC_CAP_DELAY))
		return 0;

	while (1)
	{
		av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n", stream_index);

		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);

		ret = avcodec_encode_video2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt, NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame)
		{ 
			ret=0;
			break;
		}

		/* prepare packet for muxing */
		enc_pkt.stream_index = stream_index;
		enc_pkt.dts = av_rescale_q_rnd(enc_pkt.dts, fmt_ctx->streams[stream_index]->codec->time_base,
			fmt_ctx->streams[stream_index]->time_base, (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		enc_pkt.pts = av_rescale_q_rnd(enc_pkt.pts, fmt_ctx->streams[stream_index]->codec->time_base,
			fmt_ctx->streams[stream_index]->time_base, (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		enc_pkt.duration = av_rescale_q(enc_pkt.duration, fmt_ctx->streams[stream_index]->codec->time_base,
			fmt_ctx->streams[stream_index]->time_base);

		av_log(NULL, AV_LOG_DEBUG, "Muxing 1 frame\n");

		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}

	return ret;
}
