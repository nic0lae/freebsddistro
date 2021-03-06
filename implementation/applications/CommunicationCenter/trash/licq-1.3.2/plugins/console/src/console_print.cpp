#include "console.h"
#include "licq_countrycodes.h"
#include "licq_languagecodes.h"
#include "licq_filetransfer.h"

#include <string.h>
#include <time.h>

//======Utilities============================================================
char *EncodeFileSize(unsigned long nSize)
{
  char szUnit[6];

  if(nSize >= (1024 * 1024))
  {
    nSize /= (1024 * 1024) / 10;
    strcpy(szUnit, "MB");
  }
  else if(nSize >= 1024)
  {
    nSize /= (1024 / 10);
    strcpy(szUnit, "KB");
  }
  else if(nSize != 1)
  {
    nSize *= 10;
    strcpy(szUnit, "Bytes");
  }
  else
  {
    nSize *= 10;
    strcpy(szUnit, "Byte");
  }

  char buf[16];
  sprintf(buf, "%ld.%ld %s", (nSize / 10), (nSize % 10), szUnit);
  return strdup(buf);
}

/*---------------------------------------------------------------------------
 * CLicqConsole::PrintBadInput
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintBadInput(const char *_szIn)
{
  winMain->wprintf("%CInvalid command [%A%s%Z].  Type \"help\" for help.\n",
                   16, A_BOLD, _szIn, A_BOLD);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintBoxTop
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintBoxTop(const char *_szTitle, short _nColor, short _nLength)
{
  unsigned short i, j;
  wattrset(winMain->Win(), COLOR_PAIR(8));
  waddch(winMain->Win(), '\n');
  waddch(winMain->Win(), ACS_ULCORNER);
  for (i = 0; i < 10; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), ACS_RTEE);
  winMain->wprintf("%C %s ", _nColor, _szTitle);
  waddch(winMain->Win(), ACS_LTEE);
  j = _nLength - 16 - strlen(_szTitle);
  for (i = 0; i < j; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), ACS_URCORNER);
  waddch(winMain->Win(), '\n');
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintBoxLeft
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintBoxLeft()
{
  waddch(winMain->Win(), ACS_VLINE);
  waddch(winMain->Win(), ' ');
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintBoxRight
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintBoxRight(short _nLength)
{
  int y, x;
  getyx(winMain->Win(), y, x);
  mvwaddch(winMain->Win(), y, _nLength - 1, ACS_VLINE);
  waddch(winMain->Win(), '\n');
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintBoxBottom
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintBoxBottom(short _nLength)
{
  unsigned short i;
  waddch(winMain->Win(), ACS_LLCORNER);
  for (i = 0; i < _nLength - 2; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), ACS_LRCORNER);
  waddch(winMain->Win(), '\n');

  winMain->RefreshWin();
  wattrset(winMain->Win(), COLOR_PAIR(8));
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintPrompt
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintPrompt()
{
  werase(winPrompt->Win());
  winPrompt->wprintf("%C> ", 56);
  winPrompt->RefreshWin();
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintStatus
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintStatus()
{
  static char szMsgStr[16];
  static char *szLastUser;

  werase(winStatus->Win());

  unsigned short nNumOwnerEvents = 0;
  ICQOwner *o = gUserManager.FetchOwner(LOCK_R);
  if (o)
  {
    nNumOwnerEvents = o->NewMessages();
    gUserManager.DropOwner();
  }

  unsigned short nNumUserEvents = ICQUser::getNumUserEvents() - nNumOwnerEvents;
  if (nNumOwnerEvents > 0)
    sprintf (szMsgStr, "System Message");
  else if (nNumUserEvents > 0)
    sprintf (szMsgStr, "%d Message%c", nNumUserEvents, nNumUserEvents == 1 ? ' ' : 's');
  else
    strcpy(szMsgStr, "No Messages");

  if (winMain->sLastContact.szId != 0)
  {
    ICQUser *u = gUserManager.FetchUser(winMain->sLastContact.szId,
      winMain->sLastContact.nPPID, LOCK_R);
    if (u == NULL)
      szLastUser = strdup("<Removed>");
    else
    {
      szLastUser = strdup(u->GetAlias());
      gUserManager.DropUser(u);
    }
  }
  else
    szLastUser = strdup("<None>");

  o = gUserManager.FetchOwner(LOCK_R);
  wbkgdset(winStatus->Win(), COLOR_PAIR(8));
  mvwhline(winStatus->Win(), 0, 0, ACS_HLINE, COLS);
  //mvwaddch(winStatus->Win(), 0, COLS - USER_WIN_WIDTH - 1, ACS_BTEE);
  wmove(winStatus->Win(), 1, 0);
  wbkgdset(winStatus->Win(), COLOR_PAIR(32));
  if (o)
  {
    winStatus->wprintf("%C%A[ %C%s %C(%C%ld%C) - S: %C%s %C- G: %C%s %C- M: %C%s %C- L: %C%s %C]", 29,
                       A_BOLD, 5,  o->GetAlias(), 29,
                       5, o->Uin(), 29,
                       53, o->StatusStr(), 29,
                       53, CurrentGroupName(), 29,
                       53, szMsgStr, 29, 53,
                       szLastUser, 29);
    gUserManager.DropOwner();
  }
  
  wclrtoeol(winStatus->Win());
  winStatus->RefreshWin();
  free(szLastUser);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintGroups
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintGroups()
{
  unsigned short j = 1, k;

  PrintBoxTop("Groups", 8, 26);

  PrintBoxLeft();
  winMain->wprintf("%A%C%3d. %-19s",
      m_cColorGroupList->nAttr,
      m_cColorGroupList->nColor, 0, "All Users");
  PrintBoxRight(26);
  waddch(winMain->Win(), ACS_LTEE);
  for (k = 0; k < 24; k++) waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), ACS_RTEE);
  waddch(winMain->Win(), '\n');

  GroupList *g = gUserManager.LockGroupList(LOCK_R);
  for (GroupList::iterator i = g->begin(); i != g->end(); i++, j++)
  {
    PrintBoxLeft();
    winMain->wprintf("%A%C%3d. %-19s",
        m_cColorGroupList->nAttr,
        m_cColorGroupList->nColor, j, *i);
    PrintBoxRight(26);
  }
  gUserManager.UnlockGroupList();

  waddch(winMain->Win(), ACS_LTEE);
  for (k = 0; k < 24; k++) waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), ACS_RTEE);
  waddch(winMain->Win(), '\n');

  for (unsigned short i = 1; i <= NUM_GROUPS_SYSTEM; i++)
  {
    PrintBoxLeft();
    winMain->wprintf("%A%C*%2d. %-19s",
        m_cColorGroupList->nAttr,
        m_cColorGroupList->nColor, i, GroupsSystemNames[i]);
    PrintBoxRight(26);
  }

  PrintBoxBottom(26);

}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintVariable
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintVariable(unsigned short nVar)
{
  winMain->wprintf("%s = ", aVariables[nVar].szName);

  switch(aVariables[nVar].nType)
  {
  case INT:
    winMain->wprintf("%d\n", *(int *)aVariables[nVar].pData);
    break;

  case BOOL:
    winMain->wprintf("%s\n", *(bool *)aVariables[nVar].pData == true
        ? "<YES>" : "<NO>");
    break;

  case STRING:
    winMain->wprintf("\"%s\"\n", (char *)aVariables[nVar].pData);
    break;

  case COLOR:
    winMain->wprintf("[%s]\n",
        (*(struct SColorMap **)aVariables[nVar].pData)->szName );
    break;
  }
}


/*---------------------------------------------------------------------------
 * CLicqConsole::CreateUserList
 *-------------------------------------------------------------------------*/
