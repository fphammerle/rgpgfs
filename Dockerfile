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

COPY --chown=build:nogroup . /rgpgfs
WORKDIR /rgpgfs
RUN make


FROM alpine:3.9

RUN apk add --no-cache \
    fuse3 \
    gpgme

RUN echo user_allow_other >> /etc/fuse.conf

# optional, contains fusermount
RUN apk add --no-cache fuse

RUN adduser -S encrypt
USER encrypt

COPY --from=build /rgpgfs/rgpgfs /usr/local/bin/
