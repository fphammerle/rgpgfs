#include <gpgme.h>

int rgpgfs_gpgme_get_encrypt_key(gpgme_ctx_t gpgme_ctx, const char *key_name,
                                 gpgme_key_t *key);

int rgpgfs_gpgme_data_to_file(const char *path, gpgme_data_t data);

int rgpgfs_gpgme_encrypt_data_to_file(gpgme_ctx_t gpgme_ctx,
                                      gpgme_key_t recip_keys[],
                                      gpgme_data_t plain_data,
                                      const char *cipher_path);

int rgpgfs_gpgme_encrypt_file_to_file(gpgme_ctx_t gpgme_ctx,
                                      gpgme_key_t recip_keys[],
                                      const char *plain_path,
                                      const char *cipher_path);
