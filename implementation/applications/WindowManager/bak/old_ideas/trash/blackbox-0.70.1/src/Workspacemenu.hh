// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// WorkspaceMenu.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
//         Bradley T Hughes <bhughes at trolltech.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef   __Workspacemenu_hh
#define   __Workspacemenu_hh

#include <Menu.hh>

// forward declarations
class BScreen;
class Iconmenu;
class Workspace;

class Workspacemenu : public bt::Menu {
public:
  Workspacemenu(bt::Application &app, unsigned int screen, BScreen *bscreen);

  void insertIconMenu(Iconmenu *iconmenu);

  void insertWorkspace(Workspace *workspace);
  void removeWorkspace(unsigned int id);

  void setWorkspaceChecked(unsigned int id, bool checked);

protected:
  virtual void itemClicked(unsigned int id, unsigned int);

private:
  BScreen *_bscreen;
};

#endif // __Workspacemenu_hh
