#ifndef FF_COMMON_H
#define FF_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"

#define FF_ERROR_OUTOFMEMORY        -1000
#define FF_ERROR_AVFORMAT_OPEN_INPUT   -1001  // Could not open input file
#define FF_ERROR_FIND_STREAM_INFO          -1002  // Failed to retrieve input stream information
#define FF_ERROR_CREATE_OUTPUT_CONTEXT   -1005  // Failed to create output context
#define FF_ERROR_ALLOC_NEW_STREAM        -1006  // Failed to allocating new stream
#define FF_ERROR_CODEC_COPY                -1007  // Failed to copy codec from stream codec context
#define FF_ERROR_AVIO_ALLOC_CONTEXT -1008


typedef struct avio_buffer_s{
	AVIOContext * avio;
	uint8_t *buf;
	int buf_size;
}AVIOBuffer;

typedef int (*buffer_read)(void *opaque, uint8_t *buf, int buf_size);
typedef int (*buffer_write)(void *opaque, uint8_t *buf, int buf_size);
typedef int64_t (*buffer_seek)(void *opaque, int64_t offset, int whence);

// input format context
void clean_avio_buffer(AVIOBuffer *aviobuffer);
int init_input_fmtctx_by_avio(AVFormatContext **ifmt_ctx, AVIOBuffer *aviobuffer, int bufsize, void *opaque,
						buffer_read pread, buffer_seek pseek);
int init_input_fmtctx(AVFormatContext **ifmt_ctx, const char *filename_in);

// output format context
int init_output_fmtctx_by_fmtctx(AVFormatContext **ofmt_ctx, AVFormatContext *ifmt_ctx, const char *filename_out);

// input: ifmt_ctx, mediatype
// output: ofmt_ctx, ostream_index, istream_index
int add_stream_from_fmtctx(AVFormatContext *ofmt_ctx, int *ostream_index, int *istream_index,
                AVFormatContext *ifmt_ctx, int mediatype);

int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index);

#ifdef __cplusplus
}
#endif

#endif // FF_COMMON_H
