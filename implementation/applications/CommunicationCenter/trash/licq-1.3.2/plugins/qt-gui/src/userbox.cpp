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
#include <ctype.h>
#include <stdio.h>

#include <qpainter.h>
#include <qpopupmenu.h>
#include <qheader.h>
#include <qkeycode.h>
#include <qscrollbar.h>
#include <qdragobject.h>
#include <qstylesheet.h>
#include <qdatetime.h>
#include <qtextcodec.h>
#include <qdrawutil.h>
#include <qregexp.h>
#include <qstyle.h>

#include "userbox.moc"
#include "skin.h"
#include "mainwin.h"
#include "userbox.h"
#include "gui-defines.h"
#include "mainwin.h"
#include "usereventdlg.h"
#include "usercodec.h"

#include "licq_user.h"
#include "licq_socket.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#undef Status

#define FLASH_TIME 500

extern char *PPIDSTRING(unsigned long);

QColor  *CUserViewItem::s_cOnline = NULL,
        *CUserViewItem::s_cAway = NULL,
        *CUserViewItem::s_cOffline = NULL,
        *CUserViewItem::s_cNew = NULL,
        *CUserViewItem::s_cBack = NULL,
        *CUserViewItem::s_cGridLines = NULL;

//-----CUserViewItem::constructor-----------------------------------------------
CUserViewItem::CUserViewItem(ICQUser *_cUser, QListView *parent)
  : QListViewItem(parent)
{
  if(listView()->parent() == NULL)
    listView()->setCaption(CUserView::tr("%1 Floaty (%2)")
                           .arg(_cUser->GetAlias()).arg(_cUser->IdString()));

  m_nUin = _cUser->Uin();
  if (_cUser->IdString())
  {
    char *szRealId = 0;
    ICQUser::MakeRealId(_cUser->IdString(), _cUser->PPID(), szRealId);
    m_szId = strdup(szRealId);
    delete [] szRealId;
  }
  else
    m_szId = 0;
  m_szAlias = _cUser->GetAlias();
  m_nPPID = _cUser->PPID();
  m_bUrgent = false;
  m_bSecure = false;
  m_bBirthday = false;
  m_bPhone = false;
  m_bCellular = false;
  m_nOnlCount = 0;
  m_nEvents = 0;
  m_nStatus = ICQ_STATUS_OFFLINE;
  m_bGPGKey = false;
  m_bNotInList = _cUser->NotInList();
  setGraphics(_cUser);
}


CUserViewItem::CUserViewItem (ICQUser *_cUser, CUserViewItem* item)
  : QListViewItem(item),
    m_sGroupName()
{
  m_nGroupId = (unsigned short)(-1);
  m_nUin = _cUser->Uin();
  if (_cUser->IdString())
  {
    char *szRealId = 0;
    ICQUser::MakeRealId(_cUser->IdString(), _cUser->PPID(), szRealId);
    m_szId = strdup(szRealId);
    delete [] szRealId;
  }
  else
    m_szId = 0;

  m_szAlias = _cUser->GetAlias();
  m_nPPID = _cUser->PPID();
  m_bUrgent = false;
  m_bSecure = false;
  m_bBirthday = false;
  m_bPhone = false;
  m_bCellular = false;
  m_bGPGKey = false;
  m_nOnlCount = 0;
  m_nEvents = 0;
  m_nStatus = ICQ_STATUS_OFFLINE;
  m_bGPGKey = false;
  m_bNotInList = _cUser->NotInList();
  setGraphics(_cUser);
}

CUserViewItem::CUserViewItem(unsigned short Id, const char* name, QListView* lv)
  : QListViewItem(lv),
    m_nGroupId(Id),
    m_sGroupName(name)
{
  m_nUin = 0;
  m_szId = 0;
  m_szAlias = 0;
  m_nPPID =0;
  m_pIcon = NULL;
  m_cBack = s_cBack;
  m_cFore = s_cGridLines;
  m_bItalic = m_bStrike = false;
  m_nWeight = QFont::Bold;
  m_bUrgent = false;
  m_bSecure = false;
  m_bBirthday = false;
  m_bPhone = false;
  m_bCellular = false;
  m_nOnlCount = 0;
  m_nEvents = 0;
  m_nStatus = 0;
  // Other users group is sorted at the end
  if (m_nGroupId)
  {
    QString strTemp = QString("%1").arg((int)m_nGroupId);
    while (strTemp.length() < 10)
      strTemp = "0" + strTemp;
    m_sSortKey = strTemp;
  }
  else
    m_sSortKey = QString("9999999999");
  m_sPrefix = "1";
  setPixmap(0, gMainWindow->pmCollapsed);
  setText(1, QString::fromLocal8Bit(name));

}

CUserViewItem::CUserViewItem(BarType barType, QListView *parent)
  : QListViewItem(parent),
    m_sGroupName()
{
  m_nGroupId = (unsigned short)(-1);
  m_nUin = 0;
  m_szId = 0;
  m_nPPID = 0;
  m_nOnlCount = 0;
  m_nEvents = 0;
  m_nStatus = 0;
  m_pIcon = NULL;
  m_cBack = s_cBack;
  m_cFore = s_cOnline;
  m_bItalic = m_bStrike = false;
  m_nWeight = QFont::Normal;
  m_bUrgent = false;
  m_bSecure = false;
  setSelectable(false);
  setHeight(10);
  m_sSortKey = "";
  if (barType == BAR_ONLINE)
    m_sPrefix = "0";
  else if (barType == BAR_OFFLINE)
    m_sPrefix = "2";
  else
    m_sPrefix = "4";
}

void CUserViewItem::SetThreadViewGroupTitle()
{
    QString t;
    /*if (m_nEvents > 0)
      t = QString("* ");*/
    t += QString::fromLocal8Bit(m_sGroupName);
    if (m_nOnlCount > 0)
      t += QString(" (") + QString::number(m_nOnlCount) + QString(")");
    setText(1, t);
}


CUserViewItem::~CUserViewItem()
{
  CUserView *v = (CUserView *)listView();

  if (m_szId)
    free(m_szId);

  if (v == NULL) return;

  if (this == v->barOffline || this == v->barOnline || this == v->barNotInList)
    return;

  if (m_bNotInList)
    v->numNotInList--;
  else
  {
    if (m_nStatus == ICQ_STATUS_OFFLINE)
      v->numOffline--;
    else
      v->numOnline--;
  }

  if (parent())
  {
    CUserViewItem *i = static_cast<CUserViewItem*>(parent());
    if (m_nPPID && m_nStatus != ICQ_STATUS_OFFLINE) i->m_nOnlCount--;
    i->m_nEvents -= m_nEvents;
    i->SetThreadViewGroupTitle();
  }

  if (v->numOffline == 0 && v->barOffline != NULL)
  {
    delete v->barOffline;
    v->barOffline = NULL;
  }
  if (v->numOnline == 0 && v->barOnline != NULL)
  {
    delete v->barOnline;
    v->barOnline = NULL;
  }
  if (v->numNotInList == 0 && v->barNotInList != NULL)
  {
    delete v->barNotInList;
    v->barNotInList = NULL;
  }
}

