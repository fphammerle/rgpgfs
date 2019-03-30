FROM alpine:3.9 as build

RUN apk add --no-cache \
    fuse3-dev \
    gcc \
    gpgme-dev \
    libc-dev \
    make \
    pkgconf

RUN apk add --no-cache fuse `# convenient`

RUN adduser -S build
USER build

COPY --chown=build:nogroup . /rgpgfs
WORKDIR /rgpgfs
RUN make
