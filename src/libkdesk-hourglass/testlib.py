#!/usr/bin/env python

# testlib.py
#
# Copyright (C) 2013-2014 Kano Computing Ltd.
# License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
#
# A python test app to load and call the kdesk-hourglass dynamic library
#

import ctypes
import sys

libname='libkdesk-hourglass.so'
kdesk_hourglass_lib=ctypes.CDLL(libname)
kdesk_hourglass_lib.kdesk_hourglass()
sys.exit(0)
