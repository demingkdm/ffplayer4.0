#ifndef FF_MUXER_H
#define FF_MUXER_H

#ifdef __cplusplus
extern "C" {
#endif

//ff_play_media("bigbuckbunny_480x272.h265");
// play video
int ff_play_media(const char *filein); //simple
int ff_play_media2(const char *filein); //use sdl event

int ff_play_media_av0(const char *filein); //add video

// play audio
int ff_play_audio(const char *filein);

#ifdef __cplusplus
}
#endif

#endif // FF_MUXER_H
