// std
#include <assert.h>
#include <iso646.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

FrequencyTable *get_freq_table(char *s) {
  FrequencyTable *ft;
  ft = malloc(sizeof(FrequencyTable));
  ft->keys = malloc(1);
  ft->keys[0] = '\0';
  ft->freq = malloc(sizeof(u_int));
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

      char *k_tmp = realloc(ft->keys, cur_len + 2);
      if (!k_tmp) {
        free_freq_t(ft);
        return NULL;
      }
      ft->keys = k_tmp;
      ft->keys[cur_len] = s[i];
      ft->keys[cur_len + 1] = '\0';
      u_int *f_tmp;
      f_tmp = realloc(ft->freq, sizeof(u_int) * (cur_len + 1));
      if (!f_tmp) {
        free_freq_t(ft);
        return NULL;
      }
      ft->freq = f_tmp;
      ft->freq[cur_len] = 1;
    }
  }
  freq_t_merge_sort(ft);
  return ft;
}

void merge_sort_recursive(FrequencyTable *ft, u_int left, u_int right,
                          u_int *temp_freq, char *temp_keys) {
  if (left >= right)
    return;

  u_int mid = left + (right - left) / 2;

  merge_sort_recursive(ft, left, mid, temp_freq, temp_keys);
  merge_sort_recursive(ft, mid + 1, right, temp_freq, temp_keys);

  for (u_int i = left; i <= right; i++) {
    temp_freq[i] = ft->freq[i];
    temp_keys[i] = ft->keys[i];
  }

  u_int i = left;    // left id
  u_int j = mid + 1; // right id
  u_int k = left;    // main array id

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
  assert(ft != NULL && ft->keys != NULL && ft->freq != NULL);
  size_t len = strlen(ft->keys);
  if (len <= 1)
    return;

  u_int *temp_freq = malloc(len * sizeof(u_int));
  char *temp_keys = malloc(len); // len * byte (8bits) | sizeof(char) == byte

  if (!temp_freq || !temp_keys) {
    free(temp_keys);
    free(temp_freq);
    return;
  }

  merge_sort_recursive(ft, 0, len - 1, temp_freq, temp_keys);
  free(temp_keys);
  free(temp_freq);
}

static char *freq_table_to_bytes(FrequencyTable *ft) {
  assert(ft->freq && ft->keys);
  char *buf;
  size_t i = 0;
  size_t j = 0;
  size_t buf_size = strlen(ft->keys) * 5;
  buf = malloc(buf_size);
  while (j < strlen(ft->keys)) {
    buf[i] = ft->keys[j];
    i++;

    if (ft->freq[j] <= 9) {
      buf[i] = '0' + ft->freq[j];
      i++;
    } else {
      int freq_length = snprintf(NULL, 0, "%u", ft->freq[j]);

      if (freq_length < 1) {
        perror("Failed to format freq_table value.");
        free(buf);
        return NULL;
      }

      char *nb_str = malloc(freq_length + 1); // don't forget '\0' ;)
      snprintf(nb_str, freq_length + 1, "%u", ft->freq[j]);

      char *rb = realloc(buf, buf_size + freq_length);
      if (!rb) {
        perror("Failed to reallocate buffer.");
        free(buf);
        free(nb_str);
        return NULL;
      }
      buf = rb;
      buf_size += freq_length;

      for (int x = 0; x < freq_length; x++) {
        buf[i] = nb_str[x];
        i++;
      }
      free(nb_str);
    }
    buf[i] = '\n';
    buf[i + 1] = 25;
    buf[i + 2] = '\n';

    i += 3;
    j++;
  }
  return buf;
}

static FrequencyTable *parse_freq_table(char *str) {
  // builds FrequencyTable struct from a string storing a FrequencyTable.
  assert(str);
  FrequencyTable *ft = malloc(sizeof(FrequencyTable));
  ft->keys = malloc(256);
  ft->freq = malloc(256 * sizeof(u_int));
  u_int ft_index = 0;
  for (size_t x = 0; x < strlen(str); x++) {
    ft->keys[ft_index] = str[x];

    char *freq_str;
    int fs_length = 0;
    size_t y = x;
    while (str[y] != '\n' && str[y + 1] != 25 && str[y + 2] != '\n') {
      fs_length++;
      y++;
    }

    freq_str = malloc(fs_length + 1);
    y = x;
    while (str[y] != '\n' && str[y + 1] != 25 && str[y + 2] != '\n' &&
           y + 2 < strlen(str)) {
      freq_str[y] = str[y];
      y++;
    }
    freq_str[y] = '\0';
    if (sscanf(freq_str, "%u", &ft->freq[ft_index]) != 1) {
      perror("Failed to parse frequency value from string.");
      free_freq_t(ft);
      free(freq_str);
      return NULL;
    }
    x = y - x + 2; // here str[x] would point to the last '\n' of the separator
                   // sequence. bytes(\n 25 \n)
    assert(str[x] == '\n');
    ft_index++;
  }

  return ft;
}

