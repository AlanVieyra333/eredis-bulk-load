FROM shuppet/eredis:latest as EREDIS

#FROM registry.redhat.io/rhel-atomic
#FROM alpine:3.9
#FROM debian:stretch
FROM centos:8
LABEL maintainer="Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>"

COPY --from=EREDIS /usr/local/lib/liberedis.so /lib64/liberedis.so

RUN useradd -u 1001 -r -g 0 -d /opt/app-root/src -s /sbin/nologin \
      -c "Default Application User" default

RUN INSTALL_PKGS="libev" && \
    yum install -y --setopt=tsflags=nodocs --nogpgcheck $INSTALL_PKGS && \
    rpm -V $INSTALL_PKGS && \
    yum -y clean all --enablerepo='*'

WORKDIR /app

RUN echo -e "#!/bin/bash\n/app/redis_load_from_file /data/\$FILENAME \$REDIS_HOST \$REDIS_PORT \$REDIS_PASS" > ./entrypoint.sh
RUN chmod +x ./entrypoint.sh

ENV FILENAME=R09_80000000.txt
ENV REDIS_HOST=redis
ENV REDIS_PORT=6379
ENV REDIS_PASS=changeme

VOLUME [ "/data" ]

ADD ./redis_load_from_file .

USER 1001

CMD [ "/app/entrypoint.sh" ]