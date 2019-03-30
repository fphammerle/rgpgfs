#include "src/fs.h"
#include "src/gpgme.h"
#include "src/str.h"

// http://libfuse.github.io/doxygen/globals.html
#define FUSE_USE_VERSION 31
#include <fuse.h>
// https://www.gnupg.org/documentation/manuals/gpgme/Function-and-Data-Index.html
#include <gpgme.h>

// posix
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FUSE_PATH_BUF_LEN 256
static char cache_dir[] = "/tmp/rgpgfs-cache-XXXXXX";
static const size_t CACHE_PATH_BUF_LEN = sizeof(cache_dir) + FUSE_PATH_BUF_LEN;

static const char ENC_SUFFIX[] = ".gpg";

static gpgme_ctx_t gpgme_ctx;
static gpgme_key_t gpgme_recip_key;

static struct { char *recipient_name; } rgpgfs_config;

static struct fuse_opt rgpgfs_opts[] = {{"--recipient %s", 0, 0},
                                        {"--recipient=%s", 0, 0},
                                        {"-r %s", 0, 0},
                                        {"recipient=%s", 0, 0},
                                        FUSE_OPT_END};

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
    perror("rgpgfs_encrypt: failed to stat source file");
    return 1;
  }

  struct stat cache_stat;
  if (lstat(cache_path, &cache_stat) ||
      source_stat.st_mtim.tv_sec > cache_stat.st_mtim.tv_sec) {
    if (rgpgfs_fs_mkdirs(cache_path)) {
      perror("rgpgfs_encrypt: failed to create dirs");
      return 1;
    }

    // list of recipients may implicitly include the default recipient
    gpgme_key_t recip_keys[] = {gpgme_recip_key, NULL};
    if (rgpgfs_gpgme_encrypt_file_to_file(gpgme_ctx, recip_keys, source_path,
                                          cache_path)) {
      fprintf(stderr, "%s: failed to create encrypted cache of %s\n", __func__,
              source_path);
      return 1;
    }
    printf("encrypted %s\n", source_path);
  }

  return 0;
}

static int rgpgfs_getattr(const char *mount_path, struct stat *statbuf,
                          struct fuse_file_info *fi) {
  if (!lstat(mount_path, statbuf)) {
    if (S_ISREG(statbuf->st_mode)) {
      fprintf(stderr, "%s: tried to access path without %s suffix: %s\n",
              __func__, ENC_SUFFIX, mount_path);
      return -ENOENT; // missing ENC_SUFFIX
    }
    return 0;
  }

  char source_path[FUSE_PATH_BUF_LEN];
  if (rgpgfs_strncpy_without_suffix(source_path, mount_path, ENC_SUFFIX,
                                    FUSE_PATH_BUF_LEN - 1)) {
    return -ENOENT;
  }

  char cache_path[CACHE_PATH_BUF_LEN];
  if (rgpgfs_encrypt(source_path, cache_path))
    return -errno;
  if (lstat(cache_path, statbuf))
    return -errno;
  return 0;
}

static int rgpgfs_access(const char *mount_path, int mask) {
  struct stat statbuf;
  if (!lstat(mount_path, &statbuf)) {
    if (S_ISREG(statbuf.st_mode)) {
      fprintf(stderr, "%s: tried to access path without %s suffix: %s\n",
              __func__, ENC_SUFFIX, mount_path);
      return -ENOENT;
    }
    if (access(mount_path, mask)) {
      return -errno;
    }
    return 0;
  }

  char source_path[FUSE_PATH_BUF_LEN];
  if (rgpgfs_strncpy_without_suffix(source_path, mount_path, ENC_SUFFIX,
                                    FUSE_PATH_BUF_LEN - 1)) {
    fprintf(stderr, "%s: invalid suffix in path: %s\n", __func__, mount_path);
    return -ENOENT;
  }
  if (access(source_path, mask)) {
    return -errno;
  }
  return 0;
}

static int rgpgfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
  DIR *dirp = opendir(path);
  if (dirp == NULL) {
    return -errno;
  }

  errno = 0;

  struct dirent *entp;
  const size_t dirent_name_max_len = sizeof(entp->d_name);
  while ((entp = readdir(dirp)) != NULL) {
    struct stat statbf;
    memset(&statbf, 0, sizeof(statbf));
    statbf.st_ino = entp->d_ino;
    statbf.st_mode = entp->d_type << 12;
    if (S_ISREG(statbf.st_mode)) {
      if (strnlen(entp->d_name, dirent_name_max_len) + strlen(ENC_SUFFIX) >=
          dirent_name_max_len) {
        errno = ENAMETOOLONG;
        break;
      }
      strcat(entp->d_name, ENC_SUFFIX);
    }
    if (filler(buf, entp->d_name, &statbf, 0, 0)) {
      errno = ENOBUFS;
      break;
    }
  }

  closedir(dirp);
  return -errno;
}

static int rgpgfs_open(const char *mount_path, struct fuse_file_info *fi) {
  char source_path[FUSE_PATH_BUF_LEN];
  if (rgpgfs_strncpy_without_suffix(source_path, mount_path, ENC_SUFFIX,
                                    FUSE_PATH_BUF_LEN - 1)) {
    fprintf(stderr, "%s: invalid suffix in path: %s\n", __func__, mount_path);
    return -ENOENT;
  }

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
  if (bytes_num == -1) {
    return -errno;
  }
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
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  rgpgfs_config.recipient_name = NULL;
  fuse_opt_parse(&args, &rgpgfs_config, rgpgfs_opts, NULL);
  printf("gpgme version: %s\n", gpgme_check_version(NULL));
  gpg_error_t gpgme_init_err = gpgme_new(&gpgme_ctx);
  if (gpgme_init_err != GPG_ERR_NO_ERROR) {
    fprintf(stderr, "Failed to initialize gpgme: %s (%d)\n",
            gpg_strerror(gpgme_init_err), gpgme_init_err);
    return 1;
  }
  if (rgpgfs_config.recipient_name == NULL) {
    fprintf(stderr, "Missing parameter --recipient\n");
    return 1;
  }
  printf("recipient name: %s\n", rgpgfs_config.recipient_name);
  if (rgpgfs_gpgme_get_encrypt_key(gpgme_ctx, rgpgfs_config.recipient_name,
                                   &gpgme_recip_key)) {
    gpgme_release(gpgme_ctx);
    return 1;
  }
  gpgme_user_id_t gpgme_recip_id = gpgme_recip_key->uids;
  while (gpgme_recip_id != NULL) {
    printf("recipient: %s\n", gpgme_recip_id->uid);
    gpgme_recip_id = gpgme_recip_id->next;
  }
  printf("recipient fingerprint: %s\n", gpgme_recip_key->fpr);
  if (mkdtemp(cache_dir) == NULL) {
    return 1;
  }
  printf("cache: %s\n", cache_dir);
  int fuse_main_err =
      fuse_main(args.argc, args.argv, &rgpgfs_fuse_operations, NULL);
  // TODO rm -r cache_dir (see man nftw)
  gpgme_release(gpgme_ctx);
  return fuse_main_err;
}
