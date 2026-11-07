#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
//
#include "../src/compress.h"
#include "../src/debug.h"

int main() {

  printf("Loading lorem.txt...\n\n");
  TextObject *to = init_text_obj("./tests/lorem.txt");
  if (!to)
    return EXIT_FAILURE;
  printf("%s\n\n", to->inflatedContent);
  printf("deflating...\n\n");

  deflate(to);

  printf("%s\n\n", to->deflatedContent);

  free_text_obj(to);

  // End of file reading //

  // FrequencyTable *ft = get_char_freq_table(buf);
  //
  // print_freq_table(ft);
  //
  // BinaryTree *bt = build_btree(ft);
  //
  // print_btree(bt);
  //
  // free_btree(bt);
  //
  // free_freq_t(ft);

  return EXIT_SUCCESS;
}