//-----CUserViewItem::setGraphics-----------------------------------------------
void CUserViewItem::setGraphics(ICQUser *u)
{
   char *sTemp;
   CUserView *v = (CUserView *)listView();

   if (parent())
   {
     CUserViewItem *i = static_cast<CUserViewItem*>(parent());
     if (u->StatusOffline() && m_nStatus != ICQ_STATUS_OFFLINE)
       i->m_nOnlCount--;
     if (m_nStatus == ICQ_STATUS_OFFLINE && !u->StatusOffline())
       i->m_nOnlCount++;

     i->m_nEvents -= m_nEvents;
     i->m_nEvents += u->NewMessages();

     i->SetThreadViewGroupTitle();
   }

   m_nStatus = u->Status();
   m_nStatusFull = u->StatusFull();
   m_bStatusInvisible = u->StatusInvisible();
   m_bStatusTyping = u->GetTyping() == ICQ_TYPING_ACTIVE;
   m_nPhoneFollowMeStatus = u->PhoneFollowMeStatus();
   m_nICQphoneStatus = u->ICQphoneStatus();
   m_nSharedFilesStatus = u->SharedFilesStatus();
   m_bCustomAR = u->CustomAutoResponse()[0] != '\0';
   m_nEvents = u->NewMessages();
   m_bSecure = u->Secure();
   m_bUrgent = false;
   m_bBirthday =  (u->Birthday() == 0);
   m_bPhone  = u->GetPhoneNumber()[0] != '\0';
   m_bCellular = u->GetCellularNumber()[0] !='\0';
   m_bGPGKey = ( u->GPGKey()!=NULL ) && ( strcmp( u->GPGKey(), "" )!=0 );
   m_bGPGKeyEnabled = ( u->UseGPG() );

   // Create any necessary bars
   if (u->NotInList())
   {
     v->numNotInList++;
     if (v->barNotInList == NULL && v->parent()
         && !(gMainWindow->m_bThreadView && gMainWindow->m_nGroupType == GROUPS_USER
         && gMainWindow->m_nCurrentGroup == 0))
       v->barNotInList = new CUserViewItem(BAR_NOTINLIST, listView());
   }
   else
   {
     if (u->StatusOffline())
     {
       v->numOffline++;
       if (v->barOffline == NULL && v->parent()
           && gMainWindow->m_bShowDividers
           && !(gMainWindow->m_bThreadView && gMainWindow->m_nGroupType == GROUPS_USER
           && gMainWindow->m_nCurrentGroup == 0))
         v->barOffline = new CUserViewItem(BAR_OFFLINE, listView());
     }
     else
     {
       v->numOnline++;
       if (v->barOnline == NULL && v->parent()
           && gMainWindow->m_bShowDividers &&
           !(gMainWindow->m_bThreadView && gMainWindow->m_nGroupType == GROUPS_USER
             && gMainWindow->m_nCurrentGroup == 0))
         v->barOnline = new CUserViewItem(BAR_ONLINE, listView());
     }
   }

   m_sPrefix = "1";
   m_pIcon = &CMainWindow::iconForStatus(u->StatusFull(), u->IdString(), u->PPID());

   switch (m_nStatus)
   {
     case ICQ_STATUS_AWAY:
     case ICQ_STATUS_OCCUPIED:
     case ICQ_STATUS_DND:
     case ICQ_STATUS_NA:
       m_cFore = s_cAway;
       break;
     case ICQ_STATUS_OFFLINE:
       m_cFore = s_cOffline;
       m_sPrefix = "3";
       break;
     case ICQ_STATUS_ONLINE:
     case ICQ_STATUS_FREEFORCHAT:
     default:
       m_cFore = s_cOnline;
   }

   if (m_bNotInList)
     m_sPrefix = "5";

   //if (u->StatusInvisible())
   //   m_cFore = s_cAway;

   m_pIconStatus = m_pIcon;

   if (u->NewMessages() > 0)
   {
     m_pIcon = NULL;
     unsigned short SubCommand = 0;

     for (unsigned short i = 0; i < u->NewMessages(); i++)
     {
       switch(u->EventPeek(i)->SubCommand())
       {
         case ICQ_CMDxSUB_FILE:
           SubCommand = ICQ_CMDxSUB_FILE;
           break;
         case ICQ_CMDxSUB_CHAT:
           if (SubCommand != ICQ_CMDxSUB_FILE) SubCommand = ICQ_CMDxSUB_CHAT;
           break;
         case ICQ_CMDxSUB_URL:
           if (SubCommand != ICQ_CMDxSUB_FILE && SubCommand != ICQ_CMDxSUB_CHAT)
             SubCommand = ICQ_CMDxSUB_URL;
           break;
         case ICQ_CMDxSUB_CONTACTxLIST:
           if(SubCommand != ICQ_CMDxSUB_FILE && SubCommand != ICQ_CMDxSUB_CHAT
              && SubCommand != ICQ_CMDxSUB_URL)
             SubCommand = ICQ_CMDxSUB_CONTACTxLIST;
         case ICQ_CMDxSUB_MSG:
         default:
           if (SubCommand == 0) SubCommand = ICQ_CMDxSUB_MSG;
           break;
       }
       if (u->EventPeek(i)->IsUrgent()) m_bUrgent = true;
     }
     if(SubCommand)
       m_pIcon = &CMainWindow::iconForEvent(SubCommand);
   }

   m_bFlash = ((u->NewMessages() > 0 && gMainWindow->m_nFlash == FLASH_ALL) ||
       (m_bUrgent && gMainWindow->m_nFlash == FLASH_URGENT));

   if (v->msgTimerId == 0 && m_bFlash)
     v->msgTimerId = v->startTimer(FLASH_TIME);

   if (u->NewUser() &&
       !(gMainWindow->m_nGroupType == GROUPS_SYSTEM && gMainWindow->m_nCurrentGroup == GROUP_NEW_USERS) )
      m_cFore = s_cNew;

   m_cBack = s_cBack;
   if (m_pIcon != NULL) setPixmap(0, *m_pIcon);

   m_bItalic = m_bStrike = false;
   m_nWeight = QFont::Normal;
   if (gMainWindow->m_bFontStyles)
   {
     if (u->OnlineNotify()) m_nWeight = QFont::DemiBold;
     if (u->InvisibleList()) m_bStrike = true;
     if (u->VisibleList()) m_bItalic = true;
   }
   if (u->NewMessages() > 0) m_nWeight = QFont::Bold;

   for (unsigned short i = 0; i < gMainWindow->colInfo.size(); i++)
   {
     QTextCodec * codec = UserCodec::codecForICQUser(u);
     sTemp = u->usprintf(gMainWindow->colInfo[i]->m_szFormat);
     if (strcmp(gMainWindow->colInfo[i]->m_szFormat, "%a") == 0)
       setText(i + 1, QString::fromUtf8(sTemp));
     else
       setText(i + 1, codec->toUnicode(sTemp));
     free(sTemp);
   }

   // Set the user tag
   int sort = 9;
   switch (m_nStatus)
   {
   case ICQ_STATUS_FREEFORCHAT:
   case ICQ_STATUS_ONLINE:
     sort = 0;
     break;
   case ICQ_STATUS_OCCUPIED:
     sort = 1;
     break;
   case ICQ_STATUS_DND:
     sort = 2;
     break;
   case ICQ_STATUS_AWAY:
     sort = 3;
     break;
   case ICQ_STATUS_NA:
     sort = 4;
     break;
   case ICQ_STATUS_OFFLINE:
     sort = 5;
     break;
   }
   // Set sorting
   switch (gMainWindow->m_nSortByStatus)
   {
     case 0:  // no sorting
       break;
     case 1:  // sort by status
       m_sSortKey.sprintf("%1x", sort);
       break;
     case 2:  // sort by status and last event
       m_sSortKey.sprintf("%1x%016lx",sort, ULONG_MAX - u->Touched());
       break;
     case 3:  // sort by status and number of new messages
       m_sSortKey.sprintf("%1x%016lx",sort, ULONG_MAX - u->NewMessages());
   }
}


