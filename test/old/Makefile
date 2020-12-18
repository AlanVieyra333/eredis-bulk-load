# Test Redis
# docker-compose up --build
# docker-compose stop redis-ephemeral
# redis-cli -h localhost -a TeLcEl GET 5500000000

APP=redis_data_load_series

FILENAME=seriesSiantel081019.txt
REDIS_HOST=localhost
REDIS_PORT=6379
REDIS_PASS=TeLcEl
REDIS_DATABASE=0

.PHONY: build run clear generate docker-build deploy

build: ${APP}.o ./test/generate_file.o ./test/redis_example.o ./test/read_file.o
	@echo "Compilado."

run: ${APP}.o
	./${APP}.o /data/${FILENAME} ${REDIS_HOST} ${REDIS_PORT} ${REDIS_PASS} ${REDIS_DATABASE}

generate: ./test/generate_file.o
	./test/generate_file.o 50000000

${APP}.o: ${APP}.cpp log.o
	g++ -std=c++17 -O2 -w -Wall -pedantic ${APP}.cpp log.o -o ${APP}.o -lstdc++fs -leredis -fopenmp

log.o: log.c
	gcc log.c -o log.o -c

./test/generate_file.o: ./test/generate_file.c
	gcc -o ./test/generate_file.o ./test/generate_file.c -O2

./test/read_file.o: ./test/read_file.c
	gcc -o ./test/read_file.o ./test/read_file.c -O2

./test/redis_example.o: ./test/redis_example.c
	gcc -o ./test/redis_example.o ./test/redis_example.c -O2 -leredis
#-I /usr/local/include/hiredis -lhiredis

clear:
	rm -R -f *.o ./test/*.o ./data/log

docker-build:
	docker-compose build
	
openshift-applier/roles:
	cd openshift-applier/ && ansible-galaxy install -r requirements.yml --roles-path=roles

deploy: openshift-applier/roles
	cd openshift-applier/ && ansible-playbook -i inventory site.yml