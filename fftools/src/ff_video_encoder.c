
#include "ff_video_encoder.h"
#include "ff_common.h"
#include "libavcodec\avcodec.h"
#include "libavformat\avformat.h"
#include "libswscale\swscale.h"
#include "libavutil\pixfmt.h"
#include "libavutil\imgutils.h"


int encode_yuv2h264(const char *filename_out, const char *filename_in, 
					int width, int height, int framerate, int bitrate)
{
	AVFormatContext* ofmt_ctx;
	AVOutputFormat* ofmt;

	AVStream* ostream_v;
	AVCodecContext* codec_ctx;
	AVCodec* codec;

	uint8_t* picture_buf;
	AVFrame* picture;
	int size;
	int ret = 0;

	FILE *pf_in = NULL;	 
	int i = 0;

	AVPacket pkt;
	int y_size = 0;
	int got_picture=0;

	if(!filename_in || !filename_out) return -1;

	pf_in = fopen(filename_in, "rb");
	av_register_all();

	ofmt_ctx = avformat_alloc_context();
	ofmt = av_guess_format(NULL, filename_out, NULL);
	ofmt_ctx->oformat = ofmt;
	
	//method auto 2:
	//avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_file);
	//fmt = ofmt_ctx->oformat;


	if (avio_open(&ofmt_ctx->pb,filename_out, AVIO_FLAG_READ_WRITE) < 0)
	{
		printf("Failed in open output file!\n");
		return -1;
	}

	ostream_v = avformat_new_stream(ofmt_ctx, 0);
	if (ostream_v==NULL)
	{
		return -1;
	}
	codec_ctx = ostream_v->codec;
	codec_ctx->codec_id = ofmt->video_codec;
	codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	codec_ctx->width = width;  
	codec_ctx->height = height;
	codec_ctx->time_base.num = 1;  
	codec_ctx->time_base.den = framerate;  
	codec_ctx->bit_rate = bitrate;  
	codec_ctx->gop_size=250;
	//H264
	//codec_ctx->me_range = 16;
	//codec_ctx->max_qdiff = 4;
	codec_ctx->qmin = 10;
	codec_ctx->qmax = 51;
	//codec_ctx->qcompress = 0.6;
	
	// dump
	av_dump_format(ofmt_ctx, 0, filename_out, 1);

	codec = avcodec_find_encoder(codec_ctx->codec_id);
	if (!codec)
	{
		printf("Failed to find encoder!\n");
		return -1;
	}
	ret = avcodec_open2(codec_ctx, codec,NULL);
	if (ret< 0)
	{
		printf("Failed to open encoder!\n");
		return -1;
	}

	// init picture
	picture = av_frame_alloc();
	size = av_image_get_buffer_size(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, 1);
	picture_buf = (uint8_t *)av_malloc(size);
	av_image_fill_arrays(picture->data, picture->linesize, picture_buf, codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, 1);

	// output 

	avformat_write_header(ofmt_ctx,NULL);

	y_size = codec_ctx->width * codec_ctx->height;
	av_new_packet(&pkt,y_size*3);

	for (i=0; ; i++)
	{
		if (fread(picture_buf, 1, y_size*3/2, pf_in) < 0)
		{
			printf("Failed in read input file!\n");
			return -1;
		}
		else if(feof(pf_in))
		{
			break;
		}

		picture->data[0] = picture_buf;  // Y
		picture->data[1] = picture_buf+ y_size;  // U 
		picture->data[2] = picture_buf+ y_size*5/4; // V

		picture->pts=i;
		got_picture=0;
		
		ret = avcodec_encode_video2(codec_ctx, &pkt,picture, &got_picture);
		if(ret < 0)
		{
			printf("encode error!\n");
			return -1;
		}
		if (got_picture==1)
		{
			printf("encode succeeded!\n");
			pkt.stream_index = ostream_v->index;
			ret = av_write_frame(ofmt_ctx, &pkt);
			//av_packet_unref(&pkt);
			av_packet_unref(&pkt);
		}
	}

	//Flush Encoder
	ret = flush_encoder(ofmt_ctx,0);
	if (ret < 0) {
		printf("Flushing encoder failed\n");
		return -1;
	}

	av_write_trailer(ofmt_ctx);

	//clean
	if (ostream_v)
	{
		avcodec_close(ostream_v->codec);
		av_free(picture);
		av_free(picture_buf);
	}
	avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);

	fclose(pf_in);

	return 0;
}



