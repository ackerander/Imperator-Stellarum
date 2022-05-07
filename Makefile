LIBS = -lm -lSDL2 -lSDL2_gfx
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O3 -march=native -mtune=native
DEST = game
CC = cc
OBJ = obj/main.o obj/render.o obj/game.o

all: ${OBJ}
	${CC} ${LIBS} -o ${DEST} ${OBJ}

obj/main.o: src/main.c src/game.h
	${CC} ${CFLAGS} -c -o $@ $<

obj/render.o: src/render.c src/game.h
	${CC} ${CFLAGS} -c -o $@ $<

obj/game.o: src/game.c src/game.h
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm ${OBJ}