// ---------------------------------------------------------------------------

void CUserViewItem::paintCell( QPainter *p, const QColorGroup & cgdefault, int column, int width, int align )
{
  QFont newFont(p->font());
  newFont.setWeight(m_nWeight);
  if (gMainWindow->m_bFontStyles)
  {
    newFont.setItalic(m_bItalic);
    newFont.setStrikeOut(m_bStrike);
  }
  p->setFont(newFont);

  bool onlBlink = (listView()->onlTimerId && listView()->onlId && m_szId &&
                   strcmp(listView()->onlId, m_szId) == 0 &&
                   listView()->onlPPID == m_nPPID && listView()->onlCounter & 1);

  QColorGroup cg(cgdefault.foreground(), cgdefault.background(),
    cgdefault.light(), cgdefault.dark(), cgdefault.mid(),
    onlBlink ? *s_cOffline : *m_cFore, *m_cBack);

  const QPixmap *pix = NULL;

  if (listView()->parent() && gMainWindow->skin->frame.transparent )
    pix = listView()->QListView::parentWidget()->paletteBackgroundPixmap();
  listView()->setStaticBackground(pix && listView()->contentsHeight() >= listView()->viewport()->height());

  if (pix != NULL)
  {
    QRect r(listView()->itemRect(this));
    QPoint pd(r.topLeft()+QPoint(listView()->header()->sectionPos(column), 0));
    listView()->viewport()->mapToParent(pd);
    QPoint pp(listView()->mapToParent(pd));
    //p->drawPixmap(0, 0, *pix, pp.x(), pp.y(), width, height());
    p->drawTiledPixmap(0, 0, width, height(), *pix, pp.x(), pp.y());
  }
  else
    p->fillRect( 0, 0, width, height(), cg.base());

  if (m_szId || isGroupItem())
  {
    cg.setBrush(QColorGroup::Base, QBrush(NoBrush));
    // If this is a floaty then don't draw the highlight box
    if (listView()->parent() == NULL || isGroupItem())
    {
      cg.setBrush(QColorGroup::Highlight, QBrush(NoBrush));
      cg.setColor(QColorGroup::HighlightedText, cg.text());
    }

    if (isGroupItem())
    {
      QFont f(p->font());
      if(f.pointSize() > 2)
        f.setPointSize(f.pointSize() - 2);
      p->setFont(f);
    }

    QListViewItem::paintCell(p, cg, column, width, align);

    // first determine where to start painting for the current column
    int w = 0; // width
    switch (align)
    {
      case LEFT:
                w = p->fontMetrics().width(text(column)) + 6;
                break;
      case RIGHT:
                w = listView()->columnWidth(column) - p->fontMetrics().width(text(column)) - 6;
                break;
      case CENTER:
                w = (listView()->columnWidth(column) / 2) + (p->fontMetrics().width(text(column)) / 2) + 6;
                break;
      default:
                w = 0;
                break;
    }

    if (isGroupItem())
    {
      if (column == 1)
      {
        w = (align == RIGHT) ? w+2 : w-2;  //  +/- 2

        if (m_nEvents > 0 && !isOpen())
        {
          if (align == RIGHT)
            w -= gMainWindow->pmMessage.width();
          p->drawPixmap(w, 0, gMainWindow->pmMessage);
          w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmMessage.width() + 2;
        }
        qDrawShadeLine( p, 0, height() >> 1, width - 5, (height() >> 1) + 2, cg, true, 1, 0);
      }
      else if (column == listView()->header()->count() - 1)
      {
        qDrawShadeLine( p, 0, height() >> 1, width - 5, (height() >> 1) + 2, cg, true, 1, 0);
      }
      else if (column > 1)
      {
        qDrawShadeLine( p, 0, height() >> 1, width - 5, (height() >> 1) + 2, cg, true, 1, 0);
      }
    } // isGroupItem
    else if (m_szAlias                                        // this row contains a user 
             && text(column).ascii()                          // ...+ it is not empty 
             && strcmp(m_szAlias, text(column).ascii()) == 0  // ...+ this col contains the users alias 
             && gMainWindow->m_bShowExtendedIcons)            // ...+ extended icons are enabled
    {
      // pmPhone
      if (width - w > 8 && (m_bPhone))
      {
        if (align == RIGHT)
          w -= gMainWindow->pmPhone.width();
        p->drawPixmap(w, 0, gMainWindow->pmPhone);
        w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmPhone.width() + 2;
      }

      // pmCellular
      if (width - w > 8 && (m_bCellular))
      {
        if (align == RIGHT)
          w -= gMainWindow->pmCellular.width();
        p->drawPixmap(w, 0, gMainWindow->pmCellular);
        w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmCellular.width() + 2;
      }

      // pmBirthday
      if (width - w > 8 && (m_bBirthday))
      {
        if (align == RIGHT)
          w -= gMainWindow->pmBirthday.width();
        p->drawPixmap(w, 0, gMainWindow->pmBirthday);
        w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmBirthday.width() + 2;
      }

      // pmPrivate / pmInvisible
      if (width - w > 8 && m_bStatusInvisible)
      {
        if (gMainWindow->pmInvisible.isNull())
        {
          if (align == RIGHT)
            w -= gMainWindow->pmPrivate.width();
          p->drawPixmap(w, 0, gMainWindow->pmPrivate);
          w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmPrivate.width() + 2;
        }
        else
        {
          if (align == RIGHT)
            w -= gMainWindow->pmInvisible.width();
          p->drawPixmap(w, 0, gMainWindow->pmInvisible);
          w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmInvisible.width() + 2;
        }
      }
      
      // pmGPGKey
      if ( width - w > 8 && m_bGPGKey )
      {
        if ( m_bGPGKeyEnabled )
        {
          if (align == RIGHT)
            w -= gMainWindow->pmGPGKeyEnabled.width();
          p->drawPixmap(w, 0, gMainWindow->pmGPGKeyEnabled);
          w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmGPGKeyEnabled.width() + 2;
        }
        else
        {
          if (align == RIGHT)
            w -= gMainWindow->pmGPGKeyDisabled.width();
          p->drawPixmap(w, 0, gMainWindow->pmGPGKeyDisabled);
          w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmGPGKeyDisabled.width() + 2;
        }
      }
      if (m_nStatus != ICQ_STATUS_OFFLINE)
      {
        if (width - w > 8)
        {
          if (m_nPhoneFollowMeStatus == ICQ_PLUGIN_STATUSxACTIVE)
          {
            if (align == RIGHT)
              w -= gMainWindow->pmPhoneFollowMeActive.width();
            p->drawPixmap(w, 0, gMainWindow->pmPhoneFollowMeActive);
            w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmPhoneFollowMeActive.width() + 2;
          }
          else if (m_nPhoneFollowMeStatus == ICQ_PLUGIN_STATUSxBUSY)
          {
            if (align == RIGHT)
              w -= gMainWindow->pmPhoneFollowMeBusy.width();
            p->drawPixmap(w, 0, gMainWindow->pmPhoneFollowMeBusy);
            w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmPhoneFollowMeBusy.width() + 2;
          }
        }
        if (width - w > 8)
        {
          if (m_nICQphoneStatus == ICQ_PLUGIN_STATUSxACTIVE)
          {
            if (align == RIGHT)
              w -= gMainWindow->pmICQphoneActive.width();
            p->drawPixmap(w, 0, gMainWindow->pmICQphoneActive);
            w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmICQphoneActive.width() + 2;
          }
          else if (m_nICQphoneStatus == ICQ_PLUGIN_STATUSxBUSY)
          {
            if (align == RIGHT)
              w -= gMainWindow->pmICQphoneBusy.width();
            p->drawPixmap(w, 0, gMainWindow->pmICQphoneBusy);
            w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmICQphoneBusy.width() + 2;
          }
        }
        if (width - w > 8 && m_nSharedFilesStatus == ICQ_PLUGIN_STATUSxACTIVE)
        {
          if (align == RIGHT)
            w -= gMainWindow->pmSharedFiles.width();
          p->drawPixmap(w, 0, gMainWindow->pmSharedFiles);
          w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmSharedFiles.width() + 2;
        }
        if (width - w > 8 && m_bStatusTyping && m_nPPID == LICQ_PPID)
        {
          if (align == RIGHT)
            w -= gMainWindow->pmTyping.width();
          p->drawPixmap(w, 0, gMainWindow->pmTyping);
          w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmTyping.width() + 2;
        }
      }
      if (width - w > 8 && m_bSecure)
      {
        if (align == RIGHT)
          w -= gMainWindow->pmSecureOn.width();
        p->drawPixmap(w, 0, gMainWindow->pmSecureOn);
        w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmSecureOn.width() + 2;
      }
      if (width - w > 8 && m_bCustomAR)
      {
        if (align == RIGHT)
          w -= gMainWindow->pmCustomAR.width();
        p->drawPixmap(w, 0, gMainWindow->pmCustomAR);
        w = (align == RIGHT) ? w - 2 : w + gMainWindow->pmCustomAR.width() + 2;
      }
    }
  }
  else
  {
    QFont newFont(p->font());
    newFont.setBold(false);
    newFont.setItalic(false);
    newFont.setStrikeOut(false);
    p->setFont(newFont);
    int x1 = 0, x2 = width;
    if (column == 0)
      x1 = 5;
    if (column == listView()->header()->count() - 1)
      x2 = width - 5;
    p->setPen(QPen(QColor(128, 128, 128), 1));
    p->drawLine(x1, height() >> 1, x2, height() >> 1);
    p->setPen(QPen(QColor(255, 255, 255), 1));
    p->drawLine(x1, (height() >> 1) + 1, x2, (height() >> 1) + 1);
    if (column == 1)
    {
      QString sz = CUserView::tr("Offline");
      if (m_sPrefix == "0")
        sz = CUserView::tr("Online");
      else if (m_sPrefix == "4")
        sz = CUserView::tr("Not In List");

      if (pix)
      {
        QRect r(listView()->itemRect(this));
        QPoint pd(r.topLeft()+QPoint(listView()->header()->sectionPos(column)+5, 0));
        listView()->viewport()->mapToParent(pd);
        QPoint pp(listView()->mapToParent(pd));
        //p->drawPixmap(5, 0, *pix, pp.x(), pp.y(), p->fontMetrics().width(sz) + 6, height());
        p->drawTiledPixmap(5, 0, p->fontMetrics().width(sz) + 6, height(), *pix, pp.x(), pp.y());
      }
      else
      {
        p->fillRect(5, 0, p->fontMetrics().width(sz) + 6, height(), *m_cBack);
      }
      QFont f(p->font());
      if (f.pointSize() > 2)
        f.setPointSize(f.pointSize() - 2);
      p->setFont(f);
      p->setPen(QPen(*s_cGridLines));
      p->drawText(8, 0, width - 8, height(), AlignVCenter, sz);
    }
  }

  // add line to bottom and right side
  if (listView()->parent() && gMainWindow->m_bGridLines && m_szId)
  {
    p->setPen(*s_cGridLines);
    p->drawLine(0, height() - 1, width - 1, height() - 1);
    p->drawLine(width - 1, 0, width - 1, height() - 1);
  }

  if ((listView()->carTimerId > 0) && 
      (m_szId != NULL) &&        // Be careful, can be null when repainting a group seperator
      (strcmp(listView()->carId, m_szId) == 0) &&
      (listView()->carPPID == m_nPPID))
    drawCAROverlay(p);
}


