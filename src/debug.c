#include "debug.h"
#include "compress.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>


void print_freq_table(FrequencyTable *ft) {
  assert(ft != NULL && ft->keys != NULL && ft->freq != NULL);
  for (long unsigned int i = 0; i < strlen(ft->keys); i++) {
    printf("\nchar: %c | frequency: %d", ft->keys[i], ft->freq[i]);
  }
}
