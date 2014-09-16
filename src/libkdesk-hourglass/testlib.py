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
import time

print 'loading the hourglass dynamic library'
libname='libkdesk-hourglass.so'
kdesk_hourglass_lib=ctypes.CDLL(libname)

print 'showing the hourglass before loading the app'
kdesk_hourglass_lib.kdesk_hourglass_start()

print 'do the lengthy loading job here (hourglass is visible)'
time.sleep(10)

print 'our app is ready and responsive, removing the hourglass'
kdesk_hourglass_lib.kdesk_hourglass_end()
time.sleep(1)

print 'seeya!'
sys.exit(0)
