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

// Structs //

struct TextObject {
  char *deflatedContent;
  char *inflatedContent;
  char *checksum;
};

struct BinaryTree {
  Node       *root;         // The node that holds them all.
  TextObject *parent; // Who's the father ? (allows access to the parent TextObject from which the BinaryTree is derived from)
};

struct Node {
  char letter; // single letter value.
  u_int freq;  
  Node *children; // children letters.
};

struct FrequencyTable {
  char * keys;
  u_int * freq;
};

////////////


FrequencyTable * get_char_freq_table(char * s) {
  FrequencyTable *ft;
  ft = malloc(sizeof(FrequencyTable));
  ft->keys = malloc(strlen(s)*sizeof(char));
  ft->freq = calloc(0, sizeof(u_int));
  // TODO: Handle malloc errors.

  for (int i = 0; i < strlen(s); i++) {
    bool char_found = false;
    for (int j = 0; j < strlen(ft->keys); j++) {
      if (ft->keys[j] == s[i]) {
        char_found = true;
        ft->freq[i]++;
      }
    }
    if (!char_found) {
       char * temp = realloc(ft->keys, strlen(ft->keys)+2);
       if (temp == NULL) {
         free(ft->keys);
         free(ft->freq);
         free(ft);
         return NULL;
       }
       ft->keys = temp;
        
       ft->keys[strlen(ft->keys)-1] = s[i];
       
       temp = realloc(ft->freq, sizeof(u_int)*strlen(ft->keys));  
       if (temp == NULL) {
         free(ft->keys);
         free(ft->freq);
         free(ft);
         return NULL;
       }
       ft->freq[strlen(ft->keys)-1] = 1;
    } 
  }
  return ft;
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
