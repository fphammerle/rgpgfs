FROM alpine:3.9 as build

RUN apk add --no-cache \
    fuse3-dev \
    gcc \
    gpgme-dev \
    libc-dev \
    make \
    pkgconf

RUN adduser -S build
USER build

COPY --chown=build:nogroup Makefile /rgpgfs/
COPY --chown=build:nogroup src /rgpgfs/src
WORKDIR /rgpgfs
RUN make


FROM build as test

COPY --chown=build:nogroup tests /rgpgfs/tests
RUN make tests/str && tests/str \
    && make tests/fs && tests/fs


FROM alpine:3.9 as runtime

RUN apk add --no-cache \
    fuse3 \
    gpgme

RUN echo user_allow_other >> /etc/fuse.conf

RUN adduser -S encrypt
USER encrypt

COPY --from=build /rgpgfs/rgpgfs /usr/local/bin/

COPY --chown=encrypt:nogroup docker/ash_history /home/encrypt/.ash_history


FROM runtime as unattended

ENV RECIPIENT= \
    SOURCE_DIR=/plain \
    CIPHER_DIR=/encrypted

COPY docker/rgpgfs_unattended.sh /
CMD ["/rgpgfs_unattended.sh"]
