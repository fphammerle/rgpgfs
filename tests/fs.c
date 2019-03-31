#include "src/fs.h"

#define _GNU_SOURCE
#include <errno.h>
#include <ftw.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_BUF_LEN 256

void test_mkdirs(const char *temp_dir, const char *rel_path) {
  char abs_path[PATH_BUF_LEN];
  strcpy(abs_path, temp_dir);
  strcat(abs_path, "/");
  strcat(abs_path, rel_path);
  if (rgpgfs_fs_mkdirs(abs_path)) {
    fprintf(stderr, "rel_path = '%s'\n", rel_path);
    perror("rgpgfs_fs_mkdirs failed");
    exit(1);
  }

  FILE *f = fopen(abs_path, "w");
  assert(f);
  fclose(f);
};

int cleanup_rm_cb(const char *fpath, const struct stat *sb, int typeflag,
                  struct FTW *ftwbuf) {
  if (remove(fpath)) {
    perror(fpath);
  }
  return 0;
}

int main() {
  char temp_dir[] = "/tmp/rgpgfs-tests-fs-XXXXXX";
  assert(mkdtemp(temp_dir));
  // printf("temp dir: %s\n", temp_dir);

  test_mkdirs(temp_dir, "file");
  test_mkdirs(temp_dir, "a/file");
  test_mkdirs(temp_dir, "a/b/file");
  test_mkdirs(temp_dir, "c/d/file");

  nftw(temp_dir, cleanup_rm_cb, 8, FTW_DEPTH);
  return 0;
}
