//
//  hid.cpp - Human Input Device
//
//  This module encapsulates functions to deal with user keyboard and mouse activity
//  needed to decide when the screen saver needs to stop.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include "hid.h"

HID_HANDLE hid_init(int flags)
{
    int n=0;
    char buf[BUFF_SIZE];

    // Allocate a structure to hold a list of HID devices
    HID_HANDLE hid=(HID_HANDLE) calloc (sizeof (HID_STRUCT), 1);
    if (!hid) {
        return NULL;
    }

    // Give a gratious time for the input event streams to flush type-ahead events
    usleep (GRACE_START_TIME * 1000);

    // Open all possible input devices (keyboard / mouse),
    // save their fd like handles
    // Emptying any possible type-ahead events.

    hid->fdkbd0 = open(chkbd0, O_RDWR | O_NOCTTY | O_NDELAY);
    if (hid->fdkbd0 != -1) {
        fcntl(hid->fdkbd0, F_SETFL, O_NONBLOCK);
        n=read(hid->fdkbd0, (void*)buf, sizeof(buf));
    }

    hid->fdkbd1 = open(chkbd1, O_RDWR | O_NOCTTY | O_NDELAY);
    if (hid->fdkbd1 != -1) {
        fcntl(hid->fdkbd1, F_SETFL, O_NONBLOCK);
        n=read(hid->fdkbd1, (void*)buf, sizeof(buf));
    }

    hid->fdkbd2 = open(chkbd2, O_RDWR | O_NOCTTY | O_NDELAY);
    if (hid->fdkbd2 != -1) {
        fcntl(hid->fdkbd2, F_SETFL, O_NONBLOCK);
        n=read(hid->fdkbd2, (void*)buf, sizeof(buf));
    }
    
    hid->fdmouse0 = open(chmouse0, O_RDWR | O_NOCTTY | O_NDELAY);
    if (hid->fdmouse0 != -1) {
        fcntl(hid->fdmouse0, F_SETFL, O_NONBLOCK);
        n=read(hid->fdmouse0, (void*)buf, sizeof(buf));
    }

    hid->fdmouse1 = open(chmouse1, O_RDWR | O_NOCTTY | O_NDELAY);
    if (hid->fdmouse1 != -1) {
        fcntl(hid->fdmouse1, F_SETFL, O_NONBLOCK);
        n=read(hid->fdmouse1, (void*)buf, sizeof(buf));
    }

    hid->fdmice = open(chmice, O_RDWR | O_NOCTTY | O_NDELAY);
    if (hid->fdmice != -1) {
        fcntl(hid->fdmice, F_SETFL, O_NONBLOCK);
        n=read(hid->fdmice, (void*)buf, sizeof(buf));
    }

    return hid;
}

bool hid_is_user_idle (HID_HANDLE hid)
{
    int rc=0;
    fd_set hid_devices;
    struct timeval tv;

    if (!hid) {
        return false;
    }

    FD_ZERO(&hid_devices);

    // Add all devices we want to listen to, into a list that "access()" likes
    FD_SET(hid->fdkbd0, &hid_devices);
    FD_SET(hid->fdkbd1, &hid_devices);
    FD_SET(hid->fdkbd2, &hid_devices);
    FD_SET(hid->fdmouse0, &hid_devices);
    FD_SET(hid->fdmouse1, &hid_devices);
    FD_SET(hid->fdmice, &hid_devices);


    // setting timeout values to zero means return immediately
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // return ASAP with indication on wether there is a HID input event or not
    rc = select (6, &hid_devices, NULL, NULL, &tv);
    return ((rc ? true : false));
}

void hid_terminate(HID_HANDLE hid)
{
    return;
}
