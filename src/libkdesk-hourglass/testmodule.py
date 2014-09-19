#!/usr/bin/env python

# testmodule.py
#
# Copyright (C) 2013-2014 Kano Computing Ltd.
# License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
#
# A python test app to load and call the kdesk-hourglass dynamic library
#

from kdesk.hourglass import hourglass_start, hourglass_end
import os
import sys
import time

# Application to test for startup hourglass
app='/usr/bin/xcalc &'
appname='xcalc'

hourglass_start(appname)
rc = os.system(app)
if (rc!=0):
    hourglass_end()
    sys.exit(1)

print 'doing other tasks...'
time.sleep(5)

print 'byebye!'
sys.exit(rc)
