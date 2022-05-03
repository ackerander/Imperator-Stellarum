#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#define BDY(I) game.bodies[(I)]
#define G 6.674e-11

enum state { QUIT = 0, RUNNING = 1 };

typedef struct {
	double r;
	double m;
	double px;
	double py;
	double vx;
	double vy;
} body_t;

typedef struct {
	body_t *bodies;
	size_t nBodies;
	char status;
} game_t;

void newGame();
int physicsLoop(void *p);
void threadPhysics();
void quitGame();

char init(double zoom);
int ioLoop(void *);
void threadIO();
void quit(char code);

#endif
