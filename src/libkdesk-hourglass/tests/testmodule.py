#!/usr/bin/env python

# testmodule.py
#
# Copyright (C) 2013-2014 Kano Computing Ltd.
# License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
#
# A python test app to load and call the kdesk-hourglass dynamic library
#

from kdesk.hourglass import hourglass_start, hourglass_start_appcmd, hourglass_end
import os
import sys
import time

# Application to test for startup hourglass
app='/usr/bin/xcalc'
appname='xcalc'

# testing the appname API
hourglass_start(appname)
rc = os.system(app)
if (rc!=0):
    hourglass_end()
    sys.exit(1)

time.sleep(1)

# testing the cmdline API
hourglass_start_appcmd(app)
rc = os.system(app)
if (rc!=0):
    hourglass_end()
    sys.exit(1)
    
print 'byebye!'
sys.exit(rc)
