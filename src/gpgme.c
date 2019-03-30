#include "src/gpgme.h"

#include <gpgme.h>

#include <stdio.h>

int rgpgfs_gpgme_data_to_file(const char *path, gpgme_data_t data) {
  if (gpgme_data_seek(data, 0, SEEK_SET) != 0) {
    perror("rgpgfs_gpgme_data_to_file: failed to seek");
    return 1;
  }

  FILE *file = fopen(path, "wb");
  if (file == NULL) {
    perror("rgpgfs_gpgme_data_to_file: failed to open file");
    return 1;
  }

  ssize_t count;
  char buf[BUFSIZ];
  while ((count = gpgme_data_read(data, buf, BUFSIZ)) > 0) {
    if (fwrite(buf, 1, count, file) != count) {
      fprintf(stderr,
              "rgpgfs_gpgme_data_to_file: failed to write data to file");
      return 1;
    }
  }
  if (count != 0) {
    perror("rgpgfs_gpgme_data_to_file: failed to load data into buffer");
    return 1;
  }

  fclose(file);
  return 0;
}
