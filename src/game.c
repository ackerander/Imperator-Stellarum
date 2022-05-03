#include "game.h"
#include <math.h>

#define DISTSQ(A, B) ((BDY(A).px - BDY(B).px) * (BDY(A).px - BDY(B).px) +\
(BDY(A).py - BDY(B).py) * (BDY(A).py - BDY(B).py))
//#define CENTER

game_t game;
extern SDL_sem* gLock;
static Uint64 tps;
static SDL_Thread *physicsThread = 0;

void
newGame()
{
	tps = SDL_GetPerformanceFrequency();
	game.g = 5e-7;
#ifdef CENTER
	game.bodies = malloc(3 * sizeof(body_t));
	game.nBodies = 3;
	
	BDY(0).r = 3;
	BDY(0).m = 100000;
	BDY(0).px = 30;
	BDY(0).py = 0;
	BDY(0).vx = 0;
	BDY(0).vy = 15;

	BDY(1).r = 3;
	BDY(1).m = 100000;
	BDY(1).px = 0;
	BDY(1).py = 0;
	BDY(1).vx = 0;
	BDY(1).vy = 0;

	BDY(2).r = 3;
	BDY(2).m = 100000;
	BDY(2).px = -30;
	BDY(2).py = 0;
	BDY(2).vx = 0;
	BDY(2).vy = -15;
#else
	game.bodies = malloc(3 * sizeof(body_t));
	game.nBodies = 3;
	
	BDY(0).r = 3;
	BDY(0).m = 100000;
	BDY(0).px = -26;
	BDY(0).py = -15;
	BDY(0).vx = 5;
	BDY(0).vy = -5 * sqrt(3);

	BDY(1).r = 3;
	BDY(1).m = 100000;
	BDY(1).px = 26;
	BDY(1).py = -15;
	BDY(1).vx = 5;
	BDY(1).vy = 5 * sqrt(3);

	BDY(2).r = 3;
	BDY(2).m = 100000;
	BDY(2).px = 0;
	BDY(2).py = 30;
	BDY(2).vx = -10;
	BDY(2).vy = 0;
#endif
	game.status = RUNNING;
}

static void
update(const double dt)
{
	double ax, ay, d, a;

	for (size_t i = 0; i < game.nBodies; ++i) {
		ax = ay = 0;
		for (size_t j = 0; j < i; ++j) {
			d = DISTSQ(i, j);
			a = BDY(j).m / (d * sqrt(d));
			ax += a * (BDY(j).px - BDY(i).px);
			ay += a * (BDY(j).py - BDY(i).py);
		}
		for (size_t j = i + 1; j < game.nBodies; ++j) {
			d = DISTSQ(i, j);
			a = BDY(j).m / (d * sqrt(d));
			ax += a * (BDY(j).px - BDY(i).px);
			ay += a * (BDY(j).py - BDY(i).py);
		}
		a = game.g * BDY(i).m * dt;
		BDY(i).vx += a * ax;
		BDY(i).vy += a * ay;
	/* Add collision logic */
	}
	SDL_SemWait(gLock);
	for (size_t i = 0; i < game.nBodies; ++i) {
		BDY(i).px += BDY(i).vx * dt;
		BDY(i).py += BDY(i).vy * dt;
	}
	SDL_SemPost(gLock);
}

int
physicsLoop(void *p)
{
	Uint64 time = SDL_GetPerformanceCounter(), newT;

	while (game.status) {
		newT = SDL_GetPerformanceCounter();
		update((double)(newT - time) / tps);
		time = newT;
	}
	return 0;
}

void
threadPhysics()
{
	physicsThread = SDL_CreateThread(physicsLoop, "Physics Thread", 0);
}

void
quitGame()
{
	if (physicsThread) {
		SDL_WaitThread(physicsThread, 0);
		physicsThread = 0;
	}
	free(game.bodies);
}
