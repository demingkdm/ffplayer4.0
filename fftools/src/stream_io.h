// STREAM_IO.h : 
//

#ifndef _STREAM_IO_H
#define _STREAM_IO_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void* stream_open(char *url_name);
void stream_close(void *opaque);

int stream_read(void* opaque, uint8_t* buf, int buf_size);
int64_t stream_seek(void *opaque, int64_t offset, int whence);
int stream_write(void *opaque, uint8_t *buf, int buf_size);

#ifdef __cplusplus
}
#endif

#endif //#ifndef _STREAM_IO_H
