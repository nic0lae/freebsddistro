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

#ifndef GPGKEYSELECT_H
#define GPGKEYSELECT_H

#include "licqdialog.h"

#include <qobject.h>
#include <qlistview.h>
#include <qdialog.h>

class CICQDaemon;
class ICQUser;
class QCheckBox;
class QLineEdit;

class KeyView : public QListView
{
    Q_OBJECT
public:
  KeyView( QWidget *parent, const char *szId, unsigned long nPPID );
  ~KeyView() {};

protected:
  const char *szId;
  unsigned long nPPID;
  void testViewItem( QListViewItem *item, ICQUser *u );
  int maxItemVal;
  QListViewItem *maxItem;
  void initKeyList();
  virtual void resizeEvent(QResizeEvent *);
};

class GPGKeySelect : public QDialog
{
  Q_OBJECT
public:
  GPGKeySelect ( const char *szId, unsigned long nPPID, QWidget *parent = 0);
  ~GPGKeySelect();

protected:
  QListView *keySelect;
  QCheckBox *useGPG;
  char *szId;
  unsigned long nPPID;
  void updateIcon();
  QLineEdit *filterText;

protected slots:
  void slot_ok();
  void slotNoKey();
  void slotCancel();
  void slot_doubleClicked ( QListViewItem *, const QPoint &, int );
  void filterTextChanged ( const QString &str );
signals:
  void signal_done();
};

#endif