void CUserView::paintEmptyArea( QPainter *p, const QRect &r )
{
  const QPixmap *pix = NULL;
  if( parent() && gMainWindow->skin->frame.transparent)
    pix = QListView::parentWidget()->backgroundPixmap();

  if (pix != NULL)
  {
    QPoint pp(mapToParent(r.topLeft()));
    //p->drawPixmap(r.x(), r.y(), *pix, pp.x(), pp.y(), r.width(), r.height());
    p->drawTiledPixmap(r.x(), r.y(), r.width(), r.height(), *pix, pp.x(), pp.y());
  }
  else
  {
    QListView::paintEmptyArea(p, r);
  }
}

void CUserViewItem::drawCAROverlay(QPainter* p)
{
  QRect r(listView()->itemRect(this));
  if(!r.isValid())
    // is not on screen
    return;

  p->setBackgroundMode(OpaqueMode);
  p->setBackgroundColor((listView()->carCounter & 1) ? black : white);
  p->setPen(QPen((listView()->carCounter & 1) ? white : black, 1, SolidLine));
  p->drawRect(r);
}

void CUserView::timerEvent(QTimerEvent* e)
{
  bool doGroupView = gMainWindow->m_bThreadView &&
    gMainWindow->m_nGroupType == GROUPS_USER &&
    gMainWindow->m_nCurrentGroup == 0;

  if(e->timerId() == carTimerId)
  {
    QListViewItemIterator it(this);

    if (carCounter > 0 && carId)
    {
      QPainter p(viewport());
      for(; it.current(); ++it)
      {
        CUserViewItem* item = static_cast<CUserViewItem*>(it.current());

        char *szRealId = 0;
        ICQUser::MakeRealId(item->ItemId(), item->ItemPPID(), szRealId);
        if (item->ItemId() && (strcmp(szRealId, carId) == 0) &&
            item->ItemPPID() == carPPID)
        {
          if(carCounter == 1)
            item->repaint();
          else
            item->drawCAROverlay(&p);
          if(!doGroupView)
          {
            delete [] szRealId;
            break;
          }
        }
        delete [] szRealId;
      }
    }

    if(--carCounter == 0) {
      carUin = 0;

      if (carId)
      {
        free(carId);
        carId = 0;
      }
      carPPID = 0;
      killTimer(carTimerId);
      carTimerId = 0;
    }
  }
  else if(e->timerId() == onlTimerId)
  {
    QListViewItemIterator it(this);
    bool found = false;

    if (onlId)
    {
      for(; it.current(); ++it)
      {
        CUserViewItem* item = static_cast<CUserViewItem*>(it.current());

        char *szRealId = 0;
        ICQUser::MakeRealId(item->ItemId(), item->ItemPPID(), szRealId);

        if (szRealId && (strcmp(szRealId, onlId) == 0) &&
            item->ItemPPID() == onlPPID)
        {
          found = true;
          item->repaint();
          if(!doGroupView)
          {
            delete [] szRealId;
            break;
          }
        }

        if (szRealId)
          delete [] szRealId;
      }
    }

    if(!found || (--onlCounter == 0)) {
      onlUin = 0;

      if (onlId)
      {
        free(onlId);
        onlId = 0;
      }
      onlPPID = 0;
      killTimer(onlTimerId);
      onlTimerId = 0;
    }
  }
  else
  {
    if (m_nFlashCounter++ & 1) // hide event icon
    {
      QListViewItemIterator it(this);
      for(; it.current(); ++it)
      {
        CUserViewItem *item = static_cast<CUserViewItem*>(it.current());

        if (item->ItemId() && item->m_bFlash && item->m_pIconStatus != NULL)
        {
          item->setPixmap(0, *item->m_pIconStatus);
        }
      }
    }
    else
    {
      // show
      bool foundIcon = false;
      QListViewItemIterator it(this);

      for(; it.current(); ++it)
      {
        CUserViewItem* item = static_cast<CUserViewItem*>(it.current());

        if (item->ItemId() && item->m_bFlash && item->m_pIcon != NULL)
        {
          foundIcon = true;
          item->setPixmap(0, *item->m_pIcon);
        }
      }
      // no pending messages any more, kill timer
      if(!foundIcon)
      {
        killTimer(msgTimerId);
        msgTimerId = 0;
      }
    }
  }
}

