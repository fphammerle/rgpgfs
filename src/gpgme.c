#include "src/gpgme.h"

#include <gpgme.h>

#include <stdio.h>

int rgpgfs_gpgme_get_encrypt_key(gpgme_ctx_t gpgme_ctx, const char *key_name,
                                 gpgme_key_t *key) {
  gpg_error_t err = gpgme_get_key(gpgme_ctx, key_name, key, 0);
  if (err != GPG_ERR_NO_ERROR) {
    fprintf(stderr, "Failed to load key %s: %s (%d)\n", key_name,
            gpg_strerror(err), err);
    return 1;
  }
  if (!(*key)->can_encrypt) {
    fprintf(stderr, "Selected key %s can not be used for encryption\n",
            (*key)->fpr);
    return 1;
  }
  return 0;
}

static int _rgpgfs_gpgme_write_data_to_file(FILE *file, gpgme_data_t data) {
  ssize_t count;
  char buf[BUFSIZ];
  while ((count = gpgme_data_read(data, buf, BUFSIZ)) > 0) {
    if (fwrite(buf, 1, count, file) != count) {
      fprintf(stderr, "%s: failed to write data to file", __func__);
      return 1;
    }
  }
  if (count != 0) {
    perror("_rgpgfs_gpgme_write_data_to_file: failed to load data into buffer");
    return 1;
  }
  return 0;
}

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
  int res = _rgpgfs_gpgme_write_data_to_file(file, data);
  fclose(file);
  return res;
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

  // list of recipients may implicitly include the default recipient
  // (GPGME_ENCRYPT_NO_ENCRYPT_TO)
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

int rgpgfs_gpgme_encrypt_file_to_file(gpgme_ctx_t gpgme_ctx,
                                      gpgme_key_t recip_keys[],
                                      const char *plain_path,
                                      const char *cipher_path) {
  gpgme_data_t plain_data;
  gpgme_error_t gpgme_read_err =
      gpgme_data_new_from_file(&plain_data, plain_path, 1);
  if (gpgme_read_err != GPG_ERR_NO_ERROR) {
    fprintf(stderr, "%s: failed to read file %s: %s (%d)\n", __func__,
            plain_path, gpg_strerror(gpgme_read_err), gpgme_read_err);
    return 1;
  }
  int result = rgpgfs_gpgme_encrypt_data_to_file(gpgme_ctx, recip_keys,
                                                 plain_data, cipher_path);
  gpgme_data_release(plain_data);
  return result;
}
