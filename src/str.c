#include <stdio.h>
#include <string.h>

int rgpgfs_strncpy_without_suffix(char *dest, const char *src,
                                  const char *suffix, size_t max_len) {
  size_t src_len = strnlen(src, FILENAME_MAX);
  size_t suffix_len = strnlen(suffix, FILENAME_MAX);
  if (suffix_len > src_len) {
    return 1;
  }
  size_t suffix_pos = src_len - suffix_len;
  if (suffix_pos > max_len || strcmp(&src[suffix_pos], suffix) != 0) {
    return 1;
  }
  strncpy(dest, src, suffix_pos);
  dest[suffix_pos] = '\0';
  return 0;
}
