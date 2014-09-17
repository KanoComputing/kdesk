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
import subprocess
import time
import os

app='/usr/bin/xcalc'
appname='xcalc'

print 'loading the hourglass dynamic library'
libname='libkdesk-hourglass.so'
kdesk_hourglass_lib=ctypes.CDLL(libname)

print 'showing the hourglass before loading the app'
c_appname=ctypes.c_char_p(appname)
kdesk_hourglass_lib.kdesk_hourglass_start(c_appname)

try:
    # as soon as the app is responsive, the hourglass will self-disappear
    cmdline=app.split(' ')
    p = subprocess.Popen(cmdline)
    print 'our app is ready and responsive, removing the hourglass'
    p.communicate()
except:
    # something went wrong... stop the hourglass
    print 'could not start app.. cancelling hourglass'
    kdesk_hourglass_lib.kdesk_hourglass_start(appname)

print 'seeya!'
sys.exit(0)
