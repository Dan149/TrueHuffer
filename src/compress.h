#ifndef COMPRESS_H
#define COMPRESS_H

#if defined(_WIN32) // Checks OS an import corresponding library to use u_int
#include <winsock.h>
#elif defined(__linux__)
#include <sys/types.h>
#else
return EXIT_FAILURE;
#endif

// Structs //

typedef struct TextObject {
  char *deflatedContent;
  char *inflatedContent;
} TextObject;

typedef struct Node {
  char letter; // single letter value.
  u_int freq;  
  struct Node *child_a; // first children node.
  struct Node *child_b; // second.
} Node;

typedef struct BinaryTree{
  Node       *root_node;         // The node that holds them all.
  TextObject *parent; // Who's the father ? (allows access to the parent TextObject from which the BinaryTree is derived from)
} BinaryTree;

typedef struct FrequencyTable{
  char * keys;
  u_int * freq;
} FrequencyTable;

// TextObject implementation:
TextObject *init_text_obj(char * dc, char * ic); // dc and ic should be optional, NULL if unset. Allows inflate() and deflate equally.
void free_text_obj(TextObject * to);

// BinaryTree & Nodes
int appendNode(Node * node_list, Node new_node); // 0: OK | -1: Err | realloc() the node_list, add new child node.

void free_binary_tree(BinaryTree * bt);

// FrequencyTable:

FrequencyTable * get_char_freq_table(char * str);
void free_freq_t(FrequencyTable * ft);
void freq_t_merge_sort(FrequencyTable * ft); // sort the FrequencyTable from least occuring char to most, using merge sort algorithm.

//
int inflate(TextObject *obj); // Make it big.
int deflate(TextObject *obj); // Make it smol.

#endif //COMPRESS_H
