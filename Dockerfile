FROM alanvieyra333/hiredispool:latest as HIREDISPOOL

FROM registry.redhat.io/codeready-workspaces/stacks-cpp-rhel8
LABEL maintainer="Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>"

COPY --from=HIREDISPOOL /usr/local/include/hiredis/ /usr/local/include/
COPY --from=HIREDISPOOL /usr/local/lib/libhiredis.so.0.13 /lib64/libhiredis.so

# Install libev library.
COPY ./yum.repos.d/* /etc/yum.repos.d/

USER 0
RUN rm /etc/yum.repos.d/ubi*
RUN curl http://www.centos.org/keys/RPM-GPG-KEY-CentOS-Official >/etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-Official
RUN yum provides 'libev(x86-32)' 'libev-devel(x86-64)'
RUN yum install -y libev-devel
USER 1001

# Environments.
ENV REDIS_HOST=redis
ENV REDIS_PORT=6379
ENV REDIS_PASS=changeme
ENV REDIS_DATABASE=0
ENV FILENAME=filename.txt

COPY src/ .
RUN make

VOLUME [ "/data" ]

CMD ["sh", "-c", "/projects/main.o $REDIS_HOST $REDIS_PORT $REDIS_PASS $REDIS_DATABASE /data/$FILENAME"]
