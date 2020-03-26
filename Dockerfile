FROM shuppet/eredis:latest as EREDIS

#FROM registry.redhat.io/rhel-atomic
#FROM alpine:3.9
FROM debian:stretch
LABEL maintainer="Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>"

COPY --from=EREDIS /usr/local/lib/liberedis.so /usr/local/lib/liberedis.so

RUN set -ex; \
	\
	savedAptMark="$(apt-mark showmanual)"; \
	\
	apt-get update; \
	apt-get install -y --no-install-recommends \
		libev-dev \
	; \
	\
	apt-mark auto '.*' > /dev/null; \
	apt-mark manual $savedAptMark; \
	apt-get purge -y; \
	rm -rf /var/lib/apt/lists/*

WORKDIR /app

ADD ./redis_load_from_file .

ENV FILENAME=R09_80000000.txt
ENV REDIS_HOST=redis
ENV REDIS_PORT=6379
ENV REDIS_PASS=changeme

VOLUME [ "/data" ]

RUN echo "/app/redis_load_from_file /data/\$FILENAME \$REDIS_HOST \$REDIS_PORT \$REDIS_PASS" > ./entrypoint.sh
RUN chmod +x ./entrypoint.sh

CMD [ "/bin/bash", "-c", "/app/entrypoint.sh"]