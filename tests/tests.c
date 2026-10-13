#include "../src/debug.h"
#include "../src/compress.h"
#include <stdlib.h>
#include <stdio.h>


int main() {

  char * str;
  str = malloc(20);
  sprintf(str, "azzoxiej139821ejfie");

  FrequencyTable * ft = get_char_freq_table(str);

  print_freq_table(ft);
  free(str);
  free_freq_t(ft);

  return EXIT_SUCCESS;
}
