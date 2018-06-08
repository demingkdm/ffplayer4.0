#include <stdio.h>
#include "ff_transmux.h"
#include "ff_muxer.h"
#include "ff_remuxer.h"
#include "ff_transcache.h"




int main(void)
{
	int ret = 0;

	//ret = encode_yuv2h264("c:\\src01.h264", "c:\\video\\src01_480x272.yuv", 480, 272, 25, 400000);
	//ret = encode_yuv2h264("d:\\480x640.h264", "d:\\480x640.yuv", 480, 640, 6, 300000);

	//ret = test_ff_cache("e:\\Movie2.mp4");
	//ret = ff_transcache("c:\\cuc_ieschool.ts", "c:\\video\\cuc_ieschool.mp4");

	// remux file without using codec
	//ret = ff_remux("c:\\school.ts", "c:\\video\\cuc_ieschool.mp4");
	//ret = ff_remux("c:\\spring1.mkv", "c:\\video\\spring.mp4");
	ret = ff_remux("d:\\media\\ligang.mkv", "d:\\media\\ligang.mp4");

	//ret = ff_muxer("c:\\out21.mp4", "c:\\video\\spring.mp4", "c:\\video\\world.mp3");

    printf("ff ret = %u!\n", ret);
    return ret;
}

