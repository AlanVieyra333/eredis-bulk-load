APP=main
ARGS=-std=c++17 -O2 -pthread -lev -lhiredis -fopenmp

build: ${APP}.o

run: build
	./${APP}.o

${APP}.o: ${APP}.cpp config.h lib/log.o
	g++ ${ARGS} -o ${APP}.o ${APP}.cpp lib/log.o -lstdc++fs
	@echo "Compilado correctamente"

lib/log.o: lib/log.c
	gcc lib/log.c -o lib/log.o -c

clean:
	rm *.o

.PHONY: run build clean