FROM shuppet/eredis:latest as EREDIS

FROM centos:8
LABEL maintainer="Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>"

COPY --from=EREDIS /usr/local/lib/liberedis.so /lib64/
COPY --from=EREDIS /usr/local/include/eredis.h /usr/local/include/
COPY --from=EREDIS /usr/local/include/eredis-hiredis.h /usr/local/include/

RUN useradd -u 1001 -r -g 0 -d /opt/app-root/src -s /sbin/nologin \
      -c "Default Application User" default

RUN INSTALL_PKGS="libev gcc-c++" && \
    yum install -y --setopt=tsflags=nodocs --nogpgcheck $INSTALL_PKGS && \
    rpm -V $INSTALL_PKGS && \
    yum -y clean all --enablerepo='*'

RUN mkdir /app

WORKDIR /app

ENV FILENAME=seriesSiantel.txt
ENV REDIS_HOST=redis
ENV REDIS_PORT=6379
ENV REDIS_PASS=changeme

RUN echo -e "#!/bin/bash\n/app/redis_load_from_file /data/\$FILENAME \$REDIS_HOST \$REDIS_PORT \$REDIS_PASS" > ./entrypoint.sh
RUN chmod +x ./entrypoint.sh

RUN mkdir /tmp/src

ADD redis_load_from_file.cpp /tmp/src/
ADD FileWatcher.h /tmp/src/
ADD log.h /tmp/src/
ADD log.c /tmp/src/

RUN cd /tmp/src && \
    gcc log.c -o log.o -c && \
    g++ -std=c++17 -w -Wall -pedantic redis_load_from_file.cpp log.o -o redis_load_from_file -O2 -lstdc++fs -leredis && \
    cp ./redis_load_from_file /app && \
    rm -R /tmp/src

USER 1001

VOLUME [ "/data" ]

CMD [ "/app/entrypoint.sh" ]