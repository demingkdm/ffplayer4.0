
#include "ff_transcache.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "ff_common.h"
#include "stream_io.h"



#define FF_BUFFER_SIZE   32*1024

int ff_transcache(const char *filename_out, const char *filename_in)
{
	int ret;
	AVFormatContext* ifmt_ctx=NULL;
	AVFormatContext* ofmt_ctx=NULL;
	AVPacket packet,enc_pkt;

	AVFrame *frame = NULL;
	enum AVMediaType type;
	unsigned int stream_index;
	unsigned int i=0;
	int got_frame;
	int enc_got_frame;

	AVStream *out_stream=NULL;
	AVStream *in_stream=NULL;
	AVCodecContext *dec_ctx=NULL;
	AVCodecContext *enc_ctx=NULL;
	AVCodec *encoder=NULL;

	//unsigned char* inbuffer=NULL;
	unsigned char* outbuffer=NULL;

	//AVIOContext *avio_in = NULL;
	AVIOContext *avio_out = NULL;
	AVIOBuffer aviobuf_in;
	FILE *fp_in = NULL;
	FILE *fp_out = NULL;

	fp_in = fopen(filename_in, "rb"); 
	if(!fp_in) return -10;
	fp_out=fopen(filename_out,"wb+"); 
	if(!fp_out) return -11;

	av_register_all();
	
	// input
	ret = init_input_fmtctx_by_avio(&ifmt_ctx, &aviobuf_in, FF_BUFFER_SIZE, fp_in, stream_read, stream_seek);
	if(ret<0) goto end;

	for (i = 0; i < ifmt_ctx->nb_streams; i++) 
	{
		AVStream *stream;
		AVCodecContext *codec_ctx;
		stream = ifmt_ctx->streams[i];
		codec_ctx = stream->codec;

		/* Reencode video & audio and remux subtitles etc. */
		if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO ||
			codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			AVCodec*codec = avcodec_find_decoder(codec_ctx->codec_id);
			if(!codec) {
				fprintf(stderr, "Unsupported codec!\n");
				return -102;
			}

			ret = avcodec_open2(codec_ctx, codec, NULL);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
				return ret;
			}
		}
	}

	av_dump_format(ifmt_ctx, 0, "whatever", 0);

	// output
	avformat_alloc_output_context2(&ofmt_ctx, NULL, "mpegts", NULL);
	//avformat_alloc_output_context2(&ofmt_ctx, NULL, "matroska", NULL);
	if(!ofmt_ctx) { ret = -123;goto end;}
	outbuffer=(unsigned char*)av_malloc(FF_BUFFER_SIZE);

	avio_out =avio_alloc_context(outbuffer, FF_BUFFER_SIZE,0,fp_out,NULL, stream_write, NULL);
	if(avio_out==NULL)
		goto end;

	ofmt_ctx->pb=avio_out;
	ofmt_ctx->flags=AVFMT_FLAG_CUSTOM_IO;

	for (i = 0; i < ifmt_ctx->nb_streams; i++) 
	{
		out_stream = avformat_new_stream(ofmt_ctx, NULL);
		if (!out_stream) {
			av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
			return AVERROR_UNKNOWN;
		}
		in_stream = ifmt_ctx->streams[i];
		dec_ctx = in_stream->codec;
		enc_ctx = out_stream->codec;

		if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
			enc_ctx->height = dec_ctx->height;
			enc_ctx->width = dec_ctx->width;
			enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
			enc_ctx->pix_fmt = encoder->pix_fmts[0];
			enc_ctx->time_base = dec_ctx->time_base;
			//enc_ctx->time_base.num = 1;
			//enc_ctx->time_base.den = 25;
			//H264的必备选项，没有就会错
			enc_ctx->me_range=16;
			enc_ctx->max_qdiff = 4;
			enc_ctx->qmin = 10;
			enc_ctx->qmax = 51;
			enc_ctx->qcompress = 0.6;
			enc_ctx->refs=3;
			enc_ctx->bit_rate = 500000;

			ret = avcodec_open2(enc_ctx, encoder, NULL);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
				goto end;
			}
		}
		else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) 
		{
			av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
			return AVERROR_INVALIDDATA;
		} 
		else 
		{
			/* if this stream must be remuxed */
			ret = avcodec_copy_context(ofmt_ctx->streams[i]->codec,
				ifmt_ctx->streams[i]->codec);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Copying stream context failed\n");
				return ret;
			}
		}
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
	av_dump_format(ofmt_ctx, 0, "whatever", 1);
	
	/* init muxer, write output file header */
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
		return ret;
	}


	/* read all packets */
	for (i=0;;i++) 
	{
		if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
			break;

		stream_index = packet.stream_index;
		if(stream_index!=0)
			continue;

		type = ifmt_ctx->streams[packet.stream_index]->codec->codec_type;
		av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n",  stream_index);

		av_log(NULL, AV_LOG_DEBUG, "Going to reencode the frame\n");
		frame = av_frame_alloc();
		if (!frame) {
			ret = AVERROR(ENOMEM);
			break;
		}
		packet.dts = av_rescale_q_rnd(packet.dts, ifmt_ctx->streams[stream_index]->time_base,
			ifmt_ctx->streams[stream_index]->codec->time_base,
			(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		packet.pts = av_rescale_q_rnd(packet.pts, ifmt_ctx->streams[stream_index]->time_base,
			ifmt_ctx->streams[stream_index]->codec->time_base,
			(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));

		ret = avcodec_decode_video2(ifmt_ctx->streams[stream_index]->codec, frame, &got_frame, &packet);
		if (ret < 0) {
			av_frame_free(&frame);
			av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
			break;
		}
		printf("Decode 1 Packet\tsize:%d\tpts:%d\n",packet.size,packet.pts);

		if (got_frame) 
		{
			frame->pts = av_frame_get_best_effort_timestamp(frame);
			frame->pict_type=AV_PICTURE_TYPE_NONE;

			enc_pkt.data = NULL;
			enc_pkt.size = 0;
			av_init_packet(&enc_pkt);

			ret = avcodec_encode_video2 (ofmt_ctx->streams[stream_index]->codec, &enc_pkt, frame, &enc_got_frame);
			printf("Encode 1 Packet\tsize:%d\tpts:%d\n",enc_pkt.size,enc_pkt.pts);

			av_frame_free(&frame);

			if (ret < 0)
				goto end;

			if (!enc_got_frame)
				continue;

			// pts dts
			enc_pkt.stream_index = stream_index;
			enc_pkt.dts = av_rescale_q_rnd(enc_pkt.dts, ofmt_ctx->streams[stream_index]->codec->time_base,
				ofmt_ctx->streams[stream_index]->time_base,
				(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
			enc_pkt.pts = av_rescale_q_rnd(enc_pkt.pts, ofmt_ctx->streams[stream_index]->codec->time_base,
				ofmt_ctx->streams[stream_index]->time_base,
				(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
			enc_pkt.duration = av_rescale_q(enc_pkt.duration, ofmt_ctx->streams[stream_index]->codec->time_base,
				ofmt_ctx->streams[stream_index]->time_base);

			av_log(NULL, AV_LOG_INFO, "Muxing frame %d\n",i);

			// mux encoded frame 
			av_write_frame(ofmt_ctx,&enc_pkt);
			if (ret < 0)
				goto end;
		} 
		else
		{
			av_frame_free(&frame);
		}

		av_packet_unref(&packet);
	}

	/* flush encoders */
	ret = flush_encoder(ofmt_ctx,0);
	if (ret < 0) {
		printf("Flushing encoder failed\n");
		return -1;
	}
	av_write_trailer(ofmt_ctx);

end:
	clean_avio_buffer(&aviobuf_in);
	//av_freep(avio_in);
	//av_free(inbuffer);
	av_freep(avio_out);
	av_free(outbuffer);

	av_packet_unref(&packet);
	av_frame_free(&frame);
	avformat_close_input(&ifmt_ctx);
	avformat_free_context(ofmt_ctx);

	_fcloseall();

	if (ret < 0)
		av_log(NULL, AV_LOG_ERROR, "Error occurred\n");
	return (ret? 1:0);
}




