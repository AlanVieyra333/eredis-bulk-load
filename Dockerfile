FROM shuppet/eredis:latest as EREDIS

#FROM registry.redhat.io/rhel-atomic
#FROM alpine:3.9
FROM debian:stretch
LABEL maintainer="Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>"

COPY --from=EREDIS /usr/local/lib/liberedis.so /usr/local/lib/liberedis.so

RUN apt-get update; \
	apt-get install -y --no-install-recommends \
		build-essential \
		libev-dev

WORKDIR /app

ADD ./redis_load_from_file.o ./redis_load_from_file

ENV FILENAME=R09_80000000.txt
ENV REDIS_HOST=redis
ENV REDIS_PORT=6379
ENV REDIS_PASS=changeme

VOLUME [ "/data" ]

RUN echo "/app/redis_load_from_file /data/\$FILENAME \$REDIS_HOST \$REDIS_PORT \$REDIS_PASS" > ./entrypoint.sh
RUN chmod +x ./entrypoint.sh

CMD [ "/bin/bash", "-c", "/app/entrypoint.sh"]