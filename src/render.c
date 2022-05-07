#include "game.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#define NAME "Imperator Stellarum"
#define BG 0, 0, 0, 0xFF
#define SL 0x40, 0xA0, 0x40, 0xFF
#define FG 0xFFFFFFFF
#define SLIDERS 2
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define CLAMP(X) MAX(MIN((X), 1), 0)
#define SCREENX(X) ((int)(scale * (X) + centerX))
#define SCREENY(Y) ((int)(-scale * (Y) + centerY))

extern game_t game;
extern double timeW;
SDL_sem* gLock;

enum codes { SUCCESS = 0, INIT_SDL, RENDERER, WINDOW };

static SDL_Renderer *renderer;
static SDL_Window *window;
static double scale;
static double centerX;
static double centerY;
static int screenW;
static int screenH;
static unsigned int inflat = 1;
static SDL_Thread *ioThread = 0;
static SDL_Rect sliders[2 * SLIDERS];
static double slVals[SLIDERS] = {0};
static int slideH;

static void
renderBody(size_t idx)
{
	filledCircleColor(renderer, SCREENX(BDY(idx).px), SCREENY(BDY(idx).py),
		inflat * scale * BDY(idx).r, FG);
}

static void
renderScene()
{
	SDL_SetRenderDrawColor(renderer, BG);
	SDL_RenderClear(renderer);
	SDL_SemWait(gLock);
	for (size_t i = 0; i < game.nBodies; ++i)
		renderBody(i);
	SDL_SemPost(gLock);
	SDL_SetRenderDrawColor(renderer, SL);
	SDL_RenderFillRects(renderer, sliders, SLIDERS);
	SDL_RenderDrawRects(renderer, &sliders[SLIDERS], SLIDERS);
	SDL_RenderPresent(renderer);
}

char
init(double zoom)
{
	/* Init SDL */
	if (SDL_Init(SDL_INIT_VIDEO))
		return INIT_SDL;
	/* Create window */
	if (!(window = SDL_CreateWindow(NAME,
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0,
			SDL_WINDOW_FULLSCREEN_DESKTOP)))
		return WINDOW;
	/* Create renderer */
	if (!(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)))
		return RENDERER;
	/* Scaling */
	SDL_GetWindowSize(window, &screenW, &screenH);
	scale = 0.5 * MIN(screenW, screenH) / zoom;
	centerX = screenW / 2;
	centerY = screenH / 2;
	/* Create Semaphore */
	gLock = SDL_CreateSemaphore(1);
	/* UI */
	sliders[0].w = sliders[1].w = sliders[2].w = sliders[3].w = screenW / 64;
	sliders[0].h = sliders[1].h = 0;
	sliders[2].h = sliders[3].h = slideH = screenH / 8;
	sliders[0].x = sliders[2].x = 15 * screenW / 16;
	sliders[1].x = sliders[3].x = 31 * screenW / 32;
	sliders[2].y = sliders[3].y =
		(sliders[0].y = sliders[1].y = 63 * screenH / 64) - slideH;
	return SUCCESS;
}

int
ioLoop(void *p)
{
	SDL_Event e;
	Uint32 t1, t2;
	int x, y;

loop:
	t1 = SDL_GetTicks();
	renderScene();
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			game.status = QUIT;
			return 0;
		case SDL_MOUSEWHEEL:
			SDL_GetMouseState(&x, &y);
			if (x >= sliders[0].x &&
				x <= sliders[0].x + sliders[0].w &&
				y >= sliders[2].y &&
				y <= sliders[2].y + slideH) {
				timeW = 1000 * pow(101, (slVals[0] =
					CLAMP(3e-2 * e.wheel.y + slVals[0]))) - 999;
				sliders[0].y = 63 * screenH / 64 -
					(sliders[0].h = slideH * slVals[0]);
			} else if (x >= sliders[1].x &&
				x <= sliders[1].x + sliders[1].w &&
				y >= sliders[3].y &&
				y <= sliders[3].y + slideH) {
				inflat = 99 * (slVals[1] =
					CLAMP(3e-2 * e.wheel.y + slVals[1])) + 1;
				sliders[1].y = 63 * screenH / 64 -
					(sliders[1].h = slideH * slVals[1]);
			} else {
				scale += 5e-11 * e.wheel.y;
			}
			break;
		case SDL_KEYDOWN:
			switch(e.key.keysym.sym) {
			case SDLK_w:
				centerY += 10;
				break;
			case SDLK_s:
				centerY -= 10;
				break;
			case SDLK_a:
				centerX += 10;
				break;
			case SDLK_d:
				centerX -= 10;
				break;
			}
		}
	}
	t2 = SDL_GetTicks();
	t1 = t2 - t1;
	if (t1 < 25)
		SDL_Delay(25 - t1);
	t1 = t2;
	goto loop;
}

void
threadIO()
{
	ioThread = SDL_CreateThread(ioLoop, "Render Thread", 0);
}

void
quit(char code)
{
	if (ioThread) {
		SDL_WaitThread(ioThread, 0);
		ioThread = 0;
	}
	switch (code) {
	case SUCCESS:
		SDL_DestroyRenderer(renderer);
		/* FALLTHRU */
	case RENDERER:
		SDL_DestroyWindow(window);
		/* FALLTHRU */
	case WINDOW:
		SDL_Quit();
		/* FALLTHRU */
	default:
		SDL_DestroySemaphore(gLock);
		quitGame();
		exit(code);
	}
}