//-----CUserViewItem::key-------------------------------------------------------
QString CUserViewItem::key (int column, bool ascending) const
{
  if (column == 0)
    return (m_sPrefix + m_sSortKey + text(1).lower());
  else
    return (m_sPrefix + QListViewItem::key(column, ascending).lower());
}

UserFloatyList* CUserView::floaties = 0;

//-----UserList::constructor-----------------------------------------------------------------------
CUserView::CUserView(QPopupMenu *m, QWidget *parent, const char *name)
  : QListView(parent, name, parent == NULL ? WStyle_Customize | WStyle_NoBorder | WResizeNoErase | WRepaintNoErase /*| WStyle_StaysOnTop*/
              : WRepaintNoErase),
    QToolTip(viewport())
{
  m_nFlashCounter = carCounter = onlCounter = 0;
  msgTimerId = carTimerId = onlTimerId = 0;
  mnuUser = m;
  barOnline = barOffline = barNotInList = NULL;
  numOnline = numOffline = numNotInList = 0;

  m_typeAhead = "";
  m_typePos   = 0;

  addColumn(tr("S"), 20);

  for (unsigned short i = 0; i < gMainWindow->colInfo.size(); i++)
  {
    addColumn(gMainWindow->colInfo[i]->m_sTitle, gMainWindow->colInfo[i]->m_nWidth);
    setColumnAlignment(i + 1, 1 << gMainWindow->colInfo[i]->m_nAlign);
  }

  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);

  setShowSortIndicator(true);
  setAllColumnsShowFocus(true);
  setTreeStepSize(0);
  setSorting(gMainWindow->m_nSortColumn, gMainWindow->m_bSortColumnAscending);
  setVScrollBarMode(gMainWindow->m_bScrollBar ? Auto : AlwaysOff);

  if (parent != NULL)
  {
    setShowHeader(gMainWindow->m_bShowHeader);
    setFrameStyle(gMainWindow->skin->frame.frameStyle);
    connect(this, SIGNAL(expanded(QListViewItem*)), this, SLOT(itemExpanded(QListViewItem*)));
    connect(this, SIGNAL(collapsed(QListViewItem*)), this, SLOT(itemCollapsed(QListViewItem*)));
  }
  else
  {
    char szClass[16];
    sprintf(szClass, "Floaty%d", floaties->size() + 1);
    setWFlags(getWFlags() | WDestructiveClose);
    setShowHeader(false);
    setFrameStyle(33);
    XClassHint classhint;
    classhint.res_name = "licq";
    classhint.res_class = szClass;
    XSetClassHint(x11Display(), winId(), &classhint);
    XWMHints *hints = XGetWMHints(x11Display(), winId());
    hints->window_group = winId();
    hints->flags = WindowGroupHint;
    XSetWMHints(x11Display(), winId(), hints);
    XFree( hints );
    floaties->resize(floaties->size()+1);
    floaties->insert(floaties->size()-1, this);
  }

  carId = onlId = 0;
}

// -----------------------------------------------------------------------------
// CUserView destructor

CUserView::~CUserView()
{
  barOnline = barOffline = barNotInList = NULL;
  if (parent() == NULL)
  {
    unsigned int i = 0;
    for (; i<floaties->size(); i++)
    {
      if (floaties->at(i) == this) {
        floaties->take(i);
        break;
      }
    }
    while(i+1 < floaties->size()) {
        floaties->insert(i, floaties->at(i+1));
        i++;
    }
    if(floaties->size())
        floaties->resize(floaties->size()-1);
  }

  if (carId)  free(carId);
  if (onlId)  free(onlId);
}

CUserView *CUserView::FindFloaty(const char *szId, unsigned long nPPID)
{
  unsigned int i = 0;
  for (; i < floaties->size(); i++)
  {
    CUserViewItem *p = static_cast<CUserViewItem *>(floaties->at(i)->firstChild());
    if (p->ItemId() && (strcmp(p->ItemId(), szId) == 0) && p->ItemPPID() == nPPID)
      break;
  }
  if (i < floaties->size()) return floaties->at(i);

  return NULL;
}

void CUserView::clear()
{
  barOnline = barOffline = barNotInList = NULL;
  QListView::clear();
  numOffline = numOnline = numNotInList = 0;
}


