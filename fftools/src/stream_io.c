// stream_io.c : 
//

#include "stream_io.h"
#include <stdio.h>
#include "libavformat/avio.h"

void* stream_open(char *url_name)
{
	FILE *fp = NULL;

	if(!url_name)    return NULL;

	fp = fopen(url_name, "rb");
	if(0 == fp) return NULL;

	return (void*)fp;
}
void stream_close(void *opaque)
{
	FILE *fp = opaque;
	if(!opaque)    return;

	fclose(fp);
	return;
}

int stream_read(void* opaque, uint8_t* buf, int buf_size)
{
	FILE* fp = (FILE*)opaque;
	size_t size = 0;
	if(!fp) return -1;

	if(!feof(fp)){  
		size = fread(buf, 1, buf_size, fp);
		return size;
	}else{  
		return -1;  
	} 
}

int64_t stream_seek(void *opaque, int64_t offset, int whence)
{
	FILE *fp = (FILE*)opaque;
	if(!fp) return -1;

	if (whence == AVSEEK_SIZE) {
		return -2;
	}

	fseek(fp, offset, whence);
	return (int64_t)ftell(fp);
}

int stream_write(void *opaque, uint8_t *buf, int buf_size)
{
	FILE *fp = (FILE*)opaque;
	size_t size = 0;
	if(!fp) return -1;
	
	if(!feof(fp)){
		size=fwrite(buf,1,buf_size,fp);
		return size;
	}else{
		return -1;
	}
}