//
// kdesk-hourglass-app.cpp
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// C Client tool to bring up the hourglass for a specified app name
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "kdesk-hourglass.h"

char *defapp=(char *)"/usr/bin/xcalc";
char *defappname=(char *)"xcalc";

int main(int argc, char *argv[]) {

  char *app=defapp, *appname=defappname;

  if (argc < 2) {
      printf ("Syntax: kdesk-hourglass-app <[application name] [-r]>\n");
      printf ("  The -r parameter will cancel the hourglass\n");
      printf ("  Any other value will bring up the hourglass until the application shows up\n");
      exit(0);
  }
  else {
      // Remove the hourglass
      if (!strcmp(argv[1], "-r")) {
          kdesk_hourglass_end();
          exit(0);
      }
      else {
          // Start the hourglass for the app
          appname=strdup(argv[1]);
          kdesk_hourglass_start(appname);
          exit(0);
      }
  }
}
