#include "debug.h"
#include "compress.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void print_freq_table(FrequencyTable *ft) {
  assert(ft != NULL && ft->keys != NULL && ft->freq != NULL);
  for (long unsigned int i = 0; i < strlen(ft->keys); i++) {
    printf("\nchar: %c | frequency: %d", ft->keys[i], ft->freq[i]);
  }
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
  // Internal nodes have a '\0' key. Handle newline characters cleanly for
  // debugging.
  if (n->key == '\0') {
    printf("[00:%u]\n", n->freq);
  } else if (n->key == '\n') {
    printf("['\\n':%u]\n", n->freq);
  } else {
    printf("['%c':%u]\n", n->key, n->freq);
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
