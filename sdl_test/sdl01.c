
#include "SDL.h"
#include "SDL_surface.h"
#undef main

int render_sdl();
SDL_Texture* LoadImage(SDL_Renderer *render, const char *filename);

int main(int argc, char* args[])
{
	int ret = 0;

}

int render_sdl()
{
	int ret = 0;
	SDL_Window *win = NULL;
	SDL_Renderer *ren = NULL;

	SDL_Texture *tex = NULL;

	ret = SDL_Init(SDL_INIT_EVERYTHING);
	if(ret<0) return -1;

	//能让我们绘制的窗口前两个参数控制窗口位置，然后是窗口大小 再然后是位标（falg）
	win = SDL_CreateWindow("Hello World!", 100, 100, 565, 67, SDL_WINDOW_RESIZABLE);
	if(win==NULL) return -2;

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == NULL) return -3;

	tex = LoadImage(ren, "../res/hello.bmp");
	if (tex == NULL) return -4;

	// render
	SDL_RenderClear(ren); 
	SDL_RenderCopy(ren, tex , NULL, NULL); 
	SDL_RenderPresent(ren );/*更新屏幕*/

	SDL_Delay(20000);

	SDL_DestroyTexture(tex);/*释放内存*/
	SDL_DestroyRenderer(ren );
	SDL_DestroyWindow(win );

	SDL_Quit();
	return 0;
}

SDL_Texture* LoadImage(SDL_Renderer *render, const char *filename)
{
	SDL_Surface *loadedImage = NULL;
	SDL_Texture *texture = NULL;

	loadedImage = SDL_LoadBMP(filename);
	if (loadedImage != NULL)
	{
		texture = SDL_CreateTextureFromSurface(render, loadedImage);
		SDL_FreeSurface(loadedImage);
	}
	else
		return NULL;

	return texture;
}
/*
#include "SDL_image.h"
SDL_Texture* LoadImage2(SDL_Renderer *render, const char *filename)
{
		SDL_Texture* tex = NULL;
		tex = IMG_LoadTexture(render, filename);
		if (tex == NULL)
		{
			printf("Failed to load image: %s. error:%s", filename, IMG_GetError());
		}
		return tex;
}
*/

void ApplySurface(SDL_Renderer *render, SDL_Texture *tex, int x, int y)
{
	SDL_Rect pos;
	pos.x = x;
	pos.y = y;
	SDL_QueryTexture(tex, NULL, NULL, &pos.w, &pos.h);

	SDL_RenderCopy(render, tex, NULL, &pos);
}
