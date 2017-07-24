/**
 * On Windows, a Unicode argument is passed as UTF-16 using ShellExecuteExW.
 * On other platforms, it is passed as UTF-8
 */

static const int args_length = 4;

#include <string.h>
#include <stdio.h>

static const char* expected_utf8[args_length] = {
  // Latin-1
  "M\xC3\xB8z\xC3\xAEll\xC3\xA5",
  // Cyrillic
  "\xD0\x9C\xD0\xBE\xD0\xB7\xD0\xB8\xD0\xBB\xD0\xBB\xD0\xB0",
  // Bengali
  "\xE0\xA6\xAE\xE0\xA7\x8B\xE0\xA6\x9C\xE0\xA6\xBF\xE0\xA6\xB2\xE0\xA6\xBE",
  // Cuneiform
  "\xF0\x92\x88\xAC\xF0\x92\x8D\xA3\xF0\x92\x86\xB7"
};

int main(int argc, char* argv[]) {
  if (argc != args_length + 1)
    return -1;

  for (int i = 1; i < argc; ++i) {
    printf("argv[%d] = %s; expected = %s\n", i, argv[i], expected_utf8[i - 1]);
    if (strcmp(expected_utf8[i - 1], argv[i])) {
      return i;
    }
  }

  return 0;
}
