# Test Redis
# docker build . -t eredis-bulk-load
# docker-compose up
# docker-compose stop redis-ephemeral
# redis-cli -h localhost -a TeLcEl GET 5500000000

FILENAME=R09.txt
REDIS_HOST=localhost
REDIS_PORT=6379
REDIS_PASS=TeLcEl

.PHONY: build run clear generate docker-build deploy

build: load_data_redis_series.o load_data_redis_planes.o ./test/generate_file.o ./test/redis_example.o ./test/read_file.o
	@echo "Compilado."

run: load_data_redis_series.o
	./load_data_redis_series.o ${FILENAME} ${REDIS_HOST} ${REDIS_PORT} ${REDIS_PASS}

generate: ./test/generate_file.o
	./test/generate_file.o 50000000

load_data_redis_series.o: load_data_redis_series.cpp log.o
	g++ -std=c++17 -w -Wall -pedantic load_data_redis_series.cpp log.o -o load_data_redis_series.o -O2 -lstdc++fs -leredis

load_data_redis_planes.o: load_data_redis_planes.cpp log.o
	g++ -std=c++17 -w -Wall -pedantic load_data_redis_planes.cpp log.o -o load_data_redis_planes.o -O2 -lstdc++fs -leredis

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
	cd openshift-applier/ && ansible-playbook site.yml