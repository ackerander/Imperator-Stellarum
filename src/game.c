#include "game.h"
#include <math.h>

#define DISTSQ(A, B) ((BDY(A).px - BDY(B).px) * (BDY(A).px - BDY(B).px) +\
(BDY(A).py - BDY(B).py) * (BDY(A).py - BDY(B).py))

game_t game;
extern SDL_sem* gLock;
static Uint64 tps;
static SDL_Thread *physicsThread = 0;

void
newGame()
{
	tps = SDL_GetPerformanceFrequency();

	game.bodies = malloc(5 * sizeof(body_t));
	game.nBodies = 5;
	
	/* Sun */
	BDY(0).r = 695700000;
	BDY(0).m = 1.9885e30;
	BDY(0).px = 0;
	BDY(0).py = 0;
	BDY(0).vx = 0;
	BDY(0).vy = 0;

	/* Mercury */
	BDY(1).r = 2439000;
	BDY(1).m = 3.301e23;
	BDY(1).px = 46001009000;
	BDY(1).py = 0;
	BDY(1).vx = 0;
	BDY(1).vy = 58976.66765397275;
//	BDY(1).vy = 0;

	/* Venus */
	BDY(2).r = 6050000;
	BDY(2).m = 4.867e24;
	BDY(2).px = 107476170000;
	BDY(2).py = 0;
	BDY(2).vx = 0;
	BDY(2).vy = 35258.70099654741;
//	BDY(2).vy = 0;

	/* Earth */
	BDY(3).r = 6378000;
	BDY(3).m = 5.972e24;
	BDY(3).px = 147098291000;
	BDY(3).py = 0;
	BDY(3).vx = 0;
	BDY(3).vy = 30286.620365400406;
//	BDY(3).vy = 0;

	/* Mars */
	BDY(4).r = 3397000;
	BDY(4).m = 6.417e23;
	BDY(4).px = 206655215000;
	BDY(4).py = 0;
	BDY(4).vx = 0;
	BDY(4).vy = 26498.48200829731;
//	BDY(4).vy = 0;

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
		a = G * dt;
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
		update(50000 * (double)(newT - time) / tps);
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