//-----CUserView::setColors-----------------------------------------------------
void CUserView::setColors(char *_sOnline, char *_sAway, char *_sOffline,
                          char *_sNew, char *_sBack, char *_sGridLines)
{
   if (CUserViewItem::s_cOnline == NULL) CUserViewItem::s_cOnline = new QColor;
   if (CUserViewItem::s_cAway == NULL) CUserViewItem::s_cAway = new QColor;
   if (CUserViewItem::s_cOffline == NULL) CUserViewItem::s_cOffline = new QColor;
   if (CUserViewItem::s_cNew == NULL) CUserViewItem::s_cNew = new QColor;
   if (CUserViewItem::s_cBack == NULL) CUserViewItem::s_cBack = new QColor;
   if (CUserViewItem::s_cGridLines == NULL) CUserViewItem::s_cGridLines = new QColor;

   CUserViewItem::s_cOnline->setNamedColor(_sOnline);
   CUserViewItem::s_cAway->setNamedColor(_sAway);
   CUserViewItem::s_cOffline->setNamedColor(_sOffline);
   CUserViewItem::s_cNew->setNamedColor(_sNew);
   CUserViewItem::s_cGridLines->setNamedColor(_sGridLines);
   if (gMainWindow->m_bSystemBackground)
   {
     *CUserViewItem::s_cBack =
      QListView::palette().color(QPalette::Normal, QColorGroup::Base);
   }
   else
   {
     if (_sBack != NULL)
       CUserViewItem::s_cBack->setNamedColor(_sBack);
     else
       CUserViewItem::s_cBack->setNamedColor("grey76");

     QPalette pal(QListView::palette());
     pal.setColor(QColorGroup::Base, *CUserViewItem::s_cBack);
     QListView::setPalette(pal);
   }
}


void CUserView::setShowHeader(bool isHeader)
{
  isHeader ? header()->show() : header()->hide();
}


unsigned long CUserView::MainWindowSelectedItemUin()
{
   CUserViewItem *i = (CUserViewItem *)currentItem();
   if (i == NULL) return (0);
   return i->ItemUin();
}

bool CUserView::MainWindowSelectedItemUser(char *&_szId, unsigned long &_nPPID)
{
  CUserViewItem *i = (CUserViewItem *)currentItem();
  if (i == NULL) return false;
  _nPPID = i->ItemPPID();
  _szId = i->ItemId() ? strdup(i->ItemId()) : 0;
  return true;
}

//-----CUserList::mousePressEvent---------------------------------------------
void CUserView::viewportMousePressEvent(QMouseEvent *e)
{
  QListView::viewportMousePressEvent(e);

  if (e->button() == LeftButton)
  { 
    mousePressPos = e->pos();
    CUserViewItem *clickedItem = static_cast<CUserViewItem*>(itemAt(e->pos()));
    if (clickedItem != NULL && e->pos().x() < header()->sectionSize(0) && 
        clickedItem->isGroupItem())
      clickedItem->setOpen(!clickedItem->isOpen());
    m_typeAhead = "";
    m_typePos = 0;
  }
  else if (e->button() == MidButton)
  {
    QPoint clickPoint(e->x(), e->y());
    QListViewItem *clickedItem = itemAt(clickPoint);
    if (clickedItem != NULL)
    {
      setSelected(clickedItem, true);
      setCurrentItem(clickedItem);
      emit doubleClicked(clickedItem);
    }
    m_typeAhead = "";
    m_typePos = 0;
  }
}

void CUserView::contentsContextMenuEvent ( QContextMenuEvent* e )
{
  CUserViewItem *clickedItem = (CUserViewItem *)itemAt(contentsToViewport( e->pos()) );
  if (clickedItem != NULL)
  {
    m_typeAhead = "";
    m_typePos = 0;
    setSelected(clickedItem, true);
    setCurrentItem(clickedItem);
    if (clickedItem->ItemId())
    {
      gMainWindow->SetUserMenuUser(clickedItem->ItemId(), clickedItem->ItemPPID());
      mnuUser->popup(viewport()->mapToGlobal(contentsToViewport(e->pos())), 1);
    }
  }
}

void CUserView::viewportDragEnterEvent(QDragEnterEvent* e)
{
  e->accept(QTextDrag::canDecode(e) || QUriDrag::canDecode(e));
}


void CUserView::viewportDropEvent(QDropEvent* e)
{
  CUserViewItem* it = static_cast<CUserViewItem*>(itemAt(e->pos()));

  if(it)
  {
    if (it->ItemId())
    {
      QString text;
      QStrList lst;
      if(QUriDrag::decode(e, lst))
      {
        QStrListIterator strIter(lst);
        if(!(text = QUriDrag::uriToLocalFile(strIter)).isEmpty())
        {
          UserSendFileEvent *e = static_cast<UserSendFileEvent *>
            (gMainWindow->callFunction(mnuUserSendFile, it->ItemId(),
                it->ItemPPID()));
          e->setFile(text, QString::null);
          
          // Add all the files
          while (strIter != lst.getLast())
          {
            ++strIter;
            if (!(text = QUriDrag::uriToLocalFile(strIter)).isEmpty())
              e->addFile(text);
          }
          
          e->show();
        }
        else
        {
          UserSendUrlEvent *e = static_cast<UserSendUrlEvent *>
            (gMainWindow->callFunction(mnuUserSendUrl, it->ItemId(),
                it->ItemPPID()));
          e->setUrl(text, QString::null);
          e->show();
        }
      }
      
      //TODO change this
      else if(QTextDrag::decode(e, text)) {
//        const char *p = (text.left(4).latin1());
        char *szId = strdup((text.mid(4, text.length() - 4).latin1()));
        unsigned long nPPID = LICQ_PPID; //TODO fix this

        if(szId) {
          if(strcmp(szId, it->ItemId()) == 0 && nPPID == it->ItemPPID()) return;
          UserSendContactEvent* e = static_cast<UserSendContactEvent*>
            (gMainWindow->callFunction(mnuUserSendContact, it->ItemId(), it->ItemPPID()));
          ICQUser* u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
          QString alias = u ? u->GetAlias() : "";
          gUserManager.DropUser(u);

          e->setContact(szId, nPPID, alias);
          e->show();
        }
        else {
          UserSendMsgEvent* e = static_cast<UserSendMsgEvent*>
            (gMainWindow->callFunction(mnuUserSendMsg, it->ItemId(), it->ItemPPID()));
          e->setText(text);
          e->show();
        }

        free (szId);
      }
    }
    //TODO Change this
    else if(it->isGroupItem())
    {
      QString text;
      if(QTextDrag::decode(e, text)) {
//        const char *p = (text.left(4).latin1());
        char *szId = strdup(text.mid(4, text.length() - 4).latin1());
        unsigned long nPPID = LICQ_PPID; //TODO Fix this

        if(szId) {
          gUserManager.AddUserToGroup(szId, nPPID, it->GroupId());
          gMainWindow->updateUserWin();
        }

        free(szId);
      }
    }
  }
}


