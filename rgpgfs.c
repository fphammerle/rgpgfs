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
// https://www.gnupg.org/documentation/manuals/gpgme/Function-and-Data-Index.html
#include <gpgme.h>

#define FUSE_PATH_BUF_LEN 256
static char cache_dir[] = "/tmp/rgpgfs-cache-XXXXXX";
static const size_t CACHE_PATH_BUF_LEN = sizeof(cache_dir) + FUSE_PATH_BUF_LEN;

static gpgme_ctx_t gpgme_ctx;
static const char gpgme_recip_fpr[] =
    "1234567890ABCDEF1234567890ABCDEF12345678";
static gpgme_key_t gpgme_recip_key;

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
  // fprintf(stderr, "rgpgfs_encrypt('%s', %p)\n", source_path, cache_path);
  size_t source_path_len = strnlen(source_path, FUSE_PATH_BUF_LEN);
  if (source_path_len >= FUSE_PATH_BUF_LEN) {
    errno = ENAMETOOLONG;
    perror("rgpgfs_encrypt");
    return 1;
  }
  strcpy(cache_path, cache_dir);
  strcat(cache_path, source_path);

  struct stat source_stat;
  if (lstat(source_path, &source_stat)) {
    perror("rgpgfs_encrypt: could not stat source file");
    return 1;
  }

  struct stat cache_stat;
  if (lstat(cache_path, &cache_stat) ||
      source_stat.st_mtim.tv_sec > cache_stat.st_mtim.tv_sec) {
    if (rgpgfs_mkdirs(cache_path)) {
      perror("rgpgfs_encrypt: failed to create dirs");
      return 1;
    }

    FILE *cache_file = fopen(cache_path, "w");
    if (cache_file == NULL) {
      perror("rgpgfs_encrypt: failed to open cache file");
      return 1;
    }
    fprintf(cache_file, "path: %s\n", source_path);
    fprintf(cache_file, "size: %lu bytes\n", source_stat.st_size);
    fprintf(cache_file, "mod time: %ld\n", source_stat.st_mtim.tv_sec);
    fclose(cache_file);
    printf("encrypted %s\n", source_path);
  }

  return 0;
}

static int rgpgfs_getattr(const char *source_path, struct stat *statbuf,
                          struct fuse_file_info *fi) {
  if (lstat(source_path, statbuf))
    return -errno;
  if (!S_ISDIR(statbuf->st_mode)) {
    char cache_path[CACHE_PATH_BUF_LEN];
    if (rgpgfs_encrypt(source_path, cache_path))
      return -errno;
    if (lstat(cache_path, statbuf))
      return -errno;
  }
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
  // fprintf(stderr, "rgpgfs_open('%s', %p)", source_path, fi);
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
  printf("gpgme version: %s\n", gpgme_check_version(NULL));
  gpg_error_t gpgme_init_err = gpgme_new(&gpgme_ctx);
  if (gpgme_init_err != GPG_ERR_NO_ERROR) {
    fprintf(stderr, "Failed to initialize gpgme: %s (%d)\n",
            gpg_strerror(gpgme_init_err), gpgme_init_err);
    return 1;
  }
  gpg_error_t gpgme_get_key_err =
      gpgme_get_key(gpgme_ctx, gpgme_recip_fpr, &gpgme_recip_key, 0);
  switch (gpgme_get_key_err) {
  case GPG_ERR_NO_ERROR:
    break;
  case GPG_ERR_EOF:
    fprintf(stderr, "Could not find key %s\n", gpgme_recip_fpr);
    return 1;
  case GPG_ERR_AMBIGUOUS_NAME:
    fprintf(stderr, "Key name '%s' is ambiguous\n", gpgme_recip_fpr);
    return 1;
  case GPG_ERR_INV_VALUE:
  default:
    fprintf(stderr, "Failed to load key %s: %s (%d)\n", gpgme_recip_fpr,
            gpg_strerror(gpgme_init_err), gpgme_get_key_err);
    return 1;
  }
  if (!gpgme_recip_key->can_encrypt) {
    fprintf(stderr, "Selected key %s can not be used for encryption\n",
            gpgme_recip_key->fpr);
    return 1;
  }
  printf("key fingerprint: %s\n", gpgme_recip_key->fpr);
  // TODO rm -r cache_dir (see man nftw)
  int fuse_main_err = fuse_main(argc, argv, &rgpgfs_fuse_operations, NULL);
  gpgme_release(gpgme_ctx);
  return fuse_main_err;
}
