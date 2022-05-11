#include "game.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>

#define NAME "Imperator Stellarum"
#define RELFONT "assets/ttf/NEUROPOL.ttf"
#define ABSPATH "/home/alex/Code/IS/"
#define BG 0, 0, 0, 0xFF
#define UI 0x40, 0xA0, 0x40, 0xFF
#define TEXTC 0xFF, 0xFF, 0xFF, 0xFF
#define FG 0xFFFFFFFF
#define SLIDERS 2
#define PT 48
#define TRIG45 0.707106781188
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define CLAMP(X) MAX(MIN((X), 1), 0)
#define SCREENX(X) (int)(scale * ((X) - centerX) + screenW / 2)
#define SCREENY(Y) (int)(scale * (centerY - (Y)) + screenH / 2)

extern game_t game;
extern double timeW;
SDL_sem* gLock;

enum codes { SUCCESS = 0, INIT_SDL = 1, LOAD_TTF = 2, INIT_TTF = 3, RENDERER = 4, WINDOW = 5 };

typedef struct {
	SDL_Texture *texture;
	int w, h;
} SDL_Tex;

static SDL_Renderer *renderer;
static SDL_Window *window;
static TTF_Font *font;
static SDL_Tex *textTex;
static double scale, centerX, centerY;
static int screenW, screenH;
static unsigned int inflat = 1;
static SDL_Thread *ioThread = 0;
static SDL_Rect sliders[2 * SLIDERS];
static double slVals[SLIDERS] = {0};
static int slideH;
static int8_t *selBitMask;
static size_t camLock;

static void
renderBody(size_t idx)
{
	SDL_Point verts[3];
	SDL_Rect textRec;
	int x = SCREENX(BDY(idx).px), y = SCREENY(BDY(idx).py),
	    screenR = inflat * scale * BDY(idx).r;

	if (x < -screenR || x - screenR > screenW || y < screenR || y - screenR > screenH)
		return;
	filledCircleColor(renderer, SCREENX(BDY(idx).px), SCREENY(BDY(idx).py),
		screenR, FG);
	if ((selBitMask[idx / 8] >> (idx % 8)) & 1) {
		verts[0].x = SCREENX(BDY(idx).px) + (screenR + screenW / 512) * TRIG45;
		verts[0].y = SCREENY(BDY(idx).py) - (screenR + screenW / 512) * TRIG45;
		verts[1].x = verts[0].x + screenW / 128;
		verts[1].y = verts[0].y - screenW / 128;
		textRec.x = verts[1].x;
		textRec.y = verts[1].y;
		textRec.h = screenW / 128;
		textRec.w = textTex[idx].w * textRec.h / textTex[idx].h;
		verts[2].x = verts[1].x + textRec.w;
		verts[2].y = verts[1].y;
		SDL_RenderDrawLines(renderer, verts, 3);
		SDL_RenderCopy(renderer, textTex[idx].texture, 0, &textRec);
	}
}

static void
renderScene()
{
	SDL_Rect drawRect = { .x = 173 * screenW / 192, .y = screenH / 64 };
	
	SDL_SetRenderDrawColor(renderer, BG);
	SDL_RenderClear(renderer);
	SDL_SemWait(gLock);
	if (camLock != (size_t)(-1)) {
		centerX = BDY(camLock).px;
		centerY = BDY(camLock).py;
	}
	for (size_t i = 0; i < game.nBodies; ++i)
		renderBody(i);
	SDL_SemPost(gLock);
	SDL_SetRenderDrawColor(renderer, UI);
	SDL_RenderFillRects(renderer, sliders, SLIDERS);
	SDL_RenderDrawRects(renderer, &sliders[SLIDERS], SLIDERS);

	for (size_t i = 0; i < game.nBodies; ++i) {
		drawRect.w = screenW / 12;
		drawRect.h = screenH / 32;
		if ((selBitMask[i / 8] >> (i % 8)) & 1)
			SDL_RenderFillRect(renderer, &drawRect);
		else
			SDL_RenderDrawRect(renderer, &drawRect);
		drawRect.w = textTex[i].w;
		drawRect.h = textTex[i].h;
		drawRect.x += screenW / 24 - drawRect.w / 2;
		drawRect.y += screenH / 64 - drawRect.h / 2;
		SDL_RenderCopy(renderer, textTex[i].texture, 0, &drawRect);
		drawRect.x -= screenW / 24 - drawRect.w / 2;
		drawRect.y += 3 * screenH / 128 + drawRect.h / 2;
	}
	SDL_RenderPresent(renderer);
}

