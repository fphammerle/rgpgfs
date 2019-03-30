# Build

```sh
apt-get install libfuse3-dev libgpgme-dev
make
```

# Usage

## Change source directory

```sh
rgpgfs -o modules=subdir -o subdir=/source/dir /mount/point
```
