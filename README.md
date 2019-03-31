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
docker build --target build -t rgpgfs .
```

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

### Docker 🐳

Mount an enciphered view of named volume `plain-data` at `/mnt/rgpgfs`:

```sh
docker run --rm \
    --device /dev/fuse --cap-add SYS_ADMIN \
    -e RECIPIENT=1234567890ABCDEF1234567890ABCDEF12345678 \
    -v plain-data:/plain:ro \
    -v /mnt/rgpgfs:/encrypted:shared \
    fphammerle/rgpgfs
```

Interactively:

```sh
host$ mkdir /mnt/rgpgfs && chmod a+rwx /mnt/rgpgfs
host$ docker run --rm -it \
    -v plain-data:/plain:ro \
    -v /mnt/rgpgfs:/enc:shared \
    --device /dev/fuse --cap-add SYS_ADMIN \
    fphammerle/rgpgfs ash
container$ ls /plain
example.txt
container$ gpg --recv-keys 1234567890ABCDEF1234567890ABCDEF12345678
container$ gpg --edit-key 1234567890ABCDEF1234567890ABCDEF12345678
container gpg> trust
container gpg> 5
container gpg> quit
container$ rgpgfs -o allow_other,modules=subdir,subdir=/plain,recipient=12345678 /enc
container$ ls /enc
example.txt.gpg
# meanwhile in another shell:
host$ ls /mnt/rgpgfs
example.txt.gpg
```

When AppArmor is enabled
you may need to add `--security-opt apparmor:unconfined`.

You may need to disable user namespace remapping for containers
(dockerd option `--userns-remap`)
due to https://github.com/moby/moby/issues/36472 .
