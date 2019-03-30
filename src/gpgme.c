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

int rgpgfs_gpgme_encrypt_data_to_file(gpgme_ctx_t gpgme_ctx,
                                      gpgme_key_t recip_keys[],
                                      gpgme_data_t plain_data,
                                      const char *cipher_path) {
  gpgme_data_t cipher_data;
  if (gpgme_data_new(&cipher_data) != GPG_ERR_NO_ERROR) {
    fprintf(stderr, "rgpgfs_gpgme_encrypt_data_to_file: failed to prepare "
                    "cipher data container\n");
    return 1;
  }

  int result = 0;

  if (gpgme_op_encrypt(gpgme_ctx, recip_keys, 0, plain_data, cipher_data) !=
      GPG_ERR_NO_ERROR) {
    fprintf(stderr, "rgpgfs_gpgme_encrypt_data_to_file: failed to encrypt\n");
    result = 1;
  } else if (rgpgfs_gpgme_data_to_file(cipher_path, cipher_data)) {
    fprintf(stderr, "rgpgfs_gpgme_encrypt_data_to_file: failed to write cipher "
                    "data to disk\n");
    result = 1;
  }

  gpgme_data_release(cipher_data);
  return result;
}
