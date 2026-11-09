#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
#include "../src/compress.h"
#include "../src/debug.h"

static int failures = 0;

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "\n  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);      \
      failures++;                                                              \
    }                                                                          \
  } while (0)

static TextObject *make_text_obj(const unsigned char *data, size_t len) {
  TextObject *to = calloc(1, sizeof(TextObject));
  if (!to)
    return NULL;
  to->inflatedContent = malloc(len + 1);
  if (!to->inflatedContent) {
    free(to);
    return NULL;
  }
  if (len > 0)
    memcpy(to->inflatedContent, data, len);
  to->inflatedContent[len] = '\0';
  to->inflatedSize = len;
  return to;
}

// deflate -> inflate in memory, checks we get the same bytes back.
static size_t check_roundtrip(const char *name, const unsigned char *data,
                              size_t len) {
  printf("[roundtrip] %-26s", name);
  TextObject *to = make_text_obj(data, len);
  CHECK(to != NULL, "allocation");
  if (!to)
    return 0;

  size_t deflated = 0;
  int rc = deflate(to);
  CHECK(rc == EXIT_SUCCESS, "deflate succeeds");
  if (rc == EXIT_SUCCESS) {
    CHECK(to->inflatedContent == NULL, "inflated content released by deflate");
    CHECK(isFileDeflated(to->deflatedContent, to->deflatedSize),
          "deflated content starts with the header");
    deflated = to->deflatedSize;

    rc = inflate(to);
    CHECK(rc == EXIT_SUCCESS, "inflate succeeds");
    if (rc == EXIT_SUCCESS) {
      CHECK(to->deflatedContent == NULL,
            "deflated content released by inflate");
      CHECK(to->inflatedSize == len, "same size after round trip");
      CHECK(len == 0 || memcmp(to->inflatedContent, data, len) == 0,
            "same bytes after round trip");
    }
  }
  printf(" %zu -> %zu bytes\n", len, deflated);
  free_text_obj(to);
  return deflated;
}

static void test_memory(void) {
  // empty input
  check_roundtrip("empty", (const unsigned char *)"", 0);

  // one single symbol (tree with a single leaf)
  unsigned char same[1000];
  memset(same, 'a', sizeof(same));
  check_roundtrip("single symbol", same, sizeof(same));
  check_roundtrip("one byte", (const unsigned char *)"x", 1);

  // text
  const char *text = "abracadabra";
  check_roundtrip("abracadabra", (const unsigned char *)text, strlen(text));

  // every byte value, including NUL
  unsigned char all[256 * 3];
  for (size_t i = 0; i < sizeof(all); i++)
    all[i] = (unsigned char)(i % 256);
  check_roundtrip("all 256 byte values", all, sizeof(all));

  // skewed pseudo random binary data: must actually get smaller
  size_t big_len = 200000;
  unsigned char *big = malloc(big_len);
  CHECK(big != NULL, "allocation");
  if (big) {
    uint32_t x = 12345;
    for (size_t i = 0; i < big_len; i++) {
      x = x * 1103515245u + 12345u;
      big[i] = (unsigned char)(((x >> 16) & (x >> 23)) & 0xFFu);
    }
    size_t d = check_roundtrip("skewed binary 200 kB", big, big_len);
    CHECK(d < big_len, "skewed data gets smaller");
    free(big);
  }

  // Fibonacci frequencies: the deepest possible tree for the alphabet size.
  unsigned char fib[40000];
  size_t flen = 0;
  size_t a = 1, b = 1;
  for (int sym = 0; sym < 20 && flen < sizeof(fib); sym++) {
    for (size_t k = 0; k < a && flen < sizeof(fib); k++)
      fib[flen++] = (unsigned char)('a' + sym);
    size_t next = a + b;
    a = b;
    b = next;
  }
  check_roundtrip("fibonacci (deep tree)", fib, flen);
}

