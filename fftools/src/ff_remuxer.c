

#include "ff_remuxer.h"
#include <stdio.h>
#include "libavformat/avformat.h"
#include "ff_common.h"



int ff_remux(const char *filename_out, const char *filename_in)
{
	AVOutputFormat *ofmt = NULL;

	AVFormatContext *ifmt_ctx = NULL;
	AVFormatContext *ofmt_ctx = NULL;
	AVPacket pkt;

	int ret;
	int frame_index=0;
	AVBSFContext* h264bsfc = NULL;

	av_register_all();

	// Input
	ret = init_input_fmtctx(&ifmt_ctx, filename_in);
	if(ret<0) goto end;
	// dump
	av_dump_format(ifmt_ctx, 0, filename_in, 0);

	// Output
	ret = init_output_fmtctx_by_fmtctx(&ofmt_ctx, ifmt_ctx, filename_out);
	if(ret<0) goto end;
	ofmt = ofmt_ctx->oformat;
	// dump
	av_dump_format(ofmt_ctx, 0, filename_out, 1);

	// Open output file
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, filename_out, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf( "Could not open output file '%s'", filename_out);
			goto end;
		}
	}

	// Write file header
	if (avformat_write_header(ofmt_ctx, NULL) < 0) {
		printf( "Error occurred when opening output file\n");
		goto end;
	}

	av_bsf_alloc(av_bsf_get_by_name("h264_mp4toannexb"), &h264bsfc);
	av_bsf_init(h264bsfc);

	while (1) {
		AVStream *in_stream;
		AVStream *out_stream;

		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;

		in_stream  = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];

		if(in_stream->codec->codec_type!=AVMEDIA_TYPE_VIDEO && 
			in_stream->codec->codec_type!=AVMEDIA_TYPE_AUDIO &&
			in_stream->codec->codec_type!=AVMEDIA_TYPE_SUBTITLE)
			continue;

		if(in_stream->codec->codec_type==AVMEDIA_TYPE_VIDEO &&
			in_stream->codec->codec_id==AV_CODEC_ID_H264)
		{
			//av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
			ret = av_bsf_send_packet(h264bsfc, &pkt);
			if(ret<0)
			{
				printf("av_bsf_send_packet failed \n");
			}
				 
			ret = av_bsf_receive_packet(h264bsfc, &pkt); 
			if(ret<0)
			{
				printf("av_bsf_receive_packet failed \n");
			}
		}

		// Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;

		// Write
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
			printf( "Error muxing packet\n");
			break;
		}

		printf("Write %8d frames to output file\n",frame_index);
		av_packet_unref(&pkt);
		frame_index++;
	}

	if(h264bsfc) av_bsf_free(&h264bsfc); 
	//Write file trailer
	av_write_trailer(ofmt_ctx);

end:
	avformat_close_input(&ifmt_ctx);
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);

	return ret;
}

