
#include <stdio.h>
#include "libavformat/avformat.h"
#include "ff_common.h"

/*
FIX: H.264 in some container format (FLV, MP4, MKV etc.) need 
"h264_mp4toannexb" bitstream filter (BSF)
  *Add SPS,PPS in front of IDR frame
  *Add start code ("0,0,0,1") in front of NALU
H.264 in some container (MPEG2TS) don't need this BSF.
*/
//'1': Use H.264 Bitstream Filter 
#define USE_H264BSF 0



int ff_demuxer(const char *filename_out_v, const char *filename_out_a, const char *filename_in)
{
	AVOutputFormat *ofmt_a = NULL;
	AVOutputFormat *ofmt_v = NULL;

	AVFormatContext *ifmt_ctx = NULL;
	AVFormatContext *ofmt_ctx_a = NULL;
	AVFormatContext *ofmt_ctx_v = NULL;
	AVPacket pkt;
	int ret;
	size_t i;
	int videoindex=-1,audioindex=-1;
	int frame_index=0;


	av_register_all();

	ret = init_input_fmtctx(&ifmt_ctx, filename_in);
	if (ret < 0) {
		printf( "Could not open input file.");
		goto end;
	}

	// Outputs
	ret = avformat_alloc_output_context2(&ofmt_ctx_v, NULL, NULL, filename_out_v);
	if (!ofmt_ctx_v) {
		printf( "Could not create video output context\n");
		goto end;
	}
	ofmt_v = ofmt_ctx_v->oformat;

	ret = avformat_alloc_output_context2(&ofmt_ctx_a, NULL, NULL, filename_out_a);
	if (!ofmt_ctx_a) {
		printf( "Could not create audio output context\n");
		goto end;
	}
	ofmt_a = ofmt_ctx_a->oformat;

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
			// Create output AVStream according to input AVStream
			AVFormatContext *ofmt_ctx;
			AVStream *in_stream = ifmt_ctx->streams[i];
			AVStream *out_stream = NULL;
			
			if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
				videoindex=i;
				out_stream=avformat_new_stream(ofmt_ctx_v, in_stream->codec->codec);
				ofmt_ctx=ofmt_ctx_v;
			}
			else if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
				audioindex=i;
				out_stream=avformat_new_stream(ofmt_ctx_a, in_stream->codec->codec);
				ofmt_ctx=ofmt_ctx_a;
			}
			else{
				break;
			}
			
			if (!out_stream) {
				printf( "Failed allocating output stream\n");
				ret = AVERROR_UNKNOWN;
				goto end;
			}
			// Copy the settings of AVCodecContext
			if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
				printf( "Failed to copy context from input to output stream codec context\n");
				goto end;
			}
			out_stream->codec->codec_tag = 0;

			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
				out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	//Dump Formats
	printf("\nInput Video===========================\n");
	av_dump_format(ifmt_ctx, 0, filename_in, 0);
	printf("\nOutput Video==========================\n");
	av_dump_format(ofmt_ctx_v, 0, filename_out_v, 1);
	printf("\nOutput Audio==========================\n");
	av_dump_format(ofmt_ctx_a, 0, filename_out_a, 1);
	printf("\n======================================\n");
	
	// output files
	if (!(ofmt_v->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx_v->pb, filename_out_v, AVIO_FLAG_WRITE) < 0) {
			printf( "Could not open output file '%s'", filename_out_v);
			goto end;
		}
	}

	if (!(ofmt_a->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx_a->pb, filename_out_a, AVIO_FLAG_WRITE) < 0) {
			printf( "Could not open output file '%s'", filename_out_a);
			goto end;
		}
	}

	// Write file header
	if (avformat_write_header(ofmt_ctx_v, NULL) < 0) {
		printf( "Error occurred when opening video output file\n");
		goto end;
	}
	if (avformat_write_header(ofmt_ctx_a, NULL) < 0) {
		printf( "Error occurred when opening audio output file\n");
		goto end;
	}
	
#if USE_H264BSF
//	AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb"); 
	AVBSFContext *h264bsfc = NULL;
	av_bsf_alloc(av_bsf_get_by_name("h264_mp4toannexb"), &h264bsfc);
	av_bsf_init(h264bsfc);
#endif

	while (1) {
		AVFormatContext *ofmt_ctx;
		AVStream *in_stream, *out_stream;

		if (av_read_frame(ifmt_ctx, &pkt) < 0)
			break;
		in_stream  = ifmt_ctx->streams[pkt.stream_index];

		
		if(pkt.stream_index==videoindex){
			out_stream = ofmt_ctx_v->streams[0];
			ofmt_ctx=ofmt_ctx_v;
			printf("Write Video Packet. size:%d\tpts:%d\n",pkt.size,pkt.pts);
#if USE_H264BSF
			//av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
			{
				int nret = 0;
				nret = av_bsf_send_packet(h264bsfc, &pkt);
				if(nret<0)
				{
					printf("av_bsf_send_packet failed \n");
				}

				nret = av_bsf_receive_packet(h264bsfc, &pkt); 
				if(nret<0)
				{
					printf("av_bsf_receive_packet failed \n");
				}
			}
#endif
		}
		else if(pkt.stream_index==audioindex){
			out_stream = ofmt_ctx_a->streams[0];
			ofmt_ctx=ofmt_ctx_a;
			printf("Write Audio Packet. size:%d\tpts:%d\n",pkt.size,pkt.pts);
		}
		else{
			continue;
		}


		/* copy packet */
		// Convert PTS/DTS£©
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index=0;
		// Write
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
			printf( "Error muxing packet\n");
			break;
		}

		av_packet_unref(&pkt);
		frame_index++;
	}

#if USE_H264BSF
	//av_bitstream_filter_close(h264bsfc);  
	av_bsf_free(&h264bsfc); 
#endif

	//Write file trailer
	av_write_trailer(ofmt_ctx_a);
	av_write_trailer(ofmt_ctx_v);

end:
	avformat_close_input(&ifmt_ctx);
	/* close output */
	if (ofmt_ctx_a && !(ofmt_a->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx_a->pb);

	if (ofmt_ctx_v && !(ofmt_v->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx_v->pb);

	avformat_free_context(ofmt_ctx_a);
	avformat_free_context(ofmt_ctx_v);

	return ret;
}


