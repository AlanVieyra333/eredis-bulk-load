FROM shuppet/eredis:latest as EREDIS

FROM registry.redhat.io/rhel-atomic

COPY --from=EREDIS /usr/local/lib/liberedis.so /usr/lib64/liberedis.so

WORKDIR /app

ADD libev-4.15-3.el7.x86_64.rpm /tmp
RUN rpm -Uvh /tmp/libev-4.15-3.el7.x86_64.rpm
RUN rm /tmp/libev-4.15-3.el7.x86_64.rpm

ADD ./redis_load_from_file.o .

ENV FILENAME=R09_80000000.txt
ENV REDIS_HOST=redis
ENV REDIS_PORT=6379
ENV REDIS_PASS=TeLcEl

VOLUME [ "/data" ]

RUN echo "/app/redis_load_from_file.o /data/\$FILENAME \$REDIS_HOST \$REDIS_PORT \$REDIS_PASS" > ./entrypoint.sh
RUN chmod +x ./entrypoint.sh

ENTRYPOINT ["/bin/bash", "-c", "/app/entrypoint.sh"]