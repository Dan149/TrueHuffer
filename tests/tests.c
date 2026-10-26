#include "../src/compress.h"
#include "../src/debug.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {

  char *buf;

  // Reading file //
  FILE *fptr = fopen("./tests/lorem.txt", "rb");
  assert(fptr);

  // Get file size
  fseek(fptr, 0, SEEK_END);     // moving cursor to EOF.
  long file_size = ftell(fptr); // getting byte amount.
  fseek(fptr, 0, SEEK_SET);     // moving back to top.

  buf = (char *)malloc(file_size + 1);
  if (!buf) {
    fclose(fptr);
    return EXIT_FAILURE;
  }

  size_t bytes_read = fread(buf, 1, file_size, fptr);
  buf[bytes_read] = '\0';
  fclose(fptr);

  // ENd of file reading //

  FrequencyTable *ft = get_char_freq_table(buf);

  print_freq_table(ft);

  BinaryTree *bt = build_btree(ft);

  print_btree(bt);

  free_btree(bt);

  free_freq_t(ft);

  return EXIT_SUCCESS;
}
