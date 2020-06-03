FROM shuppet/eredis:latest as EREDIS

FROM registry.redhat.io/codeready-workspaces/stacks-cpp-rhel8
LABEL maintainer="Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>"

COPY --from=EREDIS /usr/local/lib/liberedis.so /lib64/
COPY --from=EREDIS /usr/local/include/eredis.h /usr/local/include/
COPY --from=EREDIS /usr/local/include/eredis-hiredis.h /usr/local/include/

# Install libev library.
COPY yum.repos.d/*.repo /etc/yum.repos.d/
COPY yum.repos.d/RPM-GPG-KEY-CentOS-Official /etc/pki/rpm-gpg/

USER 0
RUN rm /etc/yum.repos.d/ubi*
RUN yum provides 'libev(x86-32)' 'libev-devel(x86-64)'
RUN yum install -y libev-devel
USER 1001

# Environments.
ENV FILENAME=filename.txt
ENV REDIS_HOST=redis
ENV REDIS_PORT=6379
ENV REDIS_PASS=changeme
ENV REDIS_DATABASE=0

# Program compilation.
ARG APP=redis_data_load_series

RUN mkdir /tmp/src

COPY $APP.cpp /tmp/src/
COPY FileWatcher.h /tmp/src/
COPY log.h /tmp/src/
COPY log.c /tmp/src/

RUN cd /tmp/src && \
    gcc log.c -o log.o -c && \
    g++ -std=c++17 -w -Wall -pedantic $APP.cpp log.o -o $APP.o -O2 -lstdc++fs -leredis -fopenmp && \
    cp ./$APP.o /projects/redis_data_load && \
    rm -R /tmp/src

VOLUME [ "/data" ]

CMD ["sh", "-c", "/projects/redis_data_load /data/$FILENAME $REDIS_HOST $REDIS_PORT $REDIS_PASS $REDIS_DATABASE"]