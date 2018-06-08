#ifndef MEDIA_DEFS_H
#define MEDIA_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif


#define FF_ERROR_OUTOFMEMORY        -1000

#define FF_ERROR_AVFORMAT_OPEN_INPUT   -1001  // Could not open input file
#define FF_ERROR_FIND_STREAM_INFO          -1002  // Failed to retrieve input stream information
#define FF_ERROR_FIND_CODEC          -1003  // Failed to find codec
#define FF_ERROR_OPEN_CODEC          -1004  // Failed to open codec

	//out
#define FF_ERROR_CREATE_OUTPUT_CONTEXT   -1005  // Failed to create output context
#define FF_ERROR_ALLOC_NEW_STREAM        -1006  // Failed to allocating new stream
#define FF_ERROR_CODEC_COPY                -1007  // Failed to copy codec from stream codec context
#define FF_ERROR_AVIO_ALLOC_CONTEXT -1008



#ifdef __cplusplus
}
#endif

#endif // MEDIA_DEFS_H
