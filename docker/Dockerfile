FROM shuppet/eredis

WORKDIR /app

ADD hosts.conf .
ADD app.c .

RUN gcc app.c -o app.o -O2
