#include "ff_muxer.h"
#include "ff_common.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"


//FIX: H.264 in some container format (FLV, MP4, MKV etc.) need "h264_mp4toannexb" bitstream filter (BSF)
//  Add SPS,PPS in front of IDR frame;
//  Add start code ("0,0,0,1") in front of NALU;
//H.264 in some container (MPEG2TS) don't need this BSF.
//'1': Use H.264 Bitstream Filter
#define USE_H264BSF 0

// FIX:AAC in some container format (FLV, MP4, MKV etc.) need "aac_adtstoasc" bitstream filter (BSF)
//'1': Use AAC Bitstream Filter
#define USE_AACBSF 0

//H.264裸流没有PTS，因此必须手动写入PTS

typedef struct ff_muxer_s {
	// input 
	AVFormatContext *ifmt_ctx_v;
	AVFormatContext *ifmt_ctx_a;

	int istream_index_v;
	int istream_index_a;

	//output
	AVFormatContext *ofmt_ctx;
	int ostream_index_v;
	int ostream_index_a;
}FFMuxer;

void ffmuxer_init(FFMuxer *muxer)
{
	if(!muxer) return;

	muxer->ifmt_ctx_v = NULL;
	muxer->ifmt_ctx_a = NULL;
	muxer->ofmt_ctx = NULL;

	muxer->istream_index_v = -1;
	muxer->istream_index_a = -1;
	muxer->ostream_index_v = -1;
	muxer->ostream_index_a = -1;
}

void ffmuxer_clean(FFMuxer *muxer)
{
	if(muxer)
	{
		if(muxer->ifmt_ctx_v) avformat_close_input(&muxer->ifmt_ctx_v);
		if(muxer->ifmt_ctx_a) avformat_close_input(&muxer->ifmt_ctx_a);
		if(muxer->ofmt_ctx) avformat_free_context(muxer->ofmt_ctx);
		muxer = NULL;
	}
}


int init_input_fmtctxs(FFMuxer *muxer, const char *filename_in_v, const char *filename_in_a)
{
	int ret = 0;
	if(!muxer) return -1;
	if(!filename_in_a || !filename_in_v) return -2;

	if ((ret = init_input_fmtctx(&muxer->ifmt_ctx_v, filename_in_v)) < 0) {
		goto eout;
	}

	if ((ret = init_input_fmtctx(&muxer->ifmt_ctx_a, filename_in_a)) < 0) {
		goto eout;
	}

	// test dump
	printf("Input Information=====================\n");
	av_dump_format(muxer->ifmt_ctx_v, 0, filename_in_v, 0);
	av_dump_format(muxer->ifmt_ctx_a, 0, filename_in_a, 0);
	printf("======================================\n");
	return 0;

eout:
	ffmuxer_clean(muxer);
	return ret;
}

int init_mux_output_fmtctx(FFMuxer *muxer, const char *filename_out)
{
	int ret = 0;

	if(!muxer) return -1;
	if(!filename_out) return -2;

	avformat_alloc_output_context2(&muxer->ofmt_ctx, NULL, NULL, filename_out);
	if (!muxer->ofmt_ctx) {
		ret = FF_ERROR_CREATE_OUTPUT_CONTEXT; goto eout;
	}

	ret = add_stream_from_fmtctx(muxer->ofmt_ctx, &muxer->ostream_index_v, &muxer->istream_index_v, 
						muxer->ifmt_ctx_v, AVMEDIA_TYPE_VIDEO);
    if(ret<0) goto eout;

	ret = add_stream_from_fmtctx(muxer->ofmt_ctx, &muxer->ostream_index_a, &muxer->istream_index_a, 
						muxer->ifmt_ctx_a, AVMEDIA_TYPE_AUDIO);
//	if(ret<0) goto eout; //may no audio

	// test dump
	printf("Output Information====================\n");
	av_dump_format(muxer->ofmt_ctx, 0, filename_out, 1);
	printf("======================================\n");

	return 0;
eout:
	ffmuxer_clean(muxer);
	return ret;
}

//pkt[out], cur_pts[out],frame_index[io]
int read_frame_from_istream(AVPacket *pkt, int64_t *cur_pts, int *frame_index,
				AVFormatContext *ifmt_ctx, AVStream **istream, int istream_index)
{
	if(!pkt) return -1;
	if(!cur_pts || !frame_index) return -2;
	if(!ifmt_ctx ) return -3;

	if(av_read_frame(ifmt_ctx, pkt) >= 0)
	{
		do{
			*istream  = ifmt_ctx->streams[pkt->stream_index];

			if(pkt->stream_index==istream_index)
			{
				//if no pts(Example: Raw H.264)
				if(pkt->pts==AV_NOPTS_VALUE)
				{
					AVRational time_base1=(*istream)->time_base;
					//Duration between 2 frames (us)
					int64_t calc_duration=(int64_t)((double)AV_TIME_BASE/av_q2d((*istream)->r_frame_rate));
					
					pkt->pts=(int64_t)((double)(*frame_index * calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE));
					pkt->dts=pkt->pts;
					pkt->duration=(int)((double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE));
					*frame_index++;
				}
				*cur_pts=pkt->pts;
				break;
			}
		}while(av_read_frame(ifmt_ctx, pkt) >= 0);

		return 0;
	}
	else
	{
		return -10; //error or end, break out
	}
}

