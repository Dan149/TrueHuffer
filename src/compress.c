// std
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h> // YANIS. Learn ts please.
#include <string.h> // YANIS. Also, check docs for this.

#if defined(_WIN32) // Checks OS an import corresponding library to use u_int
#include <winsock.h>
#elif defined(__linux__)
#include <sys/types.h>
#else
return EXIT_FAILURE;
#endif

// src
#include "compress.h"


////////////


FrequencyTable * get_char_freq_table(char * s) {
  FrequencyTable *ft;
  ft = malloc(sizeof(FrequencyTable));
  ft->keys = malloc(1);
  ft->freq = malloc(sizeof(u_int));
  ft->freq=NULL;
  // TODO: Handle malloc errors.
  for (long unsigned int i = 0; i < strlen(s); i++) {
    bool char_found = false;
    for (long unsigned int j = 0; j < strlen(ft->keys); j++) {
      if (ft->keys[j] == s[i]) {
        char_found = true;
        ft->freq[j]++;
        break;
      }
    }
    if (!char_found) {
      size_t cur_len = strlen(ft->keys);

      char * k_tmp = realloc(ft->keys, cur_len+2);
      if (k_tmp == NULL) {
        free_freq_t(ft);
        return NULL;
      }
      ft->keys = k_tmp; 
      ft->keys[cur_len] = s[i];  
      ft->keys[cur_len+1] = '\0';  
      u_int * f_tmp;
      f_tmp = realloc(ft->freq, sizeof(u_int)*(cur_len+1));  
      if (f_tmp == NULL) {
        free_freq_t(ft);
        return NULL;
      }
      ft->freq = f_tmp;
      ft->freq[cur_len] = 1;
    } 
  }
  return ft;
}

void free_freq_t(FrequencyTable * ft) {
  if (ft == NULL) return;
  free(ft->freq);
  free(ft->keys);
  free(ft);
}

TextObject *initTextObject(char * dc, char * ic) {
  // assume deflatedContent & inflatedContent either NULL or allocated, non-empty string :
  assert((dc == NULL || strlen(dc) != 0) && (ic == NULL || strlen(ic) != 0)); 

  TextObject *to;
  to = malloc(sizeof(TextObject)); // to malloc or not to malloc ? That is the question. (seriously, I'm not sure this is good design, have to check later.)

  to->inflatedContent=ic;
  to->deflatedContent=dc;

  return to;
}
