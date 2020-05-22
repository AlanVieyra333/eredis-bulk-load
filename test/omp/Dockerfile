FROM centos:8
LABEL maintainer="Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>"

RUN useradd -u 1001 -r -g 0 -d /opt/app-root/src -s /sbin/nologin \
      -c "Default Application User" default

RUN INSTALL_PKGS="libev gcc-c++" && \
    yum install -y --setopt=tsflags=nodocs --nogpgcheck $INSTALL_PKGS && \
    rpm -V $INSTALL_PKGS && \
    yum -y clean all --enablerepo='*'

RUN mkdir /app

WORKDIR /app

ARG APP=matrices

RUN mkdir /tmp/src

ADD $APP.cpp /tmp/src/

RUN cd /tmp/src && \
    g++ -o $APP.o -fopenmp $APP.cpp && \
    cp ./$APP.o /app/$APP && \
    rm -R /tmp/src

RUN echo -e "#!/bin/bash\n/app/$APP " > ./entrypoint.sh
RUN chmod +x ./entrypoint.sh

USER 1001

CMD [ "/app/entrypoint.sh" ]