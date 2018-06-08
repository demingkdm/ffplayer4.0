#ifndef FF_MUXER_H
#define FF_MUXER_H

#ifdef __cplusplus
extern "C" {
#endif


int ff_muxer(const char *filename_out, const char *filename_in_v, const char *filename_in_a);

#ifdef __cplusplus
}
#endif

#endif // FF_MUXER_H
