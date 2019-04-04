#!/bin/sh

set -e

if [ -z "$RECIPIENT" ]; then
    echo missing \$RECIPIENT >&2
    exit 1
fi

if [ ! -d "$SOURCE_DIR" ]; then
    echo missing source dir "$SOURCE_DIR" >&2
    echo add -v /somewhere:"$SOURCE_DIR":ro >&2
    exit 1
fi

if [ ! -d "$CIPHER_DIR" ]; then
    echo missing mount point "$CIPHER_DIR" >&2
    echo add -v /somewhere:"$CIPHER_DIR":shared >&2
    exit 1
fi

function key_available {
    gpg --quiet --list-public-keys "$RECIPIENT" > /dev/null
}
recv_retries=0
while [ $recv_retries -lt 3 ] && ! key_available; do
    [ $recv_retries -ne 0 ] && sleep 1s
    (set -x; gpg --receive-keys "$RECIPIENT") || true
    recv_retries=$((recv_retries + 1))
done
if ! key_available; then
    echo failed to fetch recipient\'s key >&2
    exit 1
fi

set -x

grep -q "^trust-model always$" ~/.gnupg/gpg.conf 2> /dev/null \
    || echo trust-model always | tee ~/.gnupg/gpg.conf

trap 'fusermount3 -u -z "$CIPHER_DIR"' SIGTERM
rgpgfs -f -o allow_other \
    -o modules=subdir,subdir="$SOURCE_DIR" \
    -o recipient="$RECIPIENT" \
    "$CIPHER_DIR" &
wait $!
