#!/bin/bash
#
#  icon_exit_sample.sh - Shell script invoked by kdesk to process Icon Alerts
#
#  $1 will contain the icon name. You can do the processing needed and then return
#  one of the icon attributes to refresh them immediately, through stdout.
#
#  To test this script, execute:
#
#   $ kdesk -a myicon
#
#  Where myicon is the filename of your desired icon without the .lnk extension
#  Your icon should change the below attributes on the desktop.
#

printf "Icon: /usr/share/icons/gnome/32x32/actions/insert-image.png\n"
printf "Caption: New Caption\n"
printf "Message: Ouch!!|Hello world\n"
