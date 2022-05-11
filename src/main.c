#include "game.h"

int
main()
{
	uint8_t code;
	newGame();
	code = init(1.2e11, -1);
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
