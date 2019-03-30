#include "src/fs.h"

// posix
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

int rgpgfs_fs_mkdirs(char *path) {
  char *delimiter = strrchr(path, '/');
  if (delimiter == NULL) {
    errno = ENOTSUP;
    return 1;
  }
  *delimiter = '\0';
  struct stat statbuf;
  if (lstat(path, &statbuf) &&
      (rgpgfs_fs_mkdirs(path) || mkdir(path, S_IRWXU))) {
    *delimiter = '/';
    return 1;
  }
  *delimiter = '/';
  return 0;
}
