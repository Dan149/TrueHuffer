// std
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// src
#include "compress.h"

#define HEADER_MAGIC_LEN 3
#define HEADER_COUNT_LEN 2
#define HEADER_ENTRY_LEN 5 // 1 byte key + 4 bytes frequency
#define MAX_SYMBOLS 256
#define MAX_CODE_LEN 64 // codes are stored in a uint64_t

static const unsigned char HEADER_MAGIC[HEADER_MAGIC_LEN] = {1, 127, 2};

////////////
// Errors //
////////////

static char last_error[256] = "";

#if defined(__GNUC__)
__attribute__((format(printf, 1, 2)))
#endif
static void set_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(last_error, sizeof(last_error), fmt, ap);
  va_end(ap);
}

const char *compress_last_error(void) {
  return last_error[0] ? last_error : "unknown error";
}

// Helpers:
static void put_u32_le(unsigned char *p, uint32_t v) {
  p[0] = (unsigned char)(v & 0xFFu);
  p[1] = (unsigned char)((v >> 8) & 0xFFu);
  p[2] = (unsigned char)((v >> 16) & 0xFFu);
  p[3] = (unsigned char)((v >> 24) & 0xFFu);
}

static uint32_t get_u32_le(const unsigned char *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static bool endsWith(const char *str, const char *suffix) {
  if (!str || !suffix)
    return false;

  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);

  if (lensuffix > lenstr)
    return false;

  return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

// FrequencyTable

static void merge_sort_recursive(FrequencyTable *ft, size_t left, size_t right,
                                 uint32_t *temp_freq,
                                 unsigned char *temp_keys) {
  if (left >= right)
    return;

  size_t mid = left + (right - left) / 2;

  merge_sort_recursive(ft, left, mid, temp_freq, temp_keys);
  merge_sort_recursive(ft, mid + 1, right, temp_freq, temp_keys);

  for (size_t x = left; x <= right; x++) {
    temp_freq[x] = ft->freq[x];
    temp_keys[x] = ft->keys[x];
  }

  size_t i = left;    // left id
  size_t j = mid + 1; // right id
  size_t k = left;    // main array id

  while (i <= mid && j <= right) {
    // invert operators for DESC order
    if (temp_freq[i] <= temp_freq[j]) {
      ft->freq[k] = temp_freq[i];
      ft->keys[k] = temp_keys[i];
      i++;
    } else {
      ft->freq[k] = temp_freq[j];
      ft->keys[k] = temp_keys[j];
      j++;
    }
    k++;
  }

  // Whatever is left of the right half is already at its final place.
  while (i <= mid) {
    ft->freq[k] = temp_freq[i];
    ft->keys[k] = temp_keys[i];
    i++;
    k++;
  }
}

void freq_t_merge_sort(
    FrequencyTable *ft) // sort the FrequencyTable from least occuring char to
                        // most, using merge sort algorithm.
{
  if (!ft || !ft->keys || !ft->freq || ft->size <= 1)
    return;

  uint32_t *temp_freq = malloc(ft->size * sizeof(uint32_t));
  unsigned char *temp_keys = malloc(ft->size);

  if (!temp_freq || !temp_keys) {
    free(temp_keys);
    free(temp_freq);
    return; // table stays unsorted, which is still a valid table.
  }

  merge_sort_recursive(ft, 0, ft->size - 1, temp_freq, temp_keys);
  free(temp_keys);
  free(temp_freq);
}

FrequencyTable *get_freq_table(const char *data, size_t len) {
  if ((!data && len > 0) || (uint64_t)len > UINT32_MAX) {
    set_error("input is too large (limit is 4 GiB)");
    return NULL;
  }

  uint32_t counts[MAX_SYMBOLS] = {0};
  const unsigned char *p = (const unsigned char *)data;
  for (size_t i = 0; i < len; i++)
    counts[p[i]]++;

  size_t distinct = 0;
  for (size_t c = 0; c < MAX_SYMBOLS; c++)
    if (counts[c] > 0)
      distinct++;

  FrequencyTable *ft = calloc(1, sizeof(FrequencyTable));
  if (!ft) {
    set_error("out of memory");
    return NULL;
  }
  size_t alloc = distinct ? distinct : 1;
  ft->keys = malloc(alloc);
  ft->freq = malloc(alloc * sizeof(uint32_t));
  if (!ft->keys || !ft->freq) {
    free_freq_t(ft);
    set_error("out of memory");
    return NULL;
  }

  for (size_t c = 0; c < MAX_SYMBOLS; c++) {
    if (counts[c] > 0) {
      ft->keys[ft->size] = (unsigned char)c;
      ft->freq[ft->size] = counts[c];
      ft->size++;
    }
  }

  freq_t_merge_sort(ft);
  return ft;
}

void free_freq_t(FrequencyTable *ft) {
  if (!ft)
    return;
  free(ft->freq);
  free(ft->keys);
  free(ft);
}

/////////////////
// Binary tree //
/////////////////

static void free_node_recursive(Node *n) {
  if (!n)
    return;
  free_node_recursive(n->child_a);
  free_node_recursive(n->child_b);
  free(n);
}

void free_btree(BinaryTree *bt) {
  if (!bt)
    return;

  free_node_recursive(bt->root_node);
  free(bt);
}

static Node *new_node(unsigned char key, uint64_t freq, Node *a, Node *b) {
  Node *n = malloc(sizeof(Node));
  if (!n)
    return NULL;
  n->key = key;
  n->freq = freq;
  n->child_a = a;
  n->child_b = b;
  return n;
}

static bool is_leaf(const Node *n) {
  return n->child_a == NULL && n->child_b == NULL;
}

BinaryTree *build_btree(FrequencyTable *ft) {
  // See Bottom up algorithm for logic.
  if (!ft || (ft->size > 0 && (!ft->keys || !ft->freq))) {
    set_error("invalid frequency table");
    return NULL;
  }

  BinaryTree *bt = calloc(1, sizeof(BinaryTree));
  if (!bt) {
    set_error("out of memory");
    return NULL;
  }

  size_t num_nodes = ft->size;

  if (num_nodes == 0) {
    bt->root_node = NULL;
    return bt;
  }

  // Working forest: array of Node pointers (NULL until created).
  Node **nodes = calloc(num_nodes, sizeof(Node *));
  if (!nodes) {
    free(bt);
    set_error("out of memory");
    return NULL;
  }

  // Initialize individual leaf nodes using the FrequencyTable data
  for (size_t i = 0; i < num_nodes; i++) {
    nodes[i] = new_node(ft->keys[i], ft->freq[i], NULL, NULL);
    if (!nodes[i])
      goto fail;
  }

  // Sort the pool in descending order (stable insertion sort, 256 entries at
  // most) so the lowest frequencies are at the end.
  for (size_t i = 1; i < num_nodes; i++) {
    Node *cur = nodes[i];
    size_t j = i;
    while (j > 0 && nodes[j - 1]->freq < cur->freq) {
      nodes[j] = nodes[j - 1];
      j--;
    }
    nodes[j] = cur;
  }

  // Execute the bottom-up algorithm: loop until only the root node remains
  while (num_nodes > 1) {
    // Extract the two lowest-frequency nodes
    Node *left = nodes[num_nodes - 1];
    Node *right = nodes[num_nodes - 2];

    // Create a new parent node and assign the sub-trees
    // (key 0 is just a placeholder: internal nodes are the ones with children)
    Node *parent = new_node(0, left->freq + right->freq, left, right);
    if (!parent)
      goto fail;

    // Remove both children from the pool and insert the parent so that the
    // pool stays sorted in descending order.
    num_nodes -= 2;
    size_t pos = num_nodes;
    while (pos > 0 && nodes[pos - 1]->freq < parent->freq) {
      nodes[pos] = nodes[pos - 1];
      pos--;
    }
    nodes[pos] = parent;
    num_nodes++;
  }

  bt->root_node = nodes[0];
  free(nodes);

  return bt;

fail:
  for (size_t i = 0; i < num_nodes; i++)
    free_node_recursive(nodes[i]);
  free(nodes);
  free(bt);
  set_error("out of memory");
  return NULL;
}

///////////
// Codes //
///////////

typedef struct HuffCode {
  uint64_t bits;
  unsigned len;
} HuffCode;

// Walks the tree and stores the code (path from the root, child_a = 0,
// child_b = 1) of every leaf. Returns false if a code would not fit.
static bool fill_codes(const Node *n, uint64_t bits, unsigned depth,
                       HuffCode *codes) {
  if (is_leaf(n)) {
    codes[n->key].bits = bits;
    // A tree made of a single leaf still needs 1 bit per symbol.
    codes[n->key].len = depth ? depth : 1;
    return true;
  }
  if (depth >= MAX_CODE_LEN)
    return false;
  return fill_codes(n->child_a, bits << 1, depth + 1, codes) &&
         fill_codes(n->child_b, (bits << 1) | 1u, depth + 1, codes);
}

static void put_code(unsigned char *payload, uint64_t *bitpos, HuffCode c) {
  for (unsigned i = c.len; i-- > 0;) {
    if ((c.bits >> i) & 1u)
      payload[*bitpos >> 3] |= (unsigned char)(0x80u >> (*bitpos & 7u));
    (*bitpos)++;
  }
}

/////////////////////////
// deflate and inflate //
/////////////////////////

int deflate(TextObject *to) {
  if (!to || !to->inflatedContent) {
    set_error("nothing to deflate: no inflated content loaded");
    return EXIT_FAILURE;
  }

  int rc = EXIT_FAILURE;
  FrequencyTable *ft = NULL;
  BinaryTree *bt = NULL;
  unsigned char *out = NULL;
  HuffCode codes[MAX_SYMBOLS];
  memset(codes, 0, sizeof(codes));

  uint64_t total_bits = 0;
  uint64_t out_len64 = 0;
  size_t out_len = 0;
  size_t header_len = 0;
  uint64_t bitpos = 0;
  unsigned char *payload = NULL;
  const unsigned char *src = (const unsigned char *)to->inflatedContent;

  ft = get_freq_table(to->inflatedContent, to->inflatedSize);
  if (!ft)
    goto done;

  bt = build_btree(ft);
  if (!bt)
    goto done;

  if (bt->root_node && !fill_codes(bt->root_node, 0, 0, codes)) {
    set_error("internal error: Huffman code longer than %d bits", MAX_CODE_LEN);
    goto done;
  }

  // Compute the output size.
  for (size_t i = 0; i < ft->size; i++)
    total_bits += (uint64_t)ft->freq[i] * codes[ft->keys[i]].len;

  header_len =
      HEADER_MAGIC_LEN + HEADER_COUNT_LEN + ft->size * HEADER_ENTRY_LEN;
  out_len64 = (uint64_t)header_len + (total_bits + 7) / 8;
  out_len = (size_t)out_len64;
  if ((uint64_t)out_len != out_len64) {
    set_error("input is too large for this platform");
    goto done;
  }

  out = calloc(out_len + 1, 1); // zeroed: bits are OR-ed in. +1 for the '\0'.
  if (!out) {
    set_error("out of memory");
    goto done;
  }

  // Header: magic, number of symbols, (key, frequency) table.
  memcpy(out, HEADER_MAGIC, HEADER_MAGIC_LEN);
  out[3] = (unsigned char)(ft->size & 0xFFu);
  out[4] = (unsigned char)((ft->size >> 8) & 0xFFu);
  for (size_t i = 0; i < ft->size; i++) {
    unsigned char *entry =
        out + HEADER_MAGIC_LEN + HEADER_COUNT_LEN + i * HEADER_ENTRY_LEN;
    entry[0] = ft->keys[i];
    put_u32_le(entry + 1, ft->freq[i]);
  }

  // Bit stream.
  payload = out + header_len;
  for (size_t i = 0; i < to->inflatedSize; i++)
    put_code(payload, &bitpos, codes[src[i]]);

  // Swap contents.
  free(to->deflatedContent);
  to->deflatedContent = (char *)out;
  to->deflatedSize = out_len;
  out = NULL;
  free(to->inflatedContent);
  to->inflatedContent = NULL;
  to->inflatedSize = 0;
  rc = EXIT_SUCCESS;

done:
  free(out);
  free_btree(bt);
  free_freq_t(ft);
  return rc;
}

int inflate(TextObject *to) {
  if (!to || !to->deflatedContent) {
    set_error("nothing to inflate: no deflated content loaded");
    return EXIT_FAILURE;
  }

  const unsigned char *in = (const unsigned char *)to->deflatedContent;
  size_t len = to->deflatedSize;

  if (!isFileDeflated(to->deflatedContent, len)) {
    set_error("not a TrueHuffer file (header not found)");
    return EXIT_FAILURE;
  }
  if (len < HEADER_MAGIC_LEN + HEADER_COUNT_LEN) {
    set_error("corrupted file: truncated header");
    return EXIT_FAILURE;
  }

  int rc = EXIT_FAILURE;
  FrequencyTable *ft = NULL;
  BinaryTree *bt = NULL;
  unsigned char *out = NULL;
  bool seen[MAX_SYMBOLS] = {false};
  uint64_t total = 0;

  size_t count = (size_t)in[3] | ((size_t)in[4] << 8);
  if (count > MAX_SYMBOLS) {
    set_error("corrupted file: invalid symbol count");
    return EXIT_FAILURE;
  }
  size_t header_len =
      HEADER_MAGIC_LEN + HEADER_COUNT_LEN + count * HEADER_ENTRY_LEN;
  if (len < header_len) {
    set_error("corrupted file: truncated frequency table");
    return EXIT_FAILURE;
  }

  // Rebuild the frequency table, in the very same order it was written.
  ft = calloc(1, sizeof(FrequencyTable));
  if (!ft) {
    set_error("out of memory");
    return EXIT_FAILURE;
  }
  size_t alloc = count ? count : 1;
  ft->keys = malloc(alloc);
  ft->freq = malloc(alloc * sizeof(uint32_t));
  if (!ft->keys || !ft->freq) {
    set_error("out of memory");
    goto done;
  }

  for (size_t i = 0; i < count; i++) {
    const unsigned char *entry =
        in + HEADER_MAGIC_LEN + HEADER_COUNT_LEN + i * HEADER_ENTRY_LEN;
    uint32_t freq = get_u32_le(entry + 1);
    if (seen[entry[0]] || freq == 0) {
      set_error("corrupted file: invalid frequency table");
      goto done;
    }
    seen[entry[0]] = true;
    ft->keys[i] = entry[0];
    ft->freq[i] = freq;
    ft->size++;
    total += freq;
  }

  // Every symbol costs at least one bit: reject absurd sizes before
  // allocating anything.
  const unsigned char *payload = in + header_len;
  uint64_t payload_bits = (uint64_t)(len - header_len) * 8;
  if (total > UINT32_MAX || total > payload_bits) {
    set_error("corrupted file: data is shorter than its header announces");
    goto done;
  }

  bt = build_btree(ft);
  if (!bt)
    goto done;

  size_t out_len = (size_t)total;
  out = malloc(out_len + 1);
  if (!out) {
    set_error("out of memory");
    goto done;
  }

  // Decode: walk the tree bit by bit.
  const Node *root = bt->root_node;
  uint64_t bitpos = 0;
  for (size_t i = 0; i < out_len; i++) {
    const Node *n = root;
    if (is_leaf(n)) {
      bitpos++; // single symbol file: 1 bit per symbol, value irrelevant.
    } else {
      while (!is_leaf(n)) {
        if (bitpos >= payload_bits) {
          set_error("corrupted file: unexpected end of data");
          goto done;
        }
        unsigned bit =
            (payload[bitpos >> 3] >> (7u - (unsigned)(bitpos & 7u))) & 1u;
        bitpos++;
        n = bit ? n->child_b : n->child_a;
      }
    }
    out[i] = n->key;
  }
  out[out_len] = '\0';

  // Swap contents.
  free(to->inflatedContent);
  to->inflatedContent = (char *)out;
  to->inflatedSize = out_len;
  out = NULL;
  free(to->deflatedContent);
  to->deflatedContent = NULL;
  to->deflatedSize = 0;
  rc = EXIT_SUCCESS;

done:
  free(out);
  free_btree(bt);
  free_freq_t(ft);
  return rc;
}

////////////////
// File input //
////////////////

bool isFileDeflated(const char *buf, size_t len) {
  return buf && len >= HEADER_MAGIC_LEN &&
         memcmp(buf, HEADER_MAGIC, HEADER_MAGIC_LEN) == 0;
}

// Reads a whole file into a freshly allocated, NUL terminated buffer.
static char *read_whole_file(const char *filepath, size_t *out_size) {
  FILE *fptr = fopen(filepath, "rb");
  if (!fptr) {
    set_error("cannot open '%s': %s", filepath, strerror(errno));
    return NULL;
  }

  size_t cap = 65536;
  size_t len = 0;
  char *buf = malloc(cap);
  if (!buf) {
    fclose(fptr);
    set_error("out of memory");
    return NULL;
  }

  for (;;) {
    if (len == cap) {
      if (cap > SIZE_MAX / 2) {
        free(buf);
        fclose(fptr);
        set_error("'%s' is too large", filepath);
        return NULL;
      }
      char *tmp = realloc(buf, cap * 2);
      if (!tmp) {
        free(buf);
        fclose(fptr);
        set_error("out of memory");
        return NULL;
      }
      buf = tmp;
      cap *= 2;
    }
    size_t n = fread(buf + len, 1, cap - len, fptr);
    len += n;
    if (n == 0)
      break;
  }

  if (ferror(fptr)) {
    set_error("cannot read '%s'", filepath);
    free(buf);
    fclose(fptr);
    return NULL;
  }
  fclose(fptr);

  if (len == cap) { // room for the terminator
    char *tmp = realloc(buf, cap + 1);
    if (!tmp) {
      free(buf);
      set_error("out of memory");
      return NULL;
    }
    buf = tmp;
  }
  buf[len] = '\0';
  *out_size = len;
  return buf;
}

TextObject *init_text_obj(const char *filepath) {
  // INFO: TrueHuffer's deflated files will have the file type: <.thman>.
  // Moreover, TrueHuffer deflated files start with a header, see compress.h
  // for the exact layout. A file is considered deflated when it starts with
  // the magic bytes 1, 127, 2 (that is what isFileDeflated() checks).
  if (!filepath) {
    set_error("no file path given");
    return NULL;
  }

  size_t size = 0;
  char *buf = read_whole_file(filepath, &size);
  if (!buf)
    return NULL;

  TextObject *to = calloc(1, sizeof(TextObject));
  if (!to) {
    free(buf);
    set_error("out of memory");
    return NULL;
  }

  if (isFileDeflated(buf, size)) {
    to->deflatedContent = buf;
    to->deflatedSize = size;
  } else {
    to->inflatedContent = buf;
    to->inflatedSize = size;
  }

  return to;
}

/////////////////
// File output //
/////////////////

int writeTextObjToFile(const TextObject *to, const char *filepath) {
  if (!to || !filepath) {
    set_error("invalid arguments");
    return EXIT_FAILURE;
  }

  const char *data;
  size_t size;
  if (to->deflatedContent && !to->inflatedContent) {
    data = to->deflatedContent;
    size = to->deflatedSize;
  } else if (to->inflatedContent && !to->deflatedContent) {
    data = to->inflatedContent;
    size = to->inflatedSize;
  } else {
    set_error("nothing to write (or both contents are set)");
    return EXIT_FAILURE;
  }

  FILE *fp = fopen(filepath, "wb");
  if (!fp) {
    set_error("cannot create '%s': %s", filepath, strerror(errno));
    return EXIT_FAILURE;
  }

  size_t written = size ? fwrite(data, 1, size, fp) : 0;
  if (written != size) {
    set_error("failed to write '%s': %s", filepath, strerror(errno));
    fclose(fp);
    return EXIT_FAILURE;
  }
  if (fclose(fp) != 0) {
    set_error("failed to write '%s': %s", filepath, strerror(errno));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

char *make_output_path(const char *input, bool deflating) {
  if (!input)
    return NULL;

  size_t len = strlen(input);
  size_t ext_len = strlen(THMAN_EXTENSION);
  const char *suffix = "";
  size_t keep = len;

  if (deflating) {
    suffix = THMAN_EXTENSION;
  } else if (len > ext_len && endsWith(input, THMAN_EXTENSION)) {
    keep = len - ext_len;
  } else {
    suffix = ".out";
  }

  size_t suffix_len = strlen(suffix);
  char *path = malloc(keep + suffix_len + 1);
  if (!path)
    return NULL;
  memcpy(path, input, keep);
  memcpy(path + keep, suffix, suffix_len + 1);
  return path;
}

// free functions:

void free_text_obj(TextObject *to) {
  if (!to)
    return;
  free(to->inflatedContent);
  free(to->deflatedContent);
  free(to);
}
