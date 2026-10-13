#include <stdlib.h>

#include "src/compress.h"
#ifndef NO_GUI
#include "gui.h" //adds gui.h only for gui compilation mode with make.
#endif

int main()
{

    #ifndef NO_GUI
    start_gui(); 
    #else
    #endif

  return EXIT_SUCCESS;
}