void CLicqConsole::CreateUserList()
{
  unsigned short i = 0;
  char *szTmp = 0;
  struct SUser *s = NULL;
  list <SUser *>::iterator it;

  // Clear the list
  for (it = m_lUsers.begin(); it != m_lUsers.end(); it++)
  {
    if ((*it)->szLine)
      delete [] (*it)->szLine;
    delete (*it);
  }
#undef clear
  m_lUsers.clear();

  FOR_EACH_USER_START(LOCK_R)
  {
    // Only show users on the current group and not on the ignore list
    if (!pUser->GetInGroup(m_nGroupType, m_nCurrentGroup) ||
        (pUser->IgnoreList() && m_nGroupType != GROUPS_SYSTEM && m_nCurrentGroup != GROUP_IGNORE_LIST) )
      FOR_EACH_USER_CONTINUE

    if (!m_bShowOffline && pUser->StatusOffline() )
    {
      FOR_EACH_USER_CONTINUE;
    }

    s = new SUser;
    sprintf(s->szKey, "%05u%010lu", pUser->Status(), pUser->Touched() ^ 0xFFFFFFFF);
    sprintf(s->szId, "%s", pUser->IdString());
    s->nPPID = pUser->PPID();
    s->bOffline = pUser->StatusOffline();

    unsigned short nStatus = pUser->Status();

    if(pUser->StatusInvisible())
    {
      szTmp = pUser->usprintf(m_szOtherOnlineFormat);
      s->color = m_cColorOnline;
    }

    else if(nStatus == ICQ_STATUS_OFFLINE)
    {
      szTmp = pUser->usprintf(m_szOfflineFormat);
      s->color = m_cColorOffline;
    }
    else if(nStatus == ICQ_STATUS_DND || nStatus == ICQ_STATUS_OCCUPIED ||
            nStatus == ICQ_STATUS_NA || nStatus == ICQ_STATUS_AWAY)
    {
      szTmp = pUser->usprintf(m_szAwayFormat);
      s->color = m_cColorAway;
    }
    else if(nStatus == ICQ_STATUS_FREEFORCHAT)
    {
      szTmp = pUser->usprintf(m_szOtherOnlineFormat);
      s->color = m_cColorOnline;
    }
    else if(nStatus == ICQ_STATUS_ONLINE)
    {
      szTmp = pUser->usprintf(m_szOnlineFormat);
      s->color = m_cColorOnline;
    }

    if (pUser->NewUser() && !(m_nGroupType == GROUPS_SYSTEM && m_nCurrentGroup == GROUP_NEW_USERS))
      s->color = m_cColorNew;

    // Create the line to printout now
    if (pUser->NewMessages() > 0)
    {
      s->szLine = new char[strlen(szTmp) + 19];
      snprintf(s->szLine, strlen(szTmp) + 19, "</%d></K>%s<!K><!%d>", s->color->nColor - 6, szTmp ? szTmp : "", s->color->nColor - 6);
      s->szLine[strlen(szTmp) + 18] = '\0';
    } else {
      s->szLine = new char[strlen(szTmp) + 11];
      snprintf(s->szLine, strlen(szTmp) + 11, "</%d>%s<!%d>", s->color->nColor, szTmp ? szTmp : "", s->color->nColor);
      s->szLine[strlen(szTmp) + 10] = '\0';
    }
    free(szTmp);
    
    // Insert into the list
    bool found = false;
    for (it = m_lUsers.begin(); it != m_lUsers.end(); it++)
    {
      if ( strcmp(s->szKey, (*it)->szKey) <= 0)
      {
        m_lUsers.insert(it, s);
        found = true;
        break;
      }
    }
    if (!found)
      m_lUsers.push_back(s);

    i++;
  }
  FOR_EACH_USER_END

}

