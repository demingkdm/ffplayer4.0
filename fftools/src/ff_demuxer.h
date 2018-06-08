#ifndef FF_DEMUXER_H
#define FF_DEMUXER_H

#ifdef __cplusplus
extern "C" {
#endif

// demux one video file to video and audio file, for example:
//char *filename_in  = "cuc_ieschool.ts";
//char *filename_out_v = "cuc_ieschool.h264";
//char *filename_out_a = "cuc_ieschool.aac";
int ff_demuxer(const char *filename_out_v, const char *filename_out_a, const char *filename_in);

#ifdef __cplusplus
}
#endif

#endif // FF_DEMUXER_H
