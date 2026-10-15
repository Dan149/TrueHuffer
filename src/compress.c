// std
#include <iso646.h>
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
      if (!k_tmp) {
        free_freq_t(ft);
        return NULL;
      }
      ft->keys = k_tmp; 
      ft->keys[cur_len] = s[i];  
      ft->keys[cur_len+1] = '\0';  
      u_int * f_tmp;
      f_tmp = realloc(ft->freq, sizeof(u_int)*(cur_len+1));  
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

static void merge_sort_recursive(FrequencyTable * ft, u_int left, u_int right, u_int *temp_freq, char *temp_keys) {
	if (left >= right) return;
	
	u_int mid = left + (right-left) / 2;

	merge_sort_recursive(ft, left, mid, temp_freq, temp_keys);
	merge_sort_recursive(ft, mid+1, right, temp_freq, temp_keys);
	
	for (u_int i = left; i <= right; i++) {
		temp_freq[i] = ft->freq[i];
		temp_keys[i] = ft->keys[i];
	}

	u_int i = left; // left id
	u_int j = mid+1;  // right id
	u_int k = left; // main array id
	
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

void freq_t_merge_sort(FrequencyTable * ft) // sort the FrequencyTable from least occuring char to most, using merge sort algorithm.
{
	assert(ft != NULL && ft->keys != NULL && ft->freq != NULL);
	size_t len = strlen(ft->keys);
	if (len <= 1) return;

	u_int *temp_freq = malloc(len * sizeof(u_int));
	char *temp_keys = malloc(len); // len * byte (8bits) | sizeof(char) == byte
	
	if (!temp_freq || !temp_keys) {
	free(temp_keys); free(temp_freq);
	return;
	}

	merge_sort_recursive(ft, 0, len-1, temp_freq, temp_keys);
	free(temp_keys); free(temp_freq);
}

TextObject *init_text_obj(char * dc, char * ic) {
  // assume deflatedContent & inflatedContent either NULL or allocated, non-empty string :
  assert((!dc || strlen(dc) != 0) && (!ic || strlen(ic) != 0)); 

  TextObject *to;
  to = malloc(sizeof(TextObject)); // to malloc or not to malloc ? That is the question. (seriously, I'm not sure this is good design, have to check later.)

  to->inflatedContent=ic;
  to->deflatedContent=dc;

  return to;
}


BinaryTree * build_btree(FrequencyTable * ft) {
  // TODO: Biggest challenge yet.
  // Build Binary Tree with the bottom-up algorithm.
  BinaryTree * bt = malloc(sizeof(BinaryTree));


  return bt;
}


// free functions:

void free_freq_t(FrequencyTable * ft) {
  if (!ft) return;
  free(ft->freq);
  free(ft->keys);
  free(ft);
}

void free_text_obj(TextObject * to) {
	if (!to) return;
	free(to->inflatedContent);
	free(to->deflatedContent);
	free(to);
}

static void free_node_recursive(Node * n) {
	if (!n) return;
	if (n->child_a) free_node_recursive(n->child_a);
	if (n->child_b) free_node_recursive(n->child_b);

	free(n->child_a); free(n->child_b); free(n);
}

void free_btree(BinaryTree * bt) {
	if (!bt) return;

	free_node_recursive(bt->root_node);
	free(bt);

}

