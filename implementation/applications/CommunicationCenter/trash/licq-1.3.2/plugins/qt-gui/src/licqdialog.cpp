// -*- c-basic-offset: 2 -*-
/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.                              

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  

*/

#include "licqdialog.h"

extern "C" {

#include <X11/Xlib.h>
#include <X11/Xutil.h>

}

LicqDialog::LicqDialog(QWidget *parent, const char *name, bool modal, WFlags f)
  : QDialog(parent, name, modal, f)
{
  XClassHint classHint;
  classHint.res_class = (char *)qAppName();
  classHint.res_name = (char *)name;
  XSetClassHint(x11Display(), winId(), &classHint);
}

#include "licqdialog.moc"