int ff_muxer(const char *filename_out, const char *filename_in_v, const char *filename_in_a)
{
	AVOutputFormat *ofmt = NULL;
	FFMuxer muxer;
	AVPacket pkt;
	int ret = 0;

	int frame_index=0;

	int64_t cur_pts_v=0;
	int64_t cur_pts_a=0;

	AVBSFContext* h264bsfc =  NULL;
	AVBSFContext* aacbsfc =  NULL;

	av_register_all();
    ffmuxer_init(&muxer);

	ret = init_input_fmtctxs(&muxer, filename_in_v, filename_in_a);
	if(ret<0) return ret;

	ret = init_mux_output_fmtctx(&muxer, filename_out);
	if(ret<0) return ret;

	ofmt = muxer.ofmt_ctx->oformat;

	// Open output file
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&muxer.ofmt_ctx->pb, filename_out, AVIO_FLAG_WRITE) < 0) {
			printf( "Could not open output file '%s'", filename_out);
			goto end;
		}
	}

	// Write file header
	if (avformat_write_header(muxer.ofmt_ctx, NULL) < 0) {
		printf( "Error occurred when opening output file\n");
		goto end;
	}


	//FIX: Bitstream Filter
	//h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");
	//aacbsfc =  av_bitstream_filter_init("aac_adtstoasc");
	av_bsf_alloc(av_bsf_get_by_name("h264_mp4toannexb"), &h264bsfc);
	av_bsf_init(h264bsfc);
	av_bsf_alloc(av_bsf_get_by_name("aac_adtstoasc"), &aacbsfc);
	av_bsf_init(aacbsfc);

	while (1)
	{
		AVFormatContext *ifmt_ctx;
		AVStream *istream = NULL;
		AVStream *ostream = NULL;

		// Get an AVPacket
		if(av_compare_ts(cur_pts_v, muxer.ifmt_ctx_v->streams[muxer.istream_index_v]->time_base,
						cur_pts_a, muxer.ifmt_ctx_a->streams[muxer.istream_index_a]->time_base) <= 0)
		{
			// video
			ret = read_frame_from_istream(&pkt, &cur_pts_v, &frame_index, muxer.ifmt_ctx_v, &istream, muxer.istream_index_v);
			if(ret>=0){
				ostream = muxer.ofmt_ctx->streams[muxer.ostream_index_v];
			}
			else
				break;
		}
		else
		{
			ret = read_frame_from_istream(&pkt, &cur_pts_a, &frame_index, muxer.ifmt_ctx_a, &istream, muxer.istream_index_a);
			if(ret>=0){
				ostream = muxer.ofmt_ctx->streams[muxer.ostream_index_a];
			}
			else
				break;
		}

		//FIX:Bitstream Filter
		if(ostream->codec->codec_type==AVMEDIA_TYPE_VIDEO && 
			ostream->codec->codec_id==AV_CODEC_ID_H264)
		{
			//av_bitstream_filter_filter(h264bsfc, istream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
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

		if(ostream->codec->codec_type==AVMEDIA_TYPE_AUDIO && 
			ostream->codec->codec_id==AV_CODEC_ID_AAC)
		{
			//av_bitstream_filter_filter(aacbsfc, ostream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
			int nret = 0;
			nret = av_bsf_send_packet(aacbsfc, &pkt);
			if(nret<0)
			{
				printf("av_bsf_send_packet failed \n");
			}

			nret = av_bsf_receive_packet(aacbsfc, &pkt); 
			if(nret<0)
			{
				printf("av_bsf_receive_packet failed \n");
			}
		}

		/* copy packet */
		// Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, istream->time_base, ostream->time_base, (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, istream->time_base, ostream->time_base,  (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, istream->time_base, ostream->time_base);
		pkt.pos = -1;
		pkt.stream_index=ostream->index;

		printf("Write 1 Packet. size:%5d\tpts:%8d, stream_index=%d\n",pkt.size,pkt.pts, pkt.stream_index);
		// Write
		ret = av_interleaved_write_frame(muxer.ofmt_ctx, &pkt);
		if ( ret< 0) {
			printf( "Error muxing packet\n");
			break;
		}
		av_packet_unref(&pkt);

	}

	//av_bitstream_filter_close(h264bsfc);
	//av_bitstream_filter_close(aacbsfc);
	av_bsf_free(&h264bsfc); 
	av_bsf_free(&aacbsfc); 

	// Write file trailer
	av_write_trailer(muxer.ofmt_ctx);

end:
	if (muxer.ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(muxer.ofmt_ctx->pb);

	ffmuxer_clean(&muxer);

	return ret;
}
