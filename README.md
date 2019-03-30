# rgpgfs 💾 🔐

PoC: PGP/GPG-enciphered view of plain directories

Mounting & unmounting does *not* require setuid, sudo, root ...

## Build

Run `make` after installing
[libfuse](https://github.com/libfuse/libfuse)
and
[gpgme](https://www.gnupg.org/software/gpgme/index.html).

### Debian / Ubuntu

```sh
apt-get install libfuse3-dev libgpgme-dev
make
```

### Docker 🐳

```sh
docker build -t rgpgfs .
docker run --rm -it --device /dev/fuse \
    --cap-add SYS_ADMIN rgpgfs
```

You may need to add `--security-opt apparmor:unconfined`.

## Usage

```sh
rgpgfs -r [fingerprint] [mountpoint]
# or
rgpgfs --recipient=[fingerprint] [mountpoint]
# or
rgpgfs -o recipient=[fingerprint] [mountpoint]
```

*rgpgfs* will refuse to encrypt with untrusted keys.
See `gpg -k [fingerprint]`.

### Example

Mount encrypted view of `/` in `~/rgpgfs`:

```sh
$ rgpgfs --recipient 1234567890ABCDEF1234567890ABCDEF12345678 ~/rgpgfs

$ ls -1 ~/rgpfs/var/log/syslog.*
/home/me/rgpgfs/var/log/syslog.gpg
/home/me/rgpgfs/var/log/syslog.1.gpg
/home/me/rgpgfs/var/log/syslog.2.gz.gpg
/home/me/rgpgfs/var/log/syslog.3.gz.gpg

$ gpg --decrypt --for-your-eyes-only /home/me/rgpgfs/var/log/syslog.gpg | wc -l
gpg: encrypted with 4096-bit RSA key, ID 89ABCDEF12345678, created 2019-03-30
      "someone <someone@somewhere.me>"
3141
```

### Change source directory

```sh
rgpgfs -o modules=subdir -o subdir=/source/dir /mount/point
```
