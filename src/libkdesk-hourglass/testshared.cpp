//
// testsharedlib.cpp  -  C++ sample te test the shared hourglass library
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// A shared library to provide apps with a desktop app loading mouse hourglass.
//
// Compilation: gcc testshared.cpp -lkdesk-hourglass -L/usr/lib -o testshared.cpp
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "kdesk-hourglass.h"

char *defapp=(char *)"/usr/bin/xcalc";
char *defappname=(char *)"xcalc";

int main(int argc, char *argv[]) {

  char *app=defapp, *appname=defappname;

  if (argc == 3) {
    appname=strdup(argv[1]);
    app=strdup(argv[2]);
  }

  printf ("Starting appname: %s (%s)\n", appname, app);
  kdesk_hourglass_start(appname);
  int rc = system (app);
  if (rc) {
    printf ("something went wrong, removing the hourglass\n");
    kdesk_hourglass_end();
  }

  printf ("app started - byebye!\n");
  exit(0);
}
