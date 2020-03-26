# Test Redis
# docker run -it -d --name redis-ephemeral --rm -p 6379:6379 -e REDIS_PASSWORD=TeLcEl registry.redhat.io/rhscl/redis-5-rhel7
# docker stop redis-ephemeral
# redis-cli -h localhost -a TeLcEl GET 5516409291

FILENAME=R09_80000000.txt
REDIS_HOST=localhost
REDIS_PORT=6379
REDIS_PASS=TeLcEl

.PHONY: build run clear generate

build: redis_load_from_file redis_data_load.o redis_example.o generate_file.o
	@echo "Compilado."

run: redis_load_from_file
	./redis_load_from_file ${FILENAME} ${REDIS_HOST} ${REDIS_PORT} ${REDIS_PASS}

generate: generate_file.o
	./generate_file.o 80000000

redis_load_from_file: redis_load_from_file.cpp
	g++ -std=c++17 -Wall -pedantic redis_load_from_file.cpp -o redis_load_from_file -O2 -lstdc++fs -leredis
#gcc -o redis_load_from_file.o redis_load_from_file.c -O2 -leredis

generate_file.o: generate_file.c
	gcc -o generate_file.o generate_file.c -O2

redis_data_load.o: redis_data_load.c
	gcc -o redis_data_load.o redis_data_load.c -O2

redis_example.o: redis_example.c
	gcc -o redis_example.o redis_example.c -O2 -leredis

#-I /usr/local/include/hiredis -lhiredis

clear:
	rm *.o