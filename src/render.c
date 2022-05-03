#include "game.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#define NAME "Astro"
#define BG 0, 0, 0, 0xFF
#define FG 0xFFFFFFFF
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define SCREENX(X) ((int)(scale * (X)) + screenW / 2)
#define SCREENY(Y) ((int)(-scale * (Y)) + screenH / 2)

extern game_t game;
SDL_sem* gLock;

enum codes { SUCCESS = 0, INIT_SDL, RENDERER, WINDOW };

static SDL_Renderer *renderer;
static SDL_Window *window;
static double scale;
static int screenW;
static int screenH;
static SDL_Thread *ioThread = 0;

static void
renderBody(size_t idx)
{
	filledCircleColor(renderer, SCREENX(BDY(idx).px), SCREENY(BDY(idx).py),
		60 * scale * BDY(idx).r, FG);
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
	/* Create Semaphore */
	gLock = SDL_CreateSemaphore(1);
	return SUCCESS;
}

int
ioLoop(void *p)
{
	SDL_Event e;
	Uint32 t1, t2;

loop:
	t1 = SDL_GetTicks();
	renderScene();
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			game.status = QUIT;
			return 0;
		case SDL_MOUSEWHEEL:
			scale += 5e-11 * e.wheel.y;
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
