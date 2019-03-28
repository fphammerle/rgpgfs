#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// posix
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// http://libfuse.github.io/doxygen/globals.html
#define FUSE_USE_VERSION 31
#include <fuse.h>

#define FUSE_PATH_BUF_LEN 256
static char cache_dir[] = "/tmp/rgpgfs-cache-XXXXXX";
static const size_t CACHE_PATH_BUF_LEN = sizeof(cache_dir) + FUSE_PATH_BUF_LEN;

static int rgpgfs_mkdirs(char *path) {
  char *delimiter = strrchr(path, '/');
  if (delimiter == NULL) {
    errno = ENOTSUP;
    return 1;
  }
  *delimiter = '\0';
  struct stat statbuf;
  if (lstat(path, &statbuf) && (rgpgfs_mkdirs(path) || mkdir(path, S_IRWXU))) {
    *delimiter = '/';
    return 1;
  }
  *delimiter = '/';
  return 0;
}

static int rgpgfs_encrypt(const char *source_path, char *cache_path) {
  size_t source_path_len = strnlen(source_path, FUSE_PATH_BUF_LEN);
  if (source_path_len >= FUSE_PATH_BUF_LEN) {
    errno = ENAMETOOLONG;
    perror("rgpgfs_encrypt");
    return 1;
  }
  strcpy(cache_path, cache_dir);
  strcpy(&cache_path[sizeof(cache_dir) - 1], source_path);

  if (rgpgfs_mkdirs(cache_path)) {
    perror("rgpgfs_encrypt: failed to create dirs");
    return 1;
  }

  FILE *cache_file = fopen(cache_path, "w");
  if (cache_file == NULL) {
    perror("rgpgfs_encrypt: failed to open cache file");
    return 1;
  }
  fprintf(cache_file, "path: %s\n", cache_path);
  fclose(cache_file);

  return 0;
}

static int rgpgfs_getattr(const char *path, struct stat *statbuf,
                          struct fuse_file_info *fi) {
  int res = lstat(path, statbuf);
  // printf("rgpgfs_getattr(%s, ...) = %d\n", path, res);
  if (res == -1)
    return -errno;
  return 0;
}

static int rgpgfs_access(const char *path, int mask) {
  int res = access(path, mask);
  // printf("rgpgfs_access(%s, %d) = %d\n", path, mask, res);
  if (res == -1)
    return -errno;
  return 0;
}

static int rgpgfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
  DIR *dirp = opendir(path);
  if (dirp == NULL)
    return -errno;

  struct dirent *entp;
  while ((entp = readdir(dirp)) != NULL) {
    struct stat statbf;
    memset(&statbf, 0, sizeof(statbf));
    statbf.st_ino = entp->d_ino;
    statbf.st_mode = entp->d_type << 12;
    if (filler(buf, entp->d_name, &statbf, 0, 0))
      break;
  }

  closedir(dirp);
  return 0;
}

static int rgpgfs_open(const char *source_path, struct fuse_file_info *fi) {
  char cache_path[CACHE_PATH_BUF_LEN];
  if (rgpgfs_encrypt(source_path, cache_path))
    return -errno;
  int res = open(cache_path, fi->flags);
  if (res == -1)
    return -errno;
  fi->fh = res;
  return 0;
}

static int rgpgfs_read(const char *path, char *buf, size_t count, off_t offset,
                       struct fuse_file_info *fi) {
  if (fi == NULL) {
    return ENOTSUP;
  }
  ssize_t bytes_num = pread(fi->fh, buf, count, offset);
  if (bytes_num == -1)
    return -errno;
  return bytes_num;
}

static int rgpgfs_release(const char *path, struct fuse_file_info *fi) {
  close(fi->fh);
  return 0;
}

static struct fuse_operations rgpgfs_fuse_operations = {
    .getattr = rgpgfs_getattr,
    .open = rgpgfs_open,
    .read = rgpgfs_read,
    .release = rgpgfs_release,
    .readdir = rgpgfs_readdir,
    .access = rgpgfs_access,
};

int main(int argc, char *argv[]) {
  if (mkdtemp(cache_dir) == NULL) {
    return 1;
  }
  printf("cache: %s\n", cache_dir);
  // TODO rm -r cache_dir (see man nftw)
  return fuse_main(argc, argv, &rgpgfs_fuse_operations, NULL);
}
