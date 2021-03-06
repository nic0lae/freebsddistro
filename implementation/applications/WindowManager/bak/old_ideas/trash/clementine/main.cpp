/* Clementine Window Manager
   Copyright 2002 Dave Berton <db@mosey.org>

   based on aewm Copyright 1998-2001 Decklin Foster <decklin@red-bean.com>

   This program is free software; see LICENSE for details. */


#include <iostream>
#include "windowmanager.h"


int main(int argc, char **argv)
{
    try {
	WindowManager wm;
	windowManager = &wm; // set global reference
	wm.eventLoop();
    } catch( const char*  e ) {
	std::cerr << "Unhandled exception: " << e << std::endl;
    }
    return 0;
}
