#include "debug.h"
#include "compress.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>

void print_freq_table(FrequencyTable *ft) {
  if (ft == NULL || (ft->size > 0 && (ft->keys == NULL || ft->freq == NULL))) {
    printf("FrequencyTable pointer is NULL or invalid.\n");
    return;
  }
  for (size_t i = 0; i < ft->size; i++) {
    if (isprint(ft->keys[i]))
      printf("\nchar: %c | frequency: %" PRIu32, ft->keys[i], ft->freq[i]);
    else
      printf("\nchar: 0x%02X | frequency: %" PRIu32, (unsigned)ft->keys[i],
             ft->freq[i]);
  }
  printf("\n");
}

void print_node(Node *n, int level) {
  if (n == NULL)
    return;
  // Traverse the right child (child_b) first to print the tree sideways
  print_node(n->child_b, level + 1);

  // Print indentation based on the current depth level
  for (int i = 0; i < level; i++) {
    printf("   -   ");
  }

  // Print the node data
  // Internal nodes are the ones with children. Handle non printable
  // characters cleanly for debugging.
  if (n->child_a != NULL || n->child_b != NULL) {
    printf("[00:%" PRIu64 "]\n", n->freq);
  } else if (n->key == '\n') {
    printf("['\\n':%" PRIu64 "]\n", n->freq);
  } else if (isprint(n->key)) {
    printf("['%c':%" PRIu64 "]\n", n->key, n->freq);
  } else {
    printf("[0x%02X:%" PRIu64 "]\n", (unsigned)n->key, n->freq);
  }

  // Traverse the left child (child_a)
  print_node(n->child_a, level + 1);
}

void print_btree(BinaryTree *bt) {
  if (bt == NULL) {
    printf("BinaryTree pointer is NULL.\n");
    return;
  }

  if (bt->root_node == NULL) {
    printf("Tree is empty (root_node is NULL).\n");
    return;
  }

  printf("\n--- Huffman Tree (Sideways) ---\n\n");
  print_node(bt->root_node, 0);
  printf("\n-------------------------------\n");
}