static void test_files(void) {
  const char *src = "./tests/lorem.txt";
  const char *tmp_def = "./tests/lorem.tmp.thman";
  const char *tmp_out = "./tests/lorem.tmp.txt";

  printf("[files] %s\n", src);
  TextObject *orig = init_text_obj(src);
  CHECK(orig != NULL, "lorem.txt can be loaded");
  if (!orig) {
    fprintf(stderr, "  %s\n", compress_last_error());
    return;
  }
  CHECK(orig->inflatedContent && !orig->deflatedContent,
        "plain file is loaded as inflated");

  size_t orig_size = orig->inflatedSize;
  char *copy = malloc(orig_size + 1);
  CHECK(copy != NULL, "allocation");
  if (!copy) {
    free_text_obj(orig);
    return;
  }
  memcpy(copy, orig->inflatedContent, orig_size + 1);

  CHECK(deflate(orig) == EXIT_SUCCESS, "deflate lorem.txt");
  CHECK(orig->deflatedSize < orig_size, "lorem.txt gets smaller");
  printf("  %zu -> %zu bytes\n", orig_size, orig->deflatedSize);
  CHECK(writeTextObjToFile(orig, tmp_def) == EXIT_SUCCESS, "write .thman");
  free_text_obj(orig);

  TextObject *packed = init_text_obj(tmp_def);
  CHECK(packed != NULL, "reload the .thman file");
  if (packed) {
    CHECK(packed->deflatedContent && !packed->inflatedContent,
          ".thman file is loaded as deflated");
    CHECK(inflate(packed) == EXIT_SUCCESS, "inflate the reloaded file");
    CHECK(writeTextObjToFile(packed, tmp_out) == EXIT_SUCCESS,
          "write inflated file");
    free_text_obj(packed);
  }

  TextObject *back = init_text_obj(tmp_out);
  CHECK(back != NULL, "reload the inflated file");
  if (back) {
    CHECK(back->inflatedSize == orig_size &&
              memcmp(back->inflatedContent, copy, orig_size) == 0,
          "file content identical after the full round trip");
    free_text_obj(back);
  }

  remove(tmp_def);
  remove(tmp_out);
  free(copy);

  CHECK(init_text_obj("./tests/does-not-exist.txt") == NULL,
        "missing file is reported");
}

static void test_corrupted(void) {
  printf("[corrupted input]\n");

  // not a TrueHuffer file at all
  TextObject *to = calloc(1, sizeof(TextObject));
  CHECK(to != NULL, "allocation");
  if (!to)
    return;
  const char *junk = "this is not a thman file";
  to->deflatedContent = malloc(strlen(junk) + 1);
  if (to->deflatedContent) {
    memcpy(to->deflatedContent, junk, strlen(junk) + 1);
    to->deflatedSize = strlen(junk);
    CHECK(inflate(to) == EXIT_FAILURE, "bad magic is rejected");
  }
  free_text_obj(to);

  // valid file cut at two different places
  const char *text = "abracadabra abracadabra abracadabra abracadabra";
  for (int cut = 0; cut < 2; cut++) {
    to = make_text_obj((const unsigned char *)text, strlen(text));
    CHECK(to && deflate(to) == EXIT_SUCCESS, "deflate sample");
    if (to && to->deflatedContent) {
      size_t full = to->deflatedSize;
      to->deflatedSize = cut == 0 ? 4 : full - 8;
      CHECK(inflate(to) == EXIT_FAILURE, cut == 0
                                             ? "truncated header is rejected"
                                             : "truncated data is rejected");
      CHECK(to->deflatedContent != NULL, "failed inflate keeps the input");
    }
    free_text_obj(to);
  }
}

static void test_paths(void) {
  printf("[output paths]\n");
  char *p = make_output_path("a.txt", true);
  CHECK(p && strcmp(p, "a.txt.thman") == 0, "deflate adds .thman");
  free(p);
  p = make_output_path("a.txt.thman", false);
  CHECK(p && strcmp(p, "a.txt") == 0, "inflate strips .thman");
  free(p);
  p = make_output_path("a.bin", false);
  CHECK(p && strcmp(p, "a.bin.out") == 0, "inflate of other names adds .out");
  free(p);
}

static void demo_debug(void) {
  printf("[debug helpers]\n");
  const char *s = "abracadabra";
  FrequencyTable *ft = get_freq_table(s, strlen(s));
  BinaryTree *bt = build_btree(ft);
  CHECK(ft && bt, "table and tree built");
  print_freq_table(ft);
  print_btree(bt);
  free_btree(bt);
  free_freq_t(ft);
}

int main(void) {
  test_memory();
  test_files();
  test_corrupted();
  test_paths();
  demo_debug();

  if (failures) {
    printf("\n%d check(s) FAILED\n", failures);
    return EXIT_FAILURE;
  }
  printf("\nAll tests passed.\n");
  return EXIT_SUCCESS;
}