BinaryTree *build_btree(FrequencyTable *ft) {
  // See Bottom up algorithm for logic.
  BinaryTree *bt = malloc(sizeof(BinaryTree));
  if (!bt || !ft || !ft->keys)
    return NULL;

  size_t num_nodes = strlen(ft->keys);

  if (num_nodes == 0) {
    bt->root_node = NULL;
    return bt;
  }

  // Allocate an array of Node pointers to serve as our working forest
  Node **nodes = malloc(num_nodes * sizeof(Node *));
  if (!nodes) {
    free(bt);
    return NULL;
  }

  // Initialize individual leaf nodes using the FrequencyTable data
  for (size_t i = 0; i < num_nodes; i++) {
    nodes[i] = malloc(sizeof(Node));
    nodes[i]->key = ft->keys[i];
    nodes[i]->freq = ft->freq[i];
    nodes[i]->child_a = NULL;
    nodes[i]->child_b = NULL;
  }

  // Execute the bottom-up algorithm: loop until only the root node remains
  while (num_nodes > 1) {
    // Re-sort the active array in descending order so the lowest frequencies
    // are at the end A simple bubble sort suffices since num_nodes is small
    // (max 256 for characters)
    for (size_t i = 0; i < num_nodes - 1; i++) {
      for (size_t j = 0; j < num_nodes - i - 1; j++) {
        if (nodes[j]->freq < nodes[j + 1]->freq) {
          Node *temp = nodes[j];
          nodes[j] = nodes[j + 1];
          nodes[j + 1] = temp;
        }
      }
    }

    // Extract the two lowest-frequency nodes
    Node *left = nodes[num_nodes - 1];
    Node *right = nodes[num_nodes - 2];

    // Create a new parent node and assign the sub-trees
    Node *parent = malloc(sizeof(Node));
    parent->freq = left->freq + right->freq;
    parent->key = '\0'; // Internal node marker
    parent->child_a = left;
    parent->child_b = right;

    // Replace the right slot with the new parent and shrink the active pool
    nodes[num_nodes - 2] = parent;
    num_nodes--;
  }

  bt->root_node = nodes[0];
  free(nodes);

  return bt;
}

bool endsWith(const char *str, const char *suffix) {
  if (!str || !suffix)
    return false;

  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);

  if (lensuffix > lenstr)
    return false;

  return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

TextObject *init_text_obj(char *filepath) {
  // INFO: TrueHuffer's deflated files will have the file type: <.thman>.
  // Moreover, TrueHuffer deflated files start with a header, between the ASCII
  // chars for header_start and file_start See: www.ascii-code.com
  // Example:
  // 1 (header_start char) 127... (HEADER) 2 (file_start char)
  // ---
  // Header composition:
  // first header char will always be N°127.
  // Nothing else for now.
  assert(filepath);
  TextObject *to = malloc(sizeof(TextObject));
  char *buf;

  // Reading file //
  FILE *fptr = fopen(filepath, "rb");
  if (!fptr) {
    perror("Error: file not found.");
    return NULL;
  }

  // Get file size
  fseek(fptr, 0, SEEK_END);     // moving cursor to EOF.
  long file_size = ftell(fptr); // getting byte amount.
  fseek(fptr, 0, SEEK_SET);     // moving back to top.

  buf = (char *)malloc(file_size + 1);
  if (!buf) {
    fclose(fptr);
    return NULL;
  }

  size_t bytes_read = fread(buf, 1, file_size, fptr);
  buf[bytes_read] = '\0';
  fclose(fptr);
  // End of file reading //

  // Is file inflated or deflated ?

  if (endsWith(filepath, ".thman"))
    to->deflatedContent = buf;
  else
    to->inflatedContent = buf;

  return to;
}

int deflate(TextObject *to) {
  if (to->inflatedContent == NULL)
    return EXIT_FAILURE;

  FrequencyTable *ft = get_freq_table(to->inflatedContent);
  if (!ft)
    return EXIT_FAILURE;

  to->deflatedContent = freq_table_to_bytes(ft);
  free_freq_t(ft);
  free(to->inflatedContent);
  to->inflatedContent = NULL;

  return EXIT_SUCCESS;
}

int inflate(TextObject *to) {
  if (to->deflatedContent == NULL)
    return EXIT_FAILURE;

  FrequencyTable *ft = parse_freq_table(to->deflatedContent);
  if (!ft)
    return EXIT_FAILURE;
  to->inflatedContent = freq_table_to_bytes(ft);
  free_freq_t(ft);
  free(to->deflatedContent);
  to->deflatedContent = NULL;

  return EXIT_SUCCESS;
}

// int writeTextObjToFile(TextObject *to, const char *filepath) {
//   if (to->deflatedContent == NULL && to->inflatedContent == NULL)
//     return EXIT_FAILURE;
//   const char HEAD[3] = {1, 127, 2};
//   FILE *fp;
//   fp = fopen(filepath, "wb");
//   if (!fp) {
//     perror("Failed to open file in wb mode.");
//     return EXIT_FAILURE;
//   }
//   if (to->deflatedContent == NULL) {
//     // inflatedContent write.
//   } else if (to->inflatedContent == NULL) {
//     // deflatedContent write.
//     size_t len = strlen(to->deflatedContent);
//     fwrite(HEAD, sizeof(char), 3, fp);
//     size_t written = fwrite(to->deflatedContent, sizeof(char), len, fp);
//   } else
//     return EXIT_FAILURE; // Both contents are not null.
//   return EXIT_SUCCESS;
// }

bool isFileDeflated(char *buf) {
  return buf[0] == 1 && buf[1] == 127 && buf[2] == 2;
}

// free functions:

void free_freq_t(FrequencyTable *ft) {
  if (!ft)
    return;
  free(ft->freq);
  free(ft->keys);
  free(ft);
}

void free_text_obj(TextObject *to) {
  if (!to)
    return;
  free(to->inflatedContent);
  free(to->deflatedContent);
  free(to);
}

static void free_node_recursive(Node *n) {
  if (!n)
    return;
  if (n->child_a)
    free_node_recursive(n->child_a);
  if (n->child_b)
    free_node_recursive(n->child_b);

  free(n);
}

void free_btree(BinaryTree *bt) {
  if (!bt)
    return;

  free_node_recursive(bt->root_node);
  free(bt);
}
