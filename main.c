#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/compress.h"

#ifndef NO_GUI
#include "src/gui.h" //adds gui.h only for gui compilation mode with make.
#endif

#define EXIT_USAGE 2

static void print_usage(FILE *out, const char *prog) {
  fprintf(out,
          "TrueHuffer - Huffman file compressor\n"
          "\n"
          "Usage:\n"
          "  %s deflate <file> [-o <output>] [-f]   compress a file "
          "(default output: <file>%s)\n"
          "  %s inflate <file> [-o <output>] [-f]   decompress a %s file\n",
          prog, THMAN_EXTENSION, prog, THMAN_EXTENSION);
#ifndef NO_GUI
  fprintf(out,
          "  %s gui                                 open the graphical "
          "interface (also the default without arguments)\n",
          prog);
#endif
  fprintf(out,
          "  %s -h | --help                         show this help\n"
          "\n"
          "Options:\n"
          "  -o, --output <file>   where to write the result\n"
          "  -f, --force           overwrite the output file if it exists\n"
          "\n"
          "Aliases: compress = deflate, decompress = inflate.\n",
          prog);
}

static bool file_exists(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp)
    return false;
  fclose(fp);
  return true;
}

static int process_file(const char *prog, bool deflating, const char *input,
                        const char *output, bool force) {
  int rc = EXIT_FAILURE;
  TextObject *to = NULL;
  char *auto_output = NULL;

  if (!output) {
    auto_output = make_output_path(input, deflating);
    if (!auto_output) {
      fprintf(stderr, "%s: out of memory\n", prog);
      return EXIT_FAILURE;
    }
    output = auto_output;
  }

  if (!force && file_exists(output)) {
    fprintf(stderr, "%s: '%s' already exists (use -f to overwrite)\n", prog,
            output);
    goto done;
  }

  to = init_text_obj(input);
  if (!to) {
    fprintf(stderr, "%s: %s\n", prog, compress_last_error());
    goto done;
  }

  size_t before;
  if (deflating) {
    if (!to->inflatedContent) {
      fprintf(stderr, "%s: '%s' is already deflated\n", prog, input);
      goto done;
    }
    before = to->inflatedSize;
    if (deflate(to) != EXIT_SUCCESS) {
      fprintf(stderr, "%s: %s\n", prog, compress_last_error());
      goto done;
    }
  } else {
    if (!to->deflatedContent) {
      fprintf(stderr, "%s: '%s' is not a TrueHuffer (%s) file\n", prog, input,
              THMAN_EXTENSION);
      goto done;
    }
    before = to->deflatedSize;
    if (inflate(to) != EXIT_SUCCESS) {
      fprintf(stderr, "%s: %s\n", prog, compress_last_error());
      goto done;
    }
  }

  if (writeTextObjToFile(to, output) != EXIT_SUCCESS) {
    fprintf(stderr, "%s: %s\n", prog, compress_last_error());
    goto done;
  }

  size_t after = deflating ? to->deflatedSize : to->inflatedSize;
  printf("%s -> %s\n", input, output);
  if (deflating && before > 0)
    printf("%zu bytes -> %zu bytes (%.1f%% of the original size)\n", before,
           after, 100.0 * (double)after / (double)before);
  else
    printf("%zu bytes -> %zu bytes\n", before, after);
  rc = EXIT_SUCCESS;

done:
  free_text_obj(to);
  free(auto_output);
  return rc;
}

static int run_cli(int argc, char **argv) {
  const char *prog = argv[0];
  const char *cmd = argv[1];

  if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0 ||
      strcmp(cmd, "help") == 0) {
    print_usage(stdout, prog);
    return EXIT_SUCCESS;
  }

  if (strcmp(cmd, "gui") == 0) {
#ifndef NO_GUI
    return start_gui(1, argv);
#else
    fprintf(stderr, "%s: this build has no GUI (built with `make no-gui`)\n",
            prog);
    return EXIT_FAILURE;
#endif
  }

  bool deflating;
  if (strcmp(cmd, "deflate") == 0 || strcmp(cmd, "compress") == 0) {
    deflating = true;
  } else if (strcmp(cmd, "inflate") == 0 || strcmp(cmd, "decompress") == 0) {
    deflating = false;
  } else {
    fprintf(stderr, "%s: unknown command '%s'\n\n", prog, cmd);
    print_usage(stderr, prog);
    return EXIT_USAGE;
  }

  const char *input = NULL;
  const char *output = NULL;
  bool force = false;

  for (int i = 2; i < argc; i++) {
    const char *arg = argv[i];
    if (strcmp(arg, "-f") == 0 || strcmp(arg, "--force") == 0) {
      force = true;
    } else if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "%s: option '%s' needs a file name\n", prog, arg);
        return EXIT_USAGE;
      }
      output = argv[++i];
    } else if (arg[0] == '-' && arg[1] != '\0') {
      fprintf(stderr, "%s: unknown option '%s'\n", prog, arg);
      return EXIT_USAGE;
    } else if (!input) {
      input = arg;
    } else {
      fprintf(stderr, "%s: only one input file is supported\n", prog);
      return EXIT_USAGE;
    }
  }

  if (!input) {
    fprintf(stderr, "%s: missing input file\n\n", prog);
    print_usage(stderr, prog);
    return EXIT_USAGE;
  }

  return process_file(prog, deflating, input, output, force);
}

int main(int argc, char **argv) {
  if (argc < 2) {
#ifndef NO_GUI
    return start_gui(argc, argv);
#else
    print_usage(stderr, argv[0]);
    return EXIT_USAGE;
#endif
  }

  return run_cli(argc, argv);
}