/*---------------------------------------------------------------------------
 * CLicqConsole::PrintUsers
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintUsers()
{
  char *title = "<C></B/40>Contacts";
  char *ulist[1024];
  int i = 0;
  struct SScrollUser *s = NULL;
  
  werase(winUsers->Win());
  m_lScrollUsers.clear();
  for (list<SUser *>::iterator it = m_lUsers.begin();
       it != m_lUsers.end();
       it++)
  {
    s = new SScrollUser;
    s->pos = i;
    s->nPPID = (*it)->nPPID;
        s->color = (*it)->color;
    sprintf(s->szId, "%s", (*it)->szId);

    m_lScrollUsers.push_back(s);
    ulist[i++] = copyChar((*it)->szLine);
  }

  if (!cdkUserList)
  {
    cdkUserList = newCDKScroll(winUsers->CDKScreen(), 0, 0, 
                               RIGHT, LINES - 5, USER_WIN_WIDTH, title,
                               ulist, i, FALSE, A_NORMAL, TRUE, TRUE);
  }
  else
  {
    setCDKScrollItems (cdkUserList, ulist, i, FALSE);
  }
  
  bindCDKObject(vSCROLL, cdkUserList, SPACE, CLicqConsole::MenuPopupWrapper, this);
  bindCDKObject(vSCROLL, cdkUserList, KEY_UP, CLicqConsole::UserListCallback, this);
  bindCDKObject(vSCROLL, cdkUserList, KEY_DOWN, CLicqConsole::UserListCallback, this);
  UserListHighlight(A_NORMAL);
  drawCDKScroll(cdkUserList, true);
  winBar->RefreshWin();
  winUsers->RefreshWin();
}

/*---------------------------------------------------------------------------
 * CLicqConsole::UserListHighlight
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserListHighlight(chtype type, chtype input)
{
  // There has got to be a better way to do this...
  list <SScrollUser *>::iterator it;
  int down;
  if (input == KEY_DOWN)
    down = 1;
  else if (input == KEY_UP)
    down = -1;
  else
    down = 0;
  for (it = m_lScrollUsers.begin(); it != m_lScrollUsers.end(); it++)
  {
    if ((*it)->pos == (cdkUserList->currentItem + down))
    {
      ICQUser *u = gUserManager.FetchUser((*it)->szId, (*it)->nPPID, LOCK_R);
      if (u && u->NewMessages())
        setCDKScrollHighlight(cdkUserList, COLOR_PAIR((*it)->color->nColor - 6) | type);
      else
        setCDKScrollHighlight(cdkUserList, COLOR_PAIR((*it)->color->nColor) | type);
      gUserManager.DropUser(u);
      break;
    }
  }
}

/*---------------------------------------------------------------------------
 * CLicqConsole::UserListCallback
 *-------------------------------------------------------------------------*/
