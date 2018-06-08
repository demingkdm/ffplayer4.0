#include <stdio.h>
#include "ff_player.h"





int main(void)
{
	int ret = 0;

	//ret = ff_play_media("c:\\video\\bigbuckbunny_480x272.h265");
	ret = ff_play_media_av0("c:\\video\\cuc_ieschool.mp4");

	//ret = ff_play_audio("c:\\video\\gowest.aac"); //ok
	//ret = ff_play_audio("c:\\video\\cuc_ieschool.mp3"); //ok
	//ret = ff_play_audio("c:\\video\\cuc_ieschool.ape"); //failed with noise
	

    printf("ff ret = %u!\n", ret);
    return ret;
}

