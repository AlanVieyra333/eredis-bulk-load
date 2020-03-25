APP=redis_example
HOST=localhost
PORT=6379
INSTANCE=0
PASS=TeLcEl

eredis: app.c
	gcc -o app.o -g app.c -leredis -O2

run: build
	./${APP}.o ${HOST} ${PORT} ${INSTANCE} ${PASS}

build: ${APP}.c
	gcc -o ${APP}.o -g ${APP}.c -I /usr/local/include/hiredis -lhiredis

clear:
	rm *.o