//
// testlib.cpp  -  C++ sample to test the hourglass library
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// A shared library to provide apps with a desktop app loading mouse hourglass.
//
// Compilation: gcc testdynlib.cpp -ldl -o testdynlib
//

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>

void *lib_handle;
void (*fn_start)(char *);
void (*fn_end)(void);
char *error=NULL;

char *app=(char *)"/usr/bin/xcalc &";
char *appname=(char *)"xcalc";

int main(void)
{

  printf ("loading the hourglass dynamic library\n");
  lib_handle = (void*) dlopen("libkdesk-hourglass.so", RTLD_LAZY);
  if (!lib_handle) 
    {
      fprintf(stderr, "%s\n", dlerror());
      exit(1);
    }

  // load the library function entry points
  fn_start = ( void (*)(char *) ) dlsym(lib_handle, "kdesk_hourglass_start");
  if ((error = dlerror()) != NULL) {
    fprintf(stderr, "%s\n", error);
    exit(1);
  }
  else {
    
    fn_end = ( void (*)() ) dlsym(lib_handle, "kdesk_hourglass_end");
    if ((error = dlerror()) != NULL) {
      fprintf(stderr, "%s\n", error);
      exit(1);
    }
    else {
      printf ("showing the hourglass before loading the app\n");
      (*fn_start)(appname);
    }
  }

  printf ("do the lengthy loading job here (hourglass is visible)\n");
  int rc = system (app);
  if (rc) {
    printf ("something went wrong, removing the hourglass\n");
    (*fn_end)();
  }
  else {
    printf ("app started, hourglass should disappear once it is responsive\n");
  }

  printf ("seeya!\n");
  dlclose(lib_handle);
  return 0;
}