int CLicqConsole::UserListCallback(EObjectType cdktype, void *object, void *clientData, chtype input)
{
  CLicqConsole *me = (CLicqConsole *)clientData; 
  me->UserListHighlight(A_REVERSE, input);
  return 0;
}

/*---------------------------------------------------------------------------
 * CLicqConsole::PrintContactPopup
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintContactPopup(char *_szAlias)
{
  char title[256];
  snprintf(title, 256, "<C></B/40>%s", _szAlias);
  char *list[10000];
  int i = 0;

  list[i++] = "Message";
  list[i++] = "View Event";

  cdkContactPopup = newCDKScroll(winMain->CDKScreen(), 0, 0,
                                 RIGHT, 10, 20, title,
                                 list, i, FALSE, A_REVERSE, TRUE, TRUE);

  setCDKScrollBackgroundColor(cdkContactPopup, "</40>");
  drawCDKScroll(cdkContactPopup, true);
  winMain->RefreshWin();
}

/*---------------------------------------------------------------------------
 * CLicqConsole::PrintHelp
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintHelp()
{
  PrintBoxTop("Menu", 8, 48);

  for (unsigned short i = 0; i < NUM_COMMANDS; i++)
  {
    waddch(winMain->Win(), ACS_VLINE);
    winMain->wprintf(aCommands[i].szHelp, m_szCommandChar[0]);
    PrintBoxRight(48);
  }

  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf(" %BF10%b to activate the contact list");
  PrintBoxRight(48);

  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf(" %BF(1-%d)%b to change between consoles", MAX_CON);
  PrintBoxRight(48);
  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf(" %BF%d%b to see the log", MAX_CON + 1);
  PrintBoxRight(48);

  waddch(winMain->Win(), ACS_VLINE);
  PrintBoxRight(48);
  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf(" %B<user>%b can be alias, uin,");
  PrintBoxRight(48);
  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf("   $ (last user) or # (owner)");
  PrintBoxRight(48);

  waddch(winMain->Win(), ACS_VLINE);
  PrintBoxRight(48);
  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf(" To end text use \".\" (accept),");
  PrintBoxRight(48);
  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf("   \".d/s\" (force direct/server),");
  PrintBoxRight(48);
  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf("   \".u\" (urgent), or \",\" (abort)");
  PrintBoxRight(48);

  PrintBoxBottom(48);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintHistory
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintHistory(HistoryList &lHistory, unsigned short nStart,
                                unsigned short nEnd, const char *szFrom)
{
  HistoryListIter it = lHistory.begin();
  unsigned short n = 0;
  for (n = 0; n < nStart && it != lHistory.end(); n++, it++);
  while (n <= nEnd && it != lHistory.end())
  {
    wattron(winMain->Win(), A_BOLD);
    for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
      waddch(winMain->Win(), ACS_HLINE);
    waddch(winMain->Win(), '\n');
    time_t t = (*it)->Time();
    char *szTime = ctime(&t);
    szTime[16] = '\0';
    winMain->wprintf("%A%C[%d of %d] %s %s %s (%s) [%c%c%c]:\n%Z%s\n", A_BOLD,
                     8, n + 1, lHistory.size(), (*it)->Description(),
                     (*it)->Direction() == D_RECEIVER ? "from" : "to", szFrom,
                     szTime, (*it)->IsDirect() ? 'D' : '-',
                     (*it)->IsMultiRec() ? 'M' : '-', (*it)->IsUrgent() ? 'U' : '-',
                     A_BOLD, (*it)->Text());

    n++;
    it++;
  }
  wattron(winMain->Win(), A_BOLD);
  for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), '\n');
  winMain->RefreshWin();
  wattroff(winMain->Win(), A_BOLD);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintInfo_General
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintInfo_General(const char *szId, unsigned long nPPID)
{
  // Print the users info to the main window
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  if (u == NULL) return;

  // Some IP, Real IP and last seen stuff
  char buf[32];
  char szPort[32];
  char szRealIp[32];
  const unsigned long nRealIp = u->RealIp();
  strcpy(szRealIp, inet_ntoa_r(*(struct in_addr *)&nRealIp, buf));
  time_t nLast = u->LastOnline();
  time_t nOnSince = u->OnlineSince();

  wattron(winMain->Win(), A_BOLD);
  for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), '\n');
  wattroff(winMain->Win(), A_BOLD);

  winMain->wprintf("%s %A(%Z%ld%A) General Info - %Z%s\n", u->GetAlias(), A_BOLD,
                   A_BOLD, u->Uin(), A_BOLD, A_BOLD, u->StatusStr());

  winMain->wprintf("%C%AName: %Z%s %s\n", 8, A_BOLD, A_BOLD,
                   u->GetFirstName(), u->GetLastName());
  winMain->wprintf("%C%AIp: %Z%s:%s\n", 8, A_BOLD, A_BOLD,
                   u->IpStr(buf), u->PortStr(szPort));
  winMain->wprintf("%C%AReal Ip: %Z%s\n", 8, A_BOLD, A_BOLD,
                   szRealIp);
  winMain->wprintf("%C%AEmail 1: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetEmailPrimary());
  winMain->wprintf("%C%AEmail 2: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetEmailSecondary());
  winMain->wprintf("%C%ACity: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCity());
  winMain->wprintf("%C%AState: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetState());
  winMain->wprintf("%C%AAddress: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetAddress());
  winMain->wprintf("%C%APhone Number: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetPhoneNumber());
  winMain->wprintf("%C%AFax Number: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetFaxNumber());
  winMain->wprintf("%C%ACellular Number: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCellularNumber());
  winMain->wprintf("%C%AZipcode: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetZipCode());
  winMain->wprintf("%C%ACountry: ", 8, A_BOLD);
  if (u->GetCountryCode() == COUNTRY_UNSPECIFIED)
    winMain->wprintf("%CUnspecified\n", 8);
  else
  {
    const SCountry *c = GetCountryByCode(u->GetCountryCode());
    if (c == NULL)
      winMain->wprintf("%CUnknown (%d)\n", 8, u->GetCountryCode());
    else  // known
      winMain->wprintf("%C%s\n", 8, c->szName);
  }
  winMain->wprintf("%C%ATimezone: %ZGMT%c%02d%s\n", 8, A_BOLD, A_BOLD,
                   u->GetTimezone() > 0 ? '-' : '+',
                   u->GetTimezone() / 2,
                   u->GetTimezone() % 2 ? "30" : "00");
  winMain->wprintf("%C%ALast Seen: %Z%s", 8, A_BOLD, A_BOLD,
    ctime(&nLast));
  if (!u->StatusOffline())
  {
    winMain->wprintf("%C%AOnline Since: %Z%s", 8, A_BOLD, A_BOLD,
      (nOnSince ? ctime(&nOnSince) : "Unknown"));
  }

  wattron(winMain->Win(), A_BOLD);
  for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), '\n');
  winMain->RefreshWin();
  wattroff(winMain->Win(), A_BOLD);

  gUserManager.DropUser(u);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintInfo_More
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintInfo_More(const char *szId, unsigned long nPPID)
{
  // Print the users info to the main window
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  if (u == NULL) return;

  wattron(winMain->Win(), A_BOLD);
  for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), '\n');
  wattroff(winMain->Win(), A_BOLD);

  winMain->wprintf("%s %A(%Z%ld%A) More Info - %Z%s\n", u->GetAlias(), A_BOLD,
                   A_BOLD, u->Uin(), A_BOLD, A_BOLD, u->StatusStr());

  if (u->GetAge() == AGE_UNSPECIFIED)
    winMain->wprintf("%C%AAge: %ZUnspecified\n", 8, A_BOLD, A_BOLD);
  else
    winMain->wprintf("%C%AAge: %Z%d\n", 8, A_BOLD, A_BOLD, u->GetAge());
  winMain->wprintf("%C%AGender: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetGender() == GENDER_MALE ? "Male" : u->GetGender() == GENDER_FEMALE ? "Female" : "Unspecified");
  winMain->wprintf("%C%AHomepage: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetHomepage());
  winMain->wprintf("%C%ABirthday: %Z%d/%d/%d\n", 8, A_BOLD, A_BOLD, u->GetBirthDay(), u->GetBirthMonth(), u->GetBirthYear());
  for (unsigned short i = 0; i < 3; i++)
  {
    winMain->wprintf("%C%ALanguage %d: ", 8, A_BOLD, i + 1);
    const SLanguage *l = GetLanguageByCode(u->GetLanguage(i));
    if (l == NULL)
      winMain->wprintf("%CUnknown (%d)\n", 8, u->GetLanguage(i));
    else  // known
      winMain->wprintf("%C%s\n", 8, l->szName);
  }

  gUserManager.DropUser(u);

  wattron(winMain->Win(), A_BOLD);
  for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), '\n');
  winMain->RefreshWin();
  wattroff(winMain->Win(), A_BOLD);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::PrintInfo_Work
 *-------------------------------------------------------------------------*/
