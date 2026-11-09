#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "compress.h"
#include "gui.h"

typedef struct GuiState {
  GtkWidget *window;
  GtkWidget *entry;  // path of the selected file
  GtkWidget *status; // result / error line at the bottom
} GuiState;

static void set_status(GuiState *st, const char *text) {
  gtk_label_set_text(GTK_LABEL(st->status), text);
}

static void show_error(GuiState *st, const char *message) {
  GtkWidget *dialog = gtk_message_dialog_new(
      GTK_WINDOW(st->window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message);
  gtk_window_set_title(GTK_WINDOW(dialog), "TrueHuffer - Error");
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  char *text = g_strdup_printf("Failed: %s", message);
  set_status(st, text);
  g_free(text);
}

// Opens a file chooser. Returns the chosen path (free with g_free()) or NULL
// if the user cancelled. For save dialogs, suggested_path pre-fills the name
// and the user is asked before overwriting an existing file.
static char *choose_file(GuiState *st, const char *title,
                         GtkFileChooserAction action, const char *accept_label,
                         const char *suggested_path) {
  GtkWidget *dialog = gtk_file_chooser_dialog_new(
      title, GTK_WINDOW(st->window), action, "_Cancel", GTK_RESPONSE_CANCEL,
      accept_label, GTK_RESPONSE_ACCEPT, NULL);
  GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

  if (action == GTK_FILE_CHOOSER_ACTION_SAVE) {
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    if (suggested_path) {
      char *dir = g_path_get_dirname(suggested_path);
      char *base = g_path_get_basename(suggested_path);
      gtk_file_chooser_set_current_folder(chooser, dir);
      gtk_file_chooser_set_current_name(chooser, base);
      g_free(dir);
      g_free(base);
    }
  }

  char *filename = NULL;
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    filename = gtk_file_chooser_get_filename(chooser);
  gtk_widget_destroy(dialog);
  return filename;
}

static void run_operation(GuiState *st, bool deflating) {
  char *input = g_strdup(gtk_entry_get_text(GTK_ENTRY(st->entry)));
  g_strstrip(input);
  if (input[0] == '\0') {
    show_error(st, "Please choose a file first.");
    g_free(input);
    return;
  }

  TextObject *to = init_text_obj(input);
  if (!to) {
    show_error(st, compress_last_error());
    g_free(input);
    return;
  }

  size_t before;
  if (deflating) {
    if (!to->inflatedContent) {
      show_error(st, "This file is already deflated.");
      goto cleanup;
    }
    before = to->inflatedSize;
    if (deflate(to) != EXIT_SUCCESS) {
      show_error(st, compress_last_error());
      goto cleanup;
    }
  } else {
    if (!to->deflatedContent) {
      show_error(st, "This is not a TrueHuffer (.thman) file.");
      goto cleanup;
    }
    before = to->deflatedSize;
    if (inflate(to) != EXIT_SUCCESS) {
      show_error(st, compress_last_error());
      goto cleanup;
    }
  }

  {
    size_t after = deflating ? to->deflatedSize : to->inflatedSize;
    char *suggestion = make_output_path(input, deflating);
    char *output =
        choose_file(st, deflating ? "Save deflated file" : "Save inflated file",
                    GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", suggestion);
    free(suggestion);

    if (!output) {
      set_status(st, "Cancelled: nothing was written.");
    } else if (writeTextObjToFile(to, output) != EXIT_SUCCESS) {
      show_error(st, compress_last_error());
    } else {
      char *base = g_path_get_basename(output);
      char *text;
      if (deflating && before > 0)
        text =
            g_strdup_printf("Saved %s: %" G_GSIZE_FORMAT " -> %" G_GSIZE_FORMAT
                            " bytes (%.1f%% of the original size)",
                            base, (gsize)before, (gsize)after,
                            100.0 * (double)after / (double)before);
      else
        text = g_strdup_printf("Saved %s: %" G_GSIZE_FORMAT
                               " -> %" G_GSIZE_FORMAT " bytes",
                               base, (gsize)before, (gsize)after);
      set_status(st, text);
      g_free(text);
      g_free(base);
    }
    g_free(output);
  }

cleanup:
  free_text_obj(to);
  g_free(input);
}

static void on_browse_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  GuiState *st = user_data;
  char *path = choose_file(st, "Select a file", GTK_FILE_CHOOSER_ACTION_OPEN,
                           "_Open", NULL);
  if (path) {
    gtk_entry_set_text(GTK_ENTRY(st->entry), path);
    set_status(st, "File selected.");
    g_free(path);
  }
}

static void on_deflate_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  run_operation(user_data, true);
}

static void on_inflate_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  run_operation(user_data, false);
}

int start_gui(int argc, char **argv) {
  if (!gtk_init_check(&argc, &argv)) {
    fprintf(stderr, "Could not open a display: cannot start the GUI.\n");
    return EXIT_FAILURE;
  }

  GuiState st = {NULL, NULL, NULL};

  st.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(st.window), "TrueHuffer");
  gtk_window_set_default_size(GTK_WINDOW(st.window), 520, 1);
  gtk_container_set_border_width(GTK_CONTAINER(st.window), 12);
  g_signal_connect(st.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_container_add(GTK_CONTAINER(st.window), vbox);

  // Row 1: [ path entry ........ ] [Browse...]
  GtkWidget *file_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  st.entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(st.entry),
                                 "File to deflate or inflate...");
  GtkWidget *browse = gtk_button_new_with_mnemonic("_Browse...");
  gtk_box_pack_start(GTK_BOX(file_row), st.entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(file_row), browse, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), file_row, FALSE, FALSE, 0);

  // Row 2: [Deflate] [Inflate]
  GtkWidget *action_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous(GTK_BOX(action_row), TRUE);
  GtkWidget *deflate_btn = gtk_button_new_with_mnemonic("_Deflate (compress)");
  GtkWidget *inflate_btn =
      gtk_button_new_with_mnemonic("_Inflate (decompress)");
  gtk_box_pack_start(GTK_BOX(action_row), deflate_btn, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(action_row), inflate_btn, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), action_row, FALSE, FALSE, 0);

  // Row 3: status line
  st.status = gtk_label_new("Choose a file, then deflate or inflate it.");
  gtk_label_set_xalign(GTK_LABEL(st.status), 0.0f);
  gtk_label_set_line_wrap(GTK_LABEL(st.status), TRUE);
  gtk_box_pack_start(GTK_BOX(vbox), st.status, FALSE, FALSE, 0);

  g_signal_connect(browse, "clicked", G_CALLBACK(on_browse_clicked), &st);
  g_signal_connect(deflate_btn, "clicked", G_CALLBACK(on_deflate_clicked), &st);
  g_signal_connect(inflate_btn, "clicked", G_CALLBACK(on_inflate_clicked), &st);

  gtk_widget_show_all(st.window);
  gtk_main();

  return EXIT_SUCCESS;
}
