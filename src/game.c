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
	game.nBodies = 11;
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

	BDY(4).name = "Luna";
	BDY(4).r = 1737400;
	BDY(4).m = 7.342e22;
	BDY(4).px = BDY(3).px + 362600000;
	BDY(4).py = 0;
	BDY(4).vx = 0;
	BDY(4).vy = BDY(3).vy + 1077.286575649856;

	BDY(5).name = "Mars";
	BDY(5).r = 3397000;
	BDY(5).m = 6.417e23;
	BDY(5).px = 206655215000;
	BDY(5).py = 0;
	BDY(5).vx = 0;
	BDY(5).vy = 26498.48200829731;

	BDY(6).name = "Jupiter";
	BDY(6).r = 69911000;
	BDY(6).m = 1.8982e27;
	BDY(6).px = 740679835000;
	BDY(6).py = 0;
	BDY(6).vx = 0;
	BDY(6).vy = 13705.699722207082;

	BDY(7).name = "Io";
	BDY(7).r = 1821600;
	BDY(7).m = 8.931938e22;
	BDY(7).px = BDY(6).px + 420000000;
	BDY(7).py = 0;
	BDY(7).vx = 0;
	BDY(7).vy = BDY(6).vy + 17402.602881771745;

	BDY(8).name = "Europa";
	BDY(8).r = 1560800;
	BDY(8).m = 4.799844e22;
	BDY(8).px = BDY(6).px + 664862000;
	BDY(8).py = 0;
	BDY(8).vx = 0;
	BDY(8).vy = BDY(6).vy + 13865.80294814375;

	BDY(9).name = "Ganymede";
	BDY(9).r = 2634100;
	BDY(9).m = 1.4819e23;
	BDY(9).px = BDY(6).px + 1069200000;
	BDY(9).py = 0;
	BDY(9).vx = 0;
	BDY(9).vy = BDY(6).vy + 10891.283327890113;

	BDY(10).name = "Callisto";
	BDY(10).r = 2410300;
	BDY(10).m = 1.075938e23;
	BDY(10).px = BDY(6).px + 1869000000;
	BDY(10).py = 0;
	BDY(10).vx = 0;
	BDY(10).vy = BDY(6).vy + 8263.596129162883;

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
