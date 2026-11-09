#ifndef COMPRESS_H
#define COMPRESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// TrueHuffer file format (.thman)
// -------------------------------
//   bytes 0..2   magic: 1, 127, 2
//   bytes 3..4   number N of distinct byte values (little endian, 0..256)
//   N entries    1 byte (the value) + 4 bytes (its frequency, little endian)
//   rest         Huffman bit stream, MSB first, last byte zero padded.
// The decoder rebuilds the exact same tree from the table (build_btree is
// deterministic) and stops after sum(frequencies) symbols.

#define THMAN_EXTENSION ".thman"

// Structs //

// Contents are raw bytes (any file works, not only text). Buffers are always
// NUL terminated for convenience, the *Size fields hold the real length.
typedef struct TextObject {
  char *deflatedContent;
  size_t deflatedSize;
  char *inflatedContent;
  size_t inflatedSize;
} TextObject;

typedef struct Node {
  unsigned char key; // byte value (only meaningful for leaves).
  uint64_t freq;     // either frequency of assigned byte or sum of both
                     // children below.
  struct Node *child_a; // first children node (bit 0).
  struct Node *child_b; // second (bit 1). Leaves have both set to NULL.
} Node;

typedef struct BinaryTree {
  Node *root_node;    // The node that holds them all.
  TextObject *parent; // Who's the father ? (allows access to the parent
                      // TextObject from which the BinaryTree is derived from)
} BinaryTree;

typedef struct FrequencyTable {
  unsigned char *keys;
  uint32_t *freq;
  size_t size; // number of distinct keys.
} FrequencyTable;

// True if buf starts with the TrueHuffer magic bytes.
bool isFileDeflated(const char *buf, size_t len);

// TextObject implementation:

// Reads a whole file. It ends up in inflatedContent, or in deflatedContent if
// it starts with the TrueHuffer magic bytes. Returns NULL on error.
TextObject *init_text_obj(const char *filepath);
void free_text_obj(TextObject *to);

// Writes whichever content the object currently holds (exactly one of
// deflatedContent / inflatedContent must be set).
int writeTextObjToFile(const TextObject *to, const char *filepath);

// Suggested output name: "a.txt" -> "a.txt.thman" (deflating),
// "a.txt.thman" -> "a.txt" (inflating, anything else -> "<name>.out").
// Returned string must be free()'d.
char *make_output_path(const char *input, bool deflating);

//
BinaryTree *build_btree(FrequencyTable *ft);
void free_btree(BinaryTree *bt);

// FrequencyTable:

FrequencyTable *get_freq_table(const char *data, size_t len);
void free_freq_t(FrequencyTable *ft);
void freq_t_merge_sort(
    FrequencyTable *ft); // sort the FrequencyTable from least occuring char to
                         // most, using merge sort algorithm.

//
int inflate(TextObject *obj); // Make it big.
int deflate(TextObject *obj); // Make it smol.

// Human readable description of the last error (set by any function above
// that returned NULL or EXIT_FAILURE).
const char *compress_last_error(void);

#endif // COMPRESS_H
