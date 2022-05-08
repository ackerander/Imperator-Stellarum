#include "game.h"
#include <math.h>

game_t game;
extern SDL_sem *gLock;
double timeW = 1;
static Uint64 tps;
static SDL_Thread *physicsThread = 0;
static double *sum;
static double *k;
static double *ks;

void
newGame()
{
	game.nBodies = 5;
	game.bodies = malloc(game.nBodies * sizeof(body_t));
	sum = malloc(4 * game.nBodies * sizeof(double));
	k = malloc(4 * game.nBodies * sizeof(double));
	ks = malloc(4 * game.nBodies * sizeof(double));
	tps = SDL_GetPerformanceFrequency();
	
	BDY(0).name = "Sol";
	BDY(0).r = 695700000;
	BDY(0).m = 1.9885e30;
	BDY(0).px = 0;
	BDY(0).py = 0;
	BDY(0).vx = 0;
	BDY(0).vy = 0;

	BDY(1).name = "Mercury";
	BDY(1).r = 2439000;
	BDY(1).m = 3.301e23;
	BDY(1).px = 46001009000;
	BDY(1).py = 0;
	BDY(1).vx = 0;
	BDY(1).vy = 58976.66765397275;

	BDY(2).name = "Venus";
	BDY(2).r = 6050000;
	BDY(2).m = 4.867e24;
	BDY(2).px = 107476170000;
	BDY(2).py = 0;
	BDY(2).vx = 0;
	BDY(2).vy = 35258.70099654741;

	BDY(3).name = "Terra";
	BDY(3).r = 6378000;
	BDY(3).m = 5.972e24;
	BDY(3).px = 147098291000;
	BDY(3).py = 0;
	BDY(3).vx = 0;
	BDY(3).vy = 30286.620365400406;

	BDY(4).name = "Mars";
	BDY(4).r = 3397000;
	BDY(4).m = 6.417e23;
	BDY(4).px = 206655215000;
	BDY(4).py = 0;
	BDY(4).vx = 0;
	BDY(4).vy = 26498.48200829731;

	game.status = RUNNING;
}

static void
accel(const double f1, const double f2)
{
	double d, dx, dy, a;

	for (size_t i = 0; i < game.nBodies; ++i) {
		ks[4*i] += f2 * (k[4*i] = f1 * sum[4*i + 2]);
		ks[4*i + 1] += f2 * (k[4*i + 1] = f1 * sum[4*i + 3]);
		k[4*i + 2] = k[4*i + 3] = 0;
		for (size_t j = 0; j < i; ++j) {
			dx = sum[4*j] - sum[4*i];
			dy = sum[4*j + 1] - sum[4*i + 1];
			d = dx * dx + dy * dy;
			a = BDY(j).m / (d * sqrt(d));
			k[4*i + 2] += a * dx;
			k[4*i + 3] += a * dy;
		}
		for (size_t j = i + 1; j < game.nBodies; ++j) {
			dx = sum[4*j] - sum[4*i];
			dy = sum[4*j + 1] - sum[4*i + 1];
			d = dx * dx + dy * dy;
			a = BDY(j).m / (d * sqrt(d));
			k[4*i + 2] += a * dx;
			k[4*i + 3] += a * dy;
		}
		k[4*i + 2] *= f1 * G;
		k[4*i + 3] *= f1 * G;
		ks[4*i + 2] += f2 * k[4*i + 2];
		ks[4*i + 3] += f2 * k[4*i + 3];
	}
}

static void
sumk()
{
	for (size_t i = 0; i < game.nBodies; ++i) {
		sum[4 * i] = BDY(i).px + k[4 * i];
		sum[4 * i + 1] = BDY(i).py + k[4 * i + 1];
		sum[4 * i + 2] = BDY(i).vx + k[4 * i + 2];
		sum[4 * i + 3] = BDY(i).vy + k[4 * i + 3];
	}
}

static void
updaterk(const double dt)
{
	for (size_t i = 0; i < game.nBodies; ++i) {
		sum[4 * i] = BDY(i).px;
		sum[4 * i + 1] = BDY(i).py;
		sum[4 * i + 2] = BDY(i).vx;
		sum[4 * i + 3] = BDY(i).vy;
	}
	memset(ks, 0, 4 * game.nBodies * sizeof(double));
	accel(0.5 * dt, 1.0 / 3);
	sumk();
	accel(0.5 * dt, 2.0 / 3);
	sumk();
	accel(dt, 1.0 / 3);
	sumk();
	accel(dt, 1.0 / 6);
	SDL_SemWait(gLock);
	for (size_t i = 0; i < game.nBodies; ++i) {
		BDY(i).px += ks[4*i];
		BDY(i).py += ks[4*i + 1];
		BDY(i).vx += ks[4*i + 2];
		BDY(i).vy += ks[4*i + 3];
	}
	SDL_SemPost(gLock);
}

int
physicsLoop(void *p)
{
	Uint64 time = SDL_GetPerformanceCounter(), newT;

	while (game.status) {
		newT = SDL_GetPerformanceCounter();
//		updaterk(50000 * (double)(newT - time) / tps);
		updaterk(timeW * (newT - time) / tps);
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
	free(sum);
	free(k);
	free(ks);
}
