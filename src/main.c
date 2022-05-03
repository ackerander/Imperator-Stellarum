#include "game.h"

int
main()
{
	char code = init(100);
	newGame();
	if (!code) {
#ifdef RENDER_THREAD
		threadIO();
		physicsLoop(0);
#else
		threadPhysics();
		ioLoop(0);
#endif
	}
	quit(code);
}