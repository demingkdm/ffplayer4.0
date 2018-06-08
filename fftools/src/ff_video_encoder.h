#ifndef FF_VIDEO_ENCODER_H
#define FF_VIDEO_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif


// example: encode_yuv2h264("src01.h264", "src01_480x272.yuv", 480, 272, 25, 400000);
int encode_yuv2h264(const char *filename_out, const char *filename_in, 
							int width, int height, int framerate, int bitrate);

#ifdef __cplusplus
}
#endif

#endif // FF_VIDEO_ENCODER_H
