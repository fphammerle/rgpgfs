#include <errno.h>
#include <stdio.h>
#include <string.h>

// posix
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define FUSE_USE_VERSION 31
#include <fuse.h>

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

static int rgpgfs_open(const char *path, struct fuse_file_info *fi) {
  int res = open(path, fi->flags);
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
  return fuse_main(argc, argv, &rgpgfs_fuse_operations, NULL);
}