char
init(double zoom, size_t lock)
{
	SDL_Surface *textSurface = 0;

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
	/* Init ttf */
	if (TTF_Init() == -1)
		return INIT_TTF;
	if (!((font = TTF_OpenFont(RELFONT, PT)) || (font = TTF_OpenFont(ABSPATH RELFONT, PT))))
		return LOAD_TTF;
	/* Camera */
	camLock = lock;
	SDL_GetWindowSize(window, &screenW, &screenH);
	scale = 0.5 * MIN(screenW, screenH) / zoom;
	centerX = centerY = 0;
	/* Create Semaphore */
	gLock = SDL_CreateSemaphore(1);
	/* UI */
	selBitMask = calloc(game.nBodies / 8, 1);
	textTex = malloc(game.nBodies * sizeof(SDL_Tex));
	for (size_t i = 0; i < game.nBodies; ++i) {
		textSurface = TTF_RenderText_Solid(font, BDY(i).name, (SDL_Color){TEXTC});
		textTex[i].texture = SDL_CreateTextureFromSurface(renderer, textSurface);
		textTex[i].w = textSurface->w;
		textTex[i].h = textSurface->h;
	}
	SDL_FreeSurface(textSurface);

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
	const Uint8* keyStates;
	int x, y;

loop:
	t1 = SDL_GetTicks();
	renderScene();
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			game.status = QUIT;
			return 0;
		case SDL_MOUSEBUTTONDOWN:
			SDL_GetMouseState(&x, &y);
			y = 128 * (y - screenH / 64) / (5 * screenH);
			if (x >= 59 * screenW / 64 && x <= screenW - screenW / 64 &&
				y >= 0 && y < game.nBodies) {
				switch (e.button.button) {
				case SDL_BUTTON_LEFT:
					selBitMask[y / 8] ^= 1 << (y % 8);
					break;
				case SDL_BUTTON_MIDDLE:
					camLock = y;
					break;
				}
			} else if (e.button.button == SDL_BUTTON_MIDDLE)
				camLock = (size_t)(-1);
			break;
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
				scale += 5e-2 * scale * e.wheel.y;
			}
			break;
		}
	}
	/* Key presses */
	keyStates = SDL_GetKeyboardState(0);
	if (keyStates[SDL_SCANCODE_W])
		centerY += 10 / scale;
	if (keyStates[SDL_SCANCODE_S])
		centerY -= 10 / scale;
	if (keyStates[SDL_SCANCODE_A])
		centerX -= 10 / scale;
	if (keyStates[SDL_SCANCODE_D])
		centerX += 10 / scale;
	t2 = SDL_GetTicks();
	t1 = t2 - t1;
	if (t1 < 20)
		SDL_Delay(20 - t1);
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
		for (size_t i = 0; i < game.nBodies; ++i)
			SDL_DestroyTexture(textTex[i].texture);
		free(textTex);
		TTF_CloseFont(font);
		/* FALLTHRU */
	case LOAD_TTF:
		TTF_Quit();
		/* FALLTHRU */
	case INIT_TTF:
		SDL_DestroyRenderer(renderer);
		/* FALLTHRU */
	case RENDERER:
		SDL_DestroyWindow(window);
		/* FALLTHRU */
	case WINDOW:
		SDL_Quit();
		/* FALLTHRU */
	default:
		free(selBitMask);
		SDL_DestroySemaphore(gLock);
		quitGame();
		exit(code);
	}
}
