#!/usr/bin/env python
#
# hourglass.py
#
# Copyright (C) 2014 Kano Computing Ltd.
# License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
#
# Provides a Python interface to kdesk hourglass library.
#

import ctypes

libname='libkdesk-hourglass.so'
kdesk_hourglass_lib=ctypes.CDLL(libname)

def hourglass_start(appname):
    c_appname=ctypes.c_char_p(appname)
    kdesk_hourglass_lib.kdesk_hourglass_start(c_appname)

def hourglass_end():
    kdesk_hourglass_lib.kdesk_hourglass_end()
