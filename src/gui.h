#ifndef GUI_H
#define GUI_H

// Opens the GTK3 window and blocks until it is closed.
// Returns EXIT_SUCCESS / EXIT_FAILURE (e.g. when no display is available).
int start_gui(int argc, char **argv);

#endif // GUI_H
