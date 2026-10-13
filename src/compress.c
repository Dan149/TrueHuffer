// std
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
#include "dict.h"


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
  char * key;
  u_int * freq;
};

////////////


FrequencyTable * get_char_freq_table(char * str) {
  FrequencyTable *ft;
  ft = malloc(sizeof(FrequencyTable));
  ft->key = malloc(strlen(str)*sizeof(char));
  ft->freq = calloc(strlen(str), sizeof(u_int));
  // TODO: Handle malloc errors.

  for (int i = 0; i < strlen(str); i++) {
        
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
