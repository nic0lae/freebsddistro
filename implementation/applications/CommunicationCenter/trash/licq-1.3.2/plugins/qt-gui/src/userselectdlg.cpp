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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qcombobox.h>

#include "userselectdlg.h"
#include "licq_icqd.h"
#include "licq_user.h"

UserSelectDlg::UserSelectDlg(CICQDaemon *s, QWidget *parent)
  : LicqDialog(parent, "UserSelectDialog", true, WStyle_ContextHelp | WDestructiveClose)
{
	server = s;

	QVBoxLayout *lay = new QVBoxLayout(this, 10, 5);

	frmUser = new QFrame(this);
	QHBoxLayout *layUser = new QHBoxLayout(frmUser);
	lblUser = new QLabel(tr("&User:"), frmUser);
	cmbUser = new QComboBox(frmUser);
	lblUser->setBuddy(cmbUser);
	layUser->addWidget(lblUser);
	layUser->addWidget(cmbUser);

	frmPassword = new QFrame(this);
	QHBoxLayout *layPassword = new QHBoxLayout(frmPassword);
	lblPassword = new QLabel(tr("&Password:"), frmPassword);
	edtPassword = new QLineEdit(frmPassword);
	edtPassword->setEchoMode(QLineEdit::Password);
	edtPassword->setFocus();
	lblPassword->setBuddy(edtPassword);
	layPassword->addWidget(lblPassword);
	layPassword->addWidget(edtPassword);

	chkSavePassword = new QCheckBox(tr("&Save Password"), this);

	frmButtons = new QFrame(this);
	QHBoxLayout *layButtons = new QHBoxLayout(frmButtons);
	btnOk = new QPushButton(tr("&Ok"), frmButtons);
	btnCancel = new QPushButton(tr("&Cancel"), frmButtons);
	layButtons->addStretch();
	layButtons->addWidget(btnOk);
	layButtons->addSpacing(20);
	layButtons->addWidget(btnCancel);

	lay->addWidget(frmUser);
	lay->addWidget(frmPassword);
	lay->addWidget(chkSavePassword);
	lay->addStretch();
	lay->addWidget(frmButtons);

	connect(cmbUser, SIGNAL(activated(const QString &)), SLOT(slot_cmbSelectUser(const QString &)));
	connect(btnOk, SIGNAL(clicked()), SLOT(slot_ok()));
	connect(btnCancel, SIGNAL(clicked()), SLOT(close()));

	setCaption(tr("Licq User Select"));

	// Populate the combo box

	// For now, just have one owner
	ICQOwner *o = gUserManager.FetchOwner(LOCK_R);
  if (o == 0)
  {
    close();
    return;
  }
	cmbUser->insertItem(QString("%1 (%2)").arg(o->GetAlias()).arg(o->Uin()));
	edtPassword->setText(o->Password());
	gUserManager.DropOwner();

        exec(); // this is different from show(), exec() does not return 
                //control to caller until we have finished
}

UserSelectDlg::~UserSelectDlg()
{
}

void UserSelectDlg::slot_ok()
{
  ICQOwner *o = gUserManager.FetchOwner(LOCK_W);
  if (o == 0)
  {
    close();
    return;
  }
  o->SetSavePassword(chkSavePassword->isChecked());
  o->SetPassword(edtPassword->text().latin1());
  gUserManager.DropOwner();

  close();
}

void UserSelectDlg::slot_cmbSelectUser(const QString &str)
{
} 


#include "userselectdlg.moc"

