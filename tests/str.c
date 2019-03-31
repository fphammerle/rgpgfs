#include "src/str.h"

#include <assert.h>
#include <string.h>

void test_strncpy_without_suffix(const char *expected_dest, const char *src,
                                 const char *suffix, size_t max_len) {
  char dest[32] = "default";
  int result = rgpgfs_strncpy_without_suffix(dest, src, suffix, max_len);
  if (expected_dest == NULL) {
    assert(result);
    assert(!strcmp(dest, "default"));
  } else {
    assert(!result);
    assert(!strcmp(dest, expected_dest));
  }
}

int main() {
  test_strncpy_without_suffix("abc", "abc", "", 8);
  test_strncpy_without_suffix("ab", "abc", "c", 8);
  test_strncpy_without_suffix("a", "abc", "bc", 8);
  test_strncpy_without_suffix("", "abc", "abc", 8);
  test_strncpy_without_suffix("abcda", "abcdabc", "bc", 8);
  test_strncpy_without_suffix(NULL, "abc", "d", 8);
  test_strncpy_without_suffix("a", "abc", "bc", 1);
  test_strncpy_without_suffix("ab", "abc", "c", 2);
  test_strncpy_without_suffix(NULL, "abc", "c", 1);
  test_strncpy_without_suffix(NULL, "abc", "abcd", 8);
  test_strncpy_without_suffix(NULL, "bcd", "abcd", 8);
  test_strncpy_without_suffix("/folder/sub/file.txt",
                              "/folder/sub/file.txt.gpg", ".gpg", 24);
  return 0;
}
