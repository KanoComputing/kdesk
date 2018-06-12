/**
 * paths.h
 *
 * Copyright (C) 2013-2018 Kano Computing Ltd.
 * License: http://www.gnu.org/licenses/gpl-2.0.txt GNU GPLv2
 *
 * Paths to standard kdesk files
 *
 */


#ifndef __KDESK_PATHS_H__
#define __KDESK_PATHS_H__


#define FILE_HOME_KDESKRC  ".kdeskrc"
#define FILE_KDESKRC       "/usr/share/kano-desktop/kdesk/.kdeskrc"
#define DIR_KDESKTOP       "/usr/share/kano-desktop/kdesk/kdesktop"
#define DIR_KDESKTOP_USER  ".kdesktop"

// Primary physical display, used to discern when to play sounds
// :0 alone means the first display on the local system.
#define DEFAULT_DISPLAY    ":0"


#endif  // __KDESK_PATHS_H__
