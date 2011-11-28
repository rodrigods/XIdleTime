/*
 * Copyright (C) 2011 Universidade Federal de Campina Grande
 *  
 * This file is part of OurGrid. 
 *
 * OurGrid is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation, either version 3 of the License, or (at your option) 
 * any later version. 
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/scrnsaver.h>
#include <stdio.h>

unsigned long workaroundCreepyXServer(Display *dpy, unsigned long _idleTime );

unsigned long getIdleTime()
{
  XScreenSaverInfo ssi;
  Display *dpy;
  int event_basep, error_basep;

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    //fprintf(stderr, "Couldn't open display\n");
    return -1;
  }
  
  if (!XScreenSaverQueryExtension(dpy, &event_basep, &error_basep)) {
    //fprintf(stderr, "Screen saver extension not supported\n");
    return -1;
  }
  
  if (!XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), &ssi)) {
    //fprintf(stderr, "Couldn't query screen saver info\n");
    return -1;
  }
  
  return workaroundCreepyXServer(dpy, ssi.idle);
}

/*!
 * This function works around an XServer idleTime bug in the
 * XScreenSaverExtension if dpms is running. In this case the current
 * dpms-state time is always subtracted from the current idletime.
 * This means: XScreenSaverInfo->idle is not the time since the last
 * user activity, as descriped in the header file of the extension.
 * This result in SUSE bug # and sf.net bug #. The bug in the XServer itself
 * is reported at https://bugs.freedesktop.org/buglist.cgi?quicksearch=6439.
 *
 * Workaround: Check if if XServer is in a dpms state, check the 
 *             current timeout for this state and add this value to 
 *             the current idle time and return.
 *
 * \param _idleTime a unsigned long value with the current idletime from
 *                  XScreenSaverInfo->idle
 * \return a unsigned long with the corrected idletime
 */
unsigned long workaroundCreepyXServer(Display *dpy, unsigned long _idleTime ){
  int dummy;
  CARD16 standby, suspend, off;
  CARD16 state;
  BOOL onoff;

  if (DPMSQueryExtension(dpy, &dummy, &dummy)) {
    if (DPMSCapable(dpy)) {
      DPMSGetTimeouts(dpy, &standby, &suspend, &off);
      DPMSInfo(dpy, &state, &onoff);

      if (onoff) {
        switch (state) {
          case DPMSModeStandby:
            /* this check is a littlebit paranoid, but be sure */
            if (_idleTime < (unsigned) (standby * 1000))
              _idleTime += (standby * 1000);
            break;
          case DPMSModeSuspend:
            if (_idleTime < (unsigned) ((suspend + standby) * 1000))
              _idleTime += ((suspend + standby) * 1000);
            break;
          case DPMSModeOff:
            if (_idleTime < (unsigned) ((off + suspend + standby) * 1000))
              _idleTime += ((off + suspend + standby) * 1000);
            break;
          case DPMSModeOn:
          default:
            break;
        }
      }
    } 
  }

  XCloseDisplay(dpy);

  return _idleTime;
}