void CUserView::keyPressEvent(QKeyEvent *e)
{
  if (e->state() & ControlButton || e->state() & AltButton)
  {
    e->ignore();
    QListView::keyPressEvent(e);
    return;
  }

  switch (e->key())
  {
    case Key_Return:
    case Key_Enter:
    case Key_Space:
    {
      CUserViewItem* item = static_cast<CUserViewItem*>(currentItem());
      if(item == NULL) return;

      if(item->isGroupItem()) {
        setOpen(item, !item->isOpen());
        return;
      }

      // user divider
      if (item->ItemId() == 0)  return;
      gMainWindow->SetUserMenuUser(item->ItemId(), item->ItemPPID());
      mnuUser->popup(viewport()->mapToGlobal(QPoint(40, itemPos(item))));
      return;
    }

    case Key_Home:
    {

      QListViewItemIterator it(this);

      while (it.current() != NULL &&
            ((CUserViewItem *)(it.current()))->ItemId() == 0) ++it;
      setSelected(it.current(), true);
      ensureItemVisible(it.current());
      m_typeAhead = "";
      m_typePos = 0;
      return;
    }

    case Key_End:
    {

      QListViewItemIterator it(this);
      QListViewItem* lastitem = 0;
      while(it.current() != NULL) {
        lastitem = it.current();
        ++it;
      }
      it = lastitem;

      while (it.current() && ((CUserViewItem *)(it.current()))->ItemId() == 0) --it;
      setSelected(it.current(), true);
      ensureItemVisible(it.current());
      m_typeAhead = "";
      m_typePos = 0;
      return;
    }

    case Key_Backspace:
      if (m_typePos > 0)
      {
        m_typeAhead.truncate(m_typeAhead.length()-1);
	m_typePos--;
      }
      /* fall through */
      
    default:
    {
      int ascii = tolower(e->ascii());
      if (!isalnum(ascii) && e->key() != Key_Backspace)
      {
        QListView::keyPressEvent(e);
	m_typeAhead = "";
	m_typePos = 0 ;
        return;
      }

      m_typeAhead+=ascii;
      m_typePos++;

      QListViewItemIterator it(firstChild());
      
      while (it.current() != NULL)
      {
        CUserViewItem* item = static_cast<CUserViewItem*>(it.current());
        {
	  if (item->text(1).lower().startsWith(m_typeAhead))
	  {
            setSelected(item, true);
            ensureItemVisible(item);
	    item->repaint();
            return;
          }
          ++it;
        }
      }

      // If we are here we didn't find any names
      QListView::keyPressEvent(e);
      m_typeAhead = ascii;
      m_typePos = 1 ;
    }
  }
}


void CUserView::resizeEvent(QResizeEvent *e)
{
  QListView::resizeEvent(e);

  unsigned short totalWidth = 0;
  unsigned short nNumCols = header()->count();
  for (unsigned short i = 0; i < nNumCols - 1; i++)
    totalWidth += columnWidth(i);

  //QScrollBar *s = verticalScrollBar();
  //if (s != NULL) totalWidth += s->width();
  int newWidth = width() - totalWidth - 2;
  if (newWidth <= 0)
  {
    setHScrollBarMode(Auto);
    setColumnWidth(nNumCols - 1, gMainWindow->colInfo[nNumCols - 2]->m_nWidth);
  }
  else
  {
    setHScrollBarMode(AlwaysOff);
    setColumnWidth(nNumCols - 1, newWidth);
  }
}

void CUserView::AnimationAutoResponseCheck(unsigned long uin)
{
  if(carTimerId == 0) {
    // no animation yet running, so start the timer
    carTimerId = startTimer(FLASH_TIME);
    carCounter = 5*1000/FLASH_TIME; // run about 5 seconds
    carUin = uin;
  }
  // well, maybe we should move the animation to the other user
}

void CUserView::AnimationAutoResponseCheck(const char *szId, unsigned long nPPID)
{
  if(carTimerId == 0) {
    // no animation yet running, so start the timer
    carTimerId = startTimer(FLASH_TIME);
    carCounter = 5*1000/FLASH_TIME; // run about 5 seconds
    carId = szId ? strdup(szId) : 0;
    carPPID = nPPID;
  }
  // well, maybe we should move the animation to the other user
}

void CUserView::AnimationOnline(unsigned long uin)
{
  if(onlTimerId == 0) {
    onlTimerId = startTimer(FLASH_TIME);
    // run about 5 seconds, make sure that that the
    // actual "flashing" starts with a delay, because of the
    // logon case.
    onlCounter = ((5*1000/FLASH_TIME)+1)&(-2);
    onlUin = uin;
  }
  else if((onlCounter & 1) == 0 && onlUin != uin)
  {
    // whoops, another user went online
    // we just block here the blinking for the
    // rest of the time
    onlUin = 0;
    // no need for a redraw, as the user is already shown
    // correctly.
  }
}

void CUserView::AnimationOnline(const char *szId, unsigned long nPPID)
{
  if(onlTimerId == 0) {
    onlTimerId = startTimer(FLASH_TIME);
    // run about 5 seconds, make sure that that the
    // actual "flashing" starts with a delay, because of the
    // logon case.
    onlCounter = ((5*1000/FLASH_TIME)+1)&(-2);
    onlId = szId ? strdup(szId) : 0;
    onlPPID = nPPID;
  }
  else if((onlCounter & 1) == 0)
  {
    if (onlPPID == nPPID)
    {
      if (onlId && strcmp(szId, onlId) != 0)
      {
        // whoops, another user went online
        // we just block here the blinking for the
        // rest of the time
        free(onlId);
        onlId = 0;
        onlPPID = 0;
        // no need for a redraw, as the user is already shown
        // correctly.
      }
    }
    else
    {
      if (onlId) free(onlId);
      onlId = 0;
      onlPPID = 0;
    }
  }
}

// -----------------------------------------------------------------------------

void  CUserView::viewportMouseReleaseEvent(QMouseEvent* me)
{
  QListView::viewportMouseReleaseEvent( me );

  mousePressPos.setX(0);
  mousePressPos.setY(0);
}


void CUserView::UpdateFloaties()
{
  for (unsigned int i = 0; i<floaties->size(); i++)
  {
    CUserViewItem* item = static_cast<CUserViewItem*>(floaties->at(i)->firstChild());
    ICQUser *u = gUserManager.FetchUser(item->ItemId(), item->ItemPPID(), LOCK_R);
    if (u == NULL) return;
    item->setGraphics(u);
    gUserManager.DropUser(u);
    floaties->at(i)->triggerUpdate();
  }
}


// -----------------------------------------------------------------------------

void CUserView::viewportMouseMoveEvent(QMouseEvent * me)
{
  CUserViewItem *i;
  QListView::viewportMouseMoveEvent(me);

  if (parent() && (me->state() & LeftButton) && (i = (CUserViewItem *)currentItem())
      && !mousePressPos.isNull() && i->ItemId() &&
      (QPoint(me->pos() - mousePressPos).manhattanLength() > 8))
  {
    char *p = PPIDSTRING(i->ItemPPID());
    QString data(p);
    data += i->ItemId();
    delete [] p;
    QTextDrag *d = new QTextDrag(data, this);
    d->dragCopy();
  }
  else if(!parent() && me->state() & LeftButton) {
    move(me->globalX()-mousePressPos.x(), me->globalY()-mousePressPos.y());
  }
}


// -----------------------------------------------------------------------------

void CUserView::itemExpanded(QListViewItem* i)
{
  if(i == NULL) return;
  CUserViewItem* it = static_cast<CUserViewItem*>(i);

  gMainWindow->m_nGroupStates |= 1<<it->GroupId();

  if(!gMainWindow->pmExpanded.isNull() && it->isGroupItem())
    i->setPixmap(0, gMainWindow->pmExpanded);
}

