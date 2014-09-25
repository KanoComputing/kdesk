//
// kdesk-hourglass.h
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// A shared library to provide apps with a desktop app loading mouse hourglass.
//

#ifdef __cplusplus
extern "C"
#endif

void kdesk_hourglass_start(char *appname);

#ifdef __cplusplus
extern "C"
#endif

void kdesk_hourglass_end(void);
