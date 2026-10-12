// std
#include <stdlib.h>
#include <stdio.h>
#include <assert.h> // YANIS. Learn ts please.
#include <string.h> // YANIS. Also, check docs for this.
// src
#include "compress.h"


// Structs //

typedef struct {
  char *deflatedContent;
  char *inflatedContent;
  char *checksum;
} TextObject;

typedef struct {
  Node       *root;         // The node that holds them all.
  TextObject *parent; // Who's the father ? (allows access to the parent TextObject from which the BinaryTree is derived from)
} BinaryTree;

typedef struct {
  char val; // single letter value.
  Node *children; // children letters.
} Node;

////////////


TextObject *initTextObject(char * dc, char * ic) {
  // assume deflatedContent & inflatedContent either NULL or allocated, non-empty string :
  assert((dc == NULL || strlen(dc) != 0) && (ic == NULL || strlen(ic) != 0)); 

  TextObject *to = malloc(sizeof(TextObject)); // to malloc or not to malloc ? That is the question. (seriously, I'm not sure this is good design, have to check later.)

  to->inflatedContent=ic;
  to->deflatedContent=dc;

  return to
}