void CUserView::itemCollapsed(QListViewItem* i)
{
  if(i == NULL) return;
  CUserViewItem* it = static_cast<CUserViewItem*>(i);

  gMainWindow->m_nGroupStates &= ~(1<<it->GroupId());

  if(!gMainWindow->pmCollapsed.isNull() && it->isGroupItem())
    i->setPixmap(0, gMainWindow->pmCollapsed);
}

// -----------------------------------------------------------------------------

void CUserView::maybeTip(const QPoint& c)
{
  CUserViewItem* item = static_cast<CUserViewItem*>(itemAt(c));
  if (item && item->m_szId)
  {
    QRect r(itemRect(item));
    QString s = QString("<nobr>") + QString(ICQUser::StatusToStatusStr(item->m_nStatus, item->m_bStatusInvisible))
      + QString("</nobr>");

    if (item->m_nStatusFull & ICQ_STATUS_FxBIRTHDAY)
      s += tr("<br><b>Birthday&nbsp;Today!</b>");
    
    if (item->m_nStatus != ICQ_STATUS_OFFLINE)
    {
      if (item->m_bStatusTyping)
        s += tr("<br>Typing&nbsp;a&nbsp;message");
      if (item->m_nPhoneFollowMeStatus == ICQ_PLUGIN_STATUSxACTIVE)
        s += tr("<br>Phone&nbsp;&quot;Follow&nbsp;Me&quot;:&nbsp;Available");
      else if (item->m_nPhoneFollowMeStatus == ICQ_PLUGIN_STATUSxBUSY)
        s += tr("<br>Phone&nbsp;&quot;Follow&nbsp;Me&quot;:&nbsp;Busy");

      if (item->m_nICQphoneStatus == ICQ_PLUGIN_STATUSxACTIVE)
        s += tr("<br>ICQphone:&nbsp;Available");
      else if (item->m_nICQphoneStatus == ICQ_PLUGIN_STATUSxBUSY)
        s += tr("<br>ICQphone:&nbsp;Busy");

      if (item->m_nSharedFilesStatus == ICQ_PLUGIN_STATUSxACTIVE)
        s += tr("<br>File&nbsp;Server:&nbsp;Enabled");
    }

    if (item->m_bSecure)
      s += tr("<br>Secure&nbsp;connection");

    if (item->m_bCustomAR)
      s += tr("<br>Custom&nbsp;Auto&nbsp;Response");

    ICQUser *u = gUserManager.FetchUser(item->m_szId, item->m_nPPID, LOCK_R);
    QTextCodec * codec = UserCodec::codecForICQUser(u);
    if (u != NULL)
    {
      if (!u->StatusOffline() && u->ClientInfo() && *u->ClientInfo())
      s += tr("<br><nobr>") + codec->toUnicode(u->ClientInfo()) + tr("</nobr>");
      
      if (u->AutoResponse() && *u->AutoResponse() &&
          item->m_nStatus != ICQ_STATUS_OFFLINE &&
          item->m_nStatus != ICQ_STATUS_ONLINE)
        s += tr("<br><u>Auto Response:</u>") + codec->toUnicode(u->AutoResponse());
      gUserManager.DropUser(u);
    }

    if (*u->GetEmailPrimary() && gMainWindow->m_bPopEmail)
      s += tr("<br><nobr>E: ") + codec->toUnicode(u->GetEmailPrimary()) + tr("</nobr>");

    if (item->m_bPhone && gMainWindow->m_bPopPhone)
      s += tr("<br><nobr>P: ") + codec->toUnicode(u->GetPhoneNumber()) + tr("</nobr>");

    if (item->m_bCellular && gMainWindow->m_bPopCellular)
      s += tr("<br><nobr>C: ") + codec->toUnicode(u->GetCellularNumber()) + tr("</nobr>");

    if ((u->GetFaxNumber()[0]!='\0') && gMainWindow->m_bPopEmail)
      s += tr("<br><nobr>F: ") + codec->toUnicode(u->GetFaxNumber()) + tr("</nobr>");

    if ((u->Ip() || u->IntIp()) && gMainWindow->m_bPopIP) {
      char buf_ip[32];
      char buf_int_ip[32];
      ip_ntoa(u->Ip(), buf_ip);
      ip_ntoa(u->IntIp(), buf_int_ip);
      if (u->Ip() != u->IntIp() && u->IntIp() != 0)
        s += tr("<br><nobr>Ip: ") + buf_ip + "/" + buf_int_ip + tr("</nobr>");
      else
        s += tr("<br><nobr>Ip: ") + buf_ip + tr("</nobr>");
    }

    if ((u->LastOnline()>0) && gMainWindow->m_bPopLastOnline) {
      QDateTime t;
      t.setTime_t(u->LastOnline());
      QString ds = t.toString();
      s += tr("<br><nobr>O: ") +  ds + tr("</nobr>");
    }

    if ((!u->StatusOffline()) && gMainWindow->m_bPopOnlineSince) {
      time_t nLoggedIn = time(0) - u->OnlineSince();
      unsigned long nWeek, nDay, nHour, nMinute;
      nWeek = nLoggedIn / 604800;
      nDay = (nLoggedIn % 604800) / 86400;
      nHour = (nLoggedIn % 86400) / 3600;
      nMinute = (nLoggedIn % 3600) / 60;

      QString ds, temp;
      if (nWeek)
      {
        ds += temp.setNum(nWeek);
        ds += " ";
        ds += (nWeek > 1 ? tr(" weeks") : tr(" week"));
      }
      if (nDay)
      {
        if (nWeek) ds += " ";
        ds += temp.setNum(nDay);
        ds += " ";
        ds += (nDay > 1 ? tr(" days") : tr(" day"));
      }
      if (nHour)
      {
        if (nWeek || nDay) ds += " ";
        ds += temp.setNum(nHour);
        ds += (nHour > 1 ? tr(" hours") : tr(" hour"));
      }
      if (nMinute)
      {
        if (nWeek || nDay || nHour) ds += " ";
        ds += temp.setNum(nMinute);
        ds += (nMinute > 1 ? tr(" minutes") : tr(" minute"));
      }
      if (!nWeek && !nDay && !nHour && !nMinute)
        ds += tr("0 minutes");

      s += tr("<br><nobr>Logged In: ") + ds + tr("</nobr>");
    }

    if (gMainWindow->m_bPopIdleTime)
    {
      if (u->IdleSince())
      {
        char *szTemp;
        szTemp = u->usprintf("%I");
        QString temp(szTemp);
        free(szTemp);
        s += tr("<br><nobr>Idle: ") + temp + tr("</nobr>");
        tip (r, s);
      }
    }
   
    if (gMainWindow->m_bPopID)
    {
      char *szTemp;
      szTemp = u->usprintf("%u");
      QString temp(szTemp);
      free(szTemp);
      s += tr("<br><nobr>ID: ") + temp + tr("</nobr>");
      tip (r, s);
    }

    tip(r, s);
  }
}

void CUserView::setSorting( int column, bool ascending)
{
  gMainWindow->m_nSortColumn = column;
  gMainWindow->m_bSortColumnAscending = ascending;
  QListView::setSorting( column, ascending );
}
