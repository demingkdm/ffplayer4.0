#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "SDL.h"
#include "SDL_thread.h"

typedef struct PacketQueue 
{
	AVPacketList *first_pkt;
	AVPacketList *last_pkt;
	int nb_packets;
	int size;
	SDL_mutex *mutex;
	SDL_cond *cond;
} PacketQueue;


void packet_queue_init(PacketQueue *q);
void packet_queue_clean(PacketQueue *q);

int packet_queue_put(PacketQueue *q, AVPacket *pkt);
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block);


#ifdef __cplusplus
}
#endif

#endif // PACKET_QUEUE_H