void CLicqConsole::PrintInfo_Work(const char *szId, unsigned long nPPID)
{
  // Print the users info to the main window
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  if (u == NULL) return;

  wattron(winMain->Win(), A_BOLD);
  for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), '\n');
  wattroff(winMain->Win(), A_BOLD);

  winMain->wprintf("%s %A(%Z%ld%A) Work Info - %Z%s\n", u->GetAlias(), A_BOLD,
                   A_BOLD, u->Uin(), A_BOLD, A_BOLD, u->StatusStr());

  winMain->wprintf("%C%ACompany Name: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCompanyName());
  winMain->wprintf("%C%ACompany Department: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCompanyDepartment());
  winMain->wprintf("%C%ACompany Position: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCompanyPosition());
  winMain->wprintf("%C%ACompany Phone Number: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCompanyPhoneNumber());
  winMain->wprintf("%C%ACompany Fax Number: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCompanyFaxNumber());
  winMain->wprintf("%C%ACompany City: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCompanyCity());
  winMain->wprintf("%C%ACompany State: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCompanyState());
  winMain->wprintf("%C%ACompany Address: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCompanyAddress());
  winMain->wprintf("%C%ACompany Zip Code: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCompanyZip());
  winMain->wprintf("%C%ACompany Country: ", 8, A_BOLD);
  if (u->GetCountryCode() == COUNTRY_UNSPECIFIED)
    winMain->wprintf("%CUnspecified\n", 8);
  else
  {
    const SCountry *c = GetCountryByCode(u->GetCountryCode());
    if (c == NULL)
      winMain->wprintf("%CUnknown (%d)\n", 8, u->GetCountryCode());
    else  // known
      winMain->wprintf("%C%s\n", 8, c->szName);
  }
  winMain->wprintf("%C%ACompany Homepage: %Z%s\n", 8, A_BOLD, A_BOLD, u->GetCompanyHomepage());

  gUserManager.DropUser(u);

  wattron(winMain->Win(), A_BOLD);
  for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), '\n');
  winMain->RefreshWin();
  wattroff(winMain->Win(), A_BOLD);
}

/*----------------------------------------------------------------------------
 * CLicqConsole::PrintInfo_About
 *--------------------------------------------------------------------------*/
void CLicqConsole::PrintInfo_About(const char *szId, unsigned long nPPID)
{
  // Print the user's about info to the main window
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  if (u == NULL)  return;

  wattron(winMain->Win(), A_BOLD);
  for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), '\n');
  wattroff(winMain->Win(), A_BOLD);

  winMain->wprintf("%s %A(%Z%ld%A) About Info - %Z%s\n", u->GetAlias(), A_BOLD,
                    A_BOLD, u->Uin(), A_BOLD, A_BOLD, u->StatusStr());

  winMain->wprintf("%s\n", u->GetAbout());

  gUserManager.DropUser(u);

  wattron(winMain->Win(), A_BOLD);
  for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
    waddch(winMain->Win(), ACS_HLINE);
  waddch(winMain->Win(), '\n');
  winMain->RefreshWin();
  wattroff(winMain->Win(), A_BOLD); 
}

/*----------------------------------------------------------------------------
 * CLicqConsole::PrintFileStat
 *--------------------------------------------------------------------------*/
void CLicqConsole::PrintFileStat(CFileTransferManager *ftman)
{
  // Get the user's name
  ICQUser *u = gUserManager.FetchUser(ftman->Uin(), LOCK_R);
  const char *szAlias = u->GetAlias();
  gUserManager.DropUser(u);

  // Make the title
  char szTitle[30];
  ftman->Direction() == D_RECEIVER ? strcpy(szTitle, "File from ") :
    strcpy(szTitle, "File to ");
  strcat(szTitle, szAlias);

  // Current file name and Current File # slash Total Batch Files
  PrintBoxTop(szTitle, 8, 48);
  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf("%ACurrent File: %Z", A_BOLD, A_BOLD);
  winMain->wprintf(const_cast<char *>(ftman->FileName()));
  PrintBoxRight(48);
 
  // Current progress, current file xferred slash total current file size
  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf("%ACurrent Progress: %Z", A_BOLD, A_BOLD);
  unsigned long fCurPercent = (100 * ftman->FilePos()) / ftman->FileSize();
  winMain->wprintf("%02ld Percent", fCurPercent);
  PrintBoxRight(48);

  // Batch progress, current batch xferred slash total batch size
  waddch(winMain->Win(), ACS_VLINE);
  winMain->wprintf("%ABatch Progress: %Z", A_BOLD, A_BOLD);
  unsigned long fBatchPercent = (100 * ftman->BatchPos()) / ftman->BatchSize();
  winMain->wprintf("%02ld Percent", fBatchPercent);
  PrintBoxRight(48);

  // Time, ETA, BPS
  waddch(winMain->Win(), ACS_VLINE);
  time_t Time = time(NULL) - ftman->StartTime();
  winMain->wprintf("%ATime: %Z%02ld:%02ld:%02ld   ", A_BOLD, A_BOLD,
    Time / 3600, (Time % 3600)/ 60, (Time % 60));

  if(ftman->BytesTransfered() == 0 || Time == 0)
  {
    winMain->wprintf("%AETA: %Z--:--:--:   ", A_BOLD, A_BOLD);
    winMain->wprintf("%ABPS: %Z---", A_BOLD, A_BOLD);
  }
  else
  {
    unsigned long nBytesLeft = ftman->FileSize() - ftman->FilePos();
    time_t nETA = (time_t)(nBytesLeft / (time_t)(ftman->BytesTransfered() / Time));
    winMain->wprintf("%AETA: %Z%02ld:%02ld:%02ld   ", A_BOLD, A_BOLD,
      nETA / 3600, (nETA % 3600)/60, (nETA % 60));
    winMain->wprintf("%ABPS: %Z%s", A_BOLD, A_BOLD, EncodeFileSize(
      ftman->BytesTransfered() / Time));
  }

  PrintBoxRight(48);

  // Close this box
  PrintBoxBottom(48);
  winMain->RefreshWin();
}


/*----------------------------------------------------------------------------
 * CLicqConsole::PrintMacros
 *--------------------------------------------------------------------------*/
void CLicqConsole::PrintMacros()
{
  MacroList::iterator iter;

  PrintBoxTop("Macros", 8, 40);

  for (iter = listMacros.begin(); iter != listMacros.end(); iter++)
  {
    PrintBoxLeft();
    winMain->wprintf("%A%C%-10s %Z->%A %-19s",
        A_BOLD,
        8,
        (*iter)->szMacro, A_BOLD, A_BOLD, (*iter)->szCommand);
    PrintBoxRight(40);
  }

  PrintBoxBottom(40);
}


