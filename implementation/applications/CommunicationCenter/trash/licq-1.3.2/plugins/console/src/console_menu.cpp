#include "console.h"
#include "event_data.h"

#include <ctype.h>

const unsigned short NUM_COMMANDS = 24;
const struct SCommand aCommands[NUM_COMMANDS] =
{
  { "contacts", &CLicqConsole::MenuContactList, NULL,
    " %B%cco%bntacts",
    "Force a refresh of the contact list." },
  { "console", &CLicqConsole::MenuSwitchConsole, NULL,
    " %B%ccons%bole <num>",
    "Switch to a console." },
  { "group", &CLicqConsole::MenuGroup, NULL,
    " %B%cg%broup [ %B#%b ]",
    "Prints the group list or changes to the given group number." },
  { "clear", &CLicqConsole::MenuClear, NULL,
    " %B%ccl%bear",
    "Clears the current window." },
  { "add", &CLicqConsole::MenuAdd, NULL,
    " %B%cad%bd %B<uin>%b [ alert ]",
    "Add a user to your list by uin." },
  { "authorize", &CLicqConsole::MenuAuthorize, NULL,
    " %B%cau%bthorize <grant | refuse> %B<uin>",
    "Authorize grant or refuse  the given user." },
  { "history", &CLicqConsole::MenuHistory, &CLicqConsole::TabUser,
    " %B%chi%bstory %B<user>[.protocol]%b [ %B#%b,%B#%b ]",
    "Print the given range of events from the history.\n"
    "'$' represents the last message, and +/- can be used to specify "
    "an offset.  For example \"history $-5,$\" will print from the "
    "fifth-to-last event to the end.\n"
    "If only the start range is specified then one event will be printed.\n"
    "If the start range begins with +/- then the index of the last message "
    "printed will be modified by the operation.  If the end range begins "
    "with +/- then it will modify the start range value.\n"
    "To cycle through the last 10 events in the history\n"
    "try the following:\n"
    "user <> \"history $ - 10\"\n"
    "last \"history +1\"\n"
    "last \"history +1\"\n"
    "...\n" },
  { "message", &CLicqConsole::MenuMessage, &CLicqConsole::TabUser,
    " %B%cm%bessage %B<user>[.protocol]%b",
    "Send a message to a user." },
  { "url", &CLicqConsole::MenuUrl, &CLicqConsole::TabUser,
    " %B%cur%bl %B<user>%b",
    "Send a URL to a user." },
  { "file", &CLicqConsole::MenuFile, &CLicqConsole::TabUser,
    " %B%cf%bile [ %B<user>%b ]",
    "Send a file to a user or display file transfer stats." },
  { "sms", &CLicqConsole::MenuSms, &CLicqConsole::TabUser,
    " %B%csms%b %B<user>%b",
    " Send an SMS to a user."},
  { "info", &CLicqConsole::MenuInfo, &CLicqConsole::TabUser,
    " %B%ci%bnfo %B<user>%b",
    "Display user information." },
  { "view", &CLicqConsole::MenuView, &CLicqConsole::TabUser,
    " %B%cv%biew [ %B<user>[.protocol]%b ]",
    "View an incoming event." },
  { "secure", &CLicqConsole::MenuSecure, &CLicqConsole::TabUser,
    " %B%cs%becure %B<user>%b [ open | close ]",
    "Establish a secure connection to a user." },
  { "auto-response", &CLicqConsole::MenuAutoResponse, &CLicqConsole::TabUser,
    " %B%ca%buto-response [ %B<user>%b ]",
    "View a user's auto-reponse or set your own (use #)." },
  { "remove", &CLicqConsole::MenuRemove, &CLicqConsole::TabUser,
    " %B%cr%bemove %B<user>[.protocol]%b",
    "Remove a user from your contact list." },
  { "status", &CLicqConsole::MenuStatus, &CLicqConsole::TabStatus,
    " %B%cst%batus [*]<online | away | na | dnd | occupied | ffc | offline>",
    "Set your status, prefix with \"*\" for invisible mode." },
  { "search", &CLicqConsole::MenuSearch, NULL,
    " %B%csea%brch",
    "Perform a search of the ICQ network." },
  { "uins", &CLicqConsole::MenuUins, NULL,
    " %B%cui%bns",
    "Print out the uins of the users in the current group.\n"
    "Useful if the user has odd characters in their alias." },
  { "set", &CLicqConsole::MenuSet, &CLicqConsole::TabSet,
    " %B%cset%b [ %B<variable>%b [ %B<value>%b ] ]",
    "Allows the setting and viewing of options.  With no arguments\n"
    "will print all current set'able values.  With one argument will\n"
    "print the value of the given argument.\n"
    "A boolean value can be yes/true/on or no/false/off.\n"
    "Color values can be red/blue/green/cyan/magenta/white/yellow\n"
    "or bright_<color> for bright colors." },
  { "plugins", &CLicqConsole::MenuPlugins, NULL,
    " %B%cp%blugins",
    "List the currently loaded plugins." },
  { "define", &CLicqConsole::MenuDefine, NULL,
    " %B%cd%befine [ %B<macro>%b [ %B<command>%b ] ]",
    "Define a new macro, enter macros by not using '/'.\n"
    "A macro can be any string of characters not containing\n"
    "a space.  The command can be any valid command, do not\n"
    "prepend the command character when defining.\n"
    "Example: \"/define r message $\" creates a macro \"r\"\n"
    "which replies to the last user you talked to."},
  { "help", &CLicqConsole::MenuHelp, NULL,
    " %B%che%blp [ %B<command>%b ]",
    "This help screen, can also be passed a command for detailed\n"
    "information about it." },
  { "quit", &CLicqConsole::MenuQuit, NULL,
    " %B%cq%buit",
    "Quit Licq." }
};

/*---------------------------------------------------------------------------
 * CLicqConsole::MenuPopupWrapper
 *
 * Callback function for when SPACE is pressed on the cdkUserList
 *-------------------------------------------------------------------------*/
int CLicqConsole::MenuPopupWrapper(EObjectType cdktype, void *object, void *clientData, chtype key)
{
  CLicqConsole *me = (CLicqConsole *)clientData;
  me->MenuPopup(me->cdkUserList->currentItem);
  return 1;
}

/*---------------------------------------------------------------------------
 * CLicqConsole::MenuPopup
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuPopup(int userSelected) {
  list <SScrollUser *>::iterator it;
  for (it = m_lScrollUsers.begin(); it != m_lScrollUsers.end(); it++)
  {
    if ((*it)->pos == userSelected)
    {
      int choice;
      ICQUser *u = gUserManager.FetchUser((*it)->szId, (*it)->nPPID, LOCK_R);
      if (u == NULL) return;

      PrintContactPopup(u->GetAlias());
      gUserManager.DropUser(u);
      nl();
      choice = activateCDKScroll(cdkContactPopup, NULL);
      eraseCDKScroll(cdkContactPopup);
      destroyCDKScroll(cdkContactPopup);
      winMain->RefreshWin();
      if (cdkContactPopup->exitType == vNORMAL)
      {
        nonl();
        switch (choice)
        {
          case 0:
            UserCommand_Msg((*it)->szId, (*it)->nPPID, NULL);
            break;
          case 1:
            UserCommand_View((*it)->szId, (*it)->nPPID, NULL);
            break;
        }
      }
      SaveLastUser((*it)->szId, (*it)->nPPID);
      break;
    }
  }
}

/*---------------------------------------------------------------------------
 * CLicqConsole::MenuSwitchConsole
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuSwitchConsole(char *_szArg)
{
  int i;
  if (!_szArg)
    return;
  i = atoi(_szArg);
  if (i <= 0 || i > 9)
    winMain->wprintf("Invalid console number - valid numbers are 1-9\n");
  else
    if (i == 9)
      SwitchToCon(0); 
    else
      SwitchToCon(i);
}

/*---------------------------------------------------------------------------
 * CLicqConsole::MenuList
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuList(char *_szArg)
{
  int userSelected;
  UserListHighlight(A_REVERSE);
  nl();
  userSelected = activateCDKScroll(cdkUserList, NULL);
  nonl();
  UserListHighlight(A_NORMAL);
  drawCDKScroll(cdkUserList, TRUE);
  if (cdkUserList->exitType == vNORMAL)
  {
    list <SScrollUser *>::iterator it;
    for (it = m_lScrollUsers.begin(); it != m_lScrollUsers.end(); it++)
    {
      if ((*it)->pos == userSelected)
      {
        ICQUser *u = gUserManager.FetchUser((*it)->szId, (*it)->nPPID, LOCK_R);
        if (u == NULL) return;
        if (u->NewMessages() > 0)
        {
          gUserManager.DropUser(u);
          UserCommand_View((*it)->szId, (*it)->nPPID, NULL);
        }
        else
        {
          gUserManager.DropUser(u);
          UserCommand_Msg((*it)->szId, (*it)->nPPID, NULL);
        }
        SaveLastUser((*it)->szId, (*it)->nPPID);
        break;
      }
    }
  }
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuHelp
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuHelp(char *_szArg)
{
  if (_szArg == NULL)
  {
    PrintHelp();
    return;
  }

  // Print help on a specific topic
  unsigned short i;
  for (i = 0; i < NUM_COMMANDS; i++)
  {
    if (strncasecmp(_szArg, aCommands[i].szName, strlen(_szArg)) == 0)
      break;
  }
  if (i == NUM_COMMANDS)
  {
    PrintBadInput(_szArg);
    return;
  }
  winMain->wprintf("%AHelp on \"%Z%s%A\":%Z\n%s\n", A_BOLD, A_BOLD,
   aCommands[i].szName, A_BOLD, A_BOLD, aCommands[i].szDescription);

}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuQuit
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuQuit(char *)
{
  licqDaemon->Shutdown();
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuPlugins
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuPlugins(char *_szArg)
{
  PluginsList l;
  PluginsListIter it;
  licqDaemon->PluginList(l);
  ProtoPluginsList p1;
  ProtoPluginsListIter pit;
  licqDaemon->ProtoPluginList(p1);
  PrintBoxTop("Plugins", 40, 70);
  for (it = l.begin(); it != l.end(); it++)
  {
    PrintBoxLeft();
    winMain->wprintf("[%3d] %s v%s (%s %s) - %s", (*it)->Id(), (*it)->Name(),
                     (*it)->Version(), (*it)->BuildDate(),
                     (*it)->BuildTime(), (*it)->Status());
    PrintBoxRight(70);
  }
  for (pit = p1.begin(); pit != p1.end(); pit++)
  {
    PrintBoxLeft();
    winMain->wprintf("[%3d] %s v%s", (*pit)->Id(), (*pit)->Name(),
             (*pit)->Version());
    PrintBoxRight(70);
  }
  PrintBoxBottom(70);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuDefine
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuDefine(char *szArg)
{
  MacroList::iterator iter;
  char *szCmd = szArg;

  if (szArg == NULL)
  {
    PrintMacros();
    return;
  }

  while (*szCmd != '\0' && *szCmd != ' ') szCmd++;

  // Check if we are undefining a macro
  if (*szCmd == '\0')
  {
    for (iter = listMacros.begin(); iter != listMacros.end(); iter++)
    {
      if (strcmp((*iter)->szMacro, szArg) == 0)
      {
        winMain->wprintf("%C%AErased macro \"%s -> %s\"\n",
         m_cColorInfo->nColor, m_cColorInfo->nAttr,
         (*iter)->szMacro, (*iter)->szCommand);
        delete *iter;
#undef erase
        listMacros.erase(iter);
        DoneOptions();
        return;
      }
    }
    winMain->wprintf("%CNo such macro \"%A%s%Z\"\n", 16, A_BOLD,
      szArg, A_BOLD);
    return;
  }

  *szCmd++ = '\0';
  while (*szCmd == ' ') szCmd++;

  // See if this is a double macro definition
  for (iter = listMacros.begin(); iter != listMacros.end(); iter++)
  {
    if (strcmp((*iter)->szMacro, szArg) == 0)
    {
      delete *iter;
#undef erase
      listMacros.erase(iter);
      break;
    }
  }

  // Set the macro
  SMacro *macro = new SMacro;
  strcpy(macro->szMacro, szArg);
  strcpy(macro->szCommand, szCmd);
  listMacros.push_back(macro);

  winMain->wprintf("%A%CAdded macro \"%s -> %s\"\n",
   m_cColorInfo->nAttr, m_cColorInfo->nColor,
   macro->szMacro, macro->szCommand);

  DoneOptions();
}



/*---------------------------------------------------------------------------
 * CLicqConsole::MenuGroup
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuGroup(char *_szArg)
{
  if (_szArg == NULL)
  {
    PrintGroups();
    return;
  }

  GroupType nGroupType;
  unsigned short nCurrentGroup;

  // Try to change groups
  if (_szArg[0] == '*')
  {
    _szArg++;
    nGroupType = GROUPS_SYSTEM;
    nCurrentGroup = atol(_szArg);

    if (nCurrentGroup > NUM_GROUPS_SYSTEM || nCurrentGroup == 0)
    {
      winMain->wprintf("%CInvalid group number (0 - %d)\n", 16,
                       NUM_GROUPS_SYSTEM);
      return;
    }
    m_nCurrentGroup = nCurrentGroup;
    m_nGroupType = nGroupType;
    winMain->wprintf("%C%ASwitching to group *%d (%s).\n",
                     m_cColorInfo->nColor, m_cColorInfo->nAttr,
                     m_nCurrentGroup,
                     GroupsSystemNames[m_nCurrentGroup]);
  }
  else
  {
    nGroupType = GROUPS_USER;
    nCurrentGroup = atol(_szArg);

    if (nCurrentGroup > gUserManager.NumGroups())
    {
      winMain->wprintf("%CInvalid group number (0 - %d)\n", 16,
                       gUserManager.NumGroups());
      return;
    }
    m_nCurrentGroup = nCurrentGroup;
    m_nGroupType = nGroupType;
    GroupList *g = gUserManager.LockGroupList(LOCK_R);
    winMain->wprintf("%C%ASwitching to group %d (%s).\n",
                     m_cColorInfo->nColor, m_cColorInfo->nAttr,
                     m_nCurrentGroup,
                     m_nCurrentGroup == 0 ? "All Users" : (*g)[m_nCurrentGroup - 1]);
    gUserManager.UnlockGroupList();
  }

  PrintStatus();
  CreateUserList();
  PrintUsers();
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuAdd
 *-------------------------------------------------------------------------*/
//TODO: fix this for other protocols
void CLicqConsole::MenuAdd(char *szArg)
{
  if (szArg == NULL)
  {
    winMain->wprintf("%CSpecify a UIN to add.\n", 16);
    return;
  }

  // Try to change groups
  bool bAlert = false;

  while (*szArg != '\0' && *szArg != ' ') szArg++;
  if (*szArg == ' ')
  {
    while (*szArg == ' ') szArg++;
    if (strcasecmp(szArg, "alert") == 0)
      bAlert = true;
  }

  if (!licqDaemon->AddUserToList(szArg, LICQ_PPID))
  {
    winMain->wprintf("%CAdding user %s failed (duplicate user or invalid uin).\n",
     16, szArg);
    return;
  }

  winMain->wprintf("%C%AAdded user %s.\n",
                     m_cColorInfo->nColor, m_cColorInfo->nAttr,
                     szArg);

  if (bAlert)
  {
    int nUin = atol(szArg);
    licqDaemon->icqAlertUser(nUin);
    winMain->wprintf("%C%AAlerted user %ld they were added.\n",
                     m_cColorInfo->nColor, m_cColorInfo->nAttr,
                     nUin);
  }

}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuAuthorize
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuAuthorize(char *szArg)
{
  if (szArg == NULL)
  {
    winMain->wprintf("%CSpecify \"grant/refuse\" and a UIN/Screen Name to authorize.\n", 16);
    return;
  }

  bool bGrant = true;

  if (strncasecmp(szArg, "grant", 5) == 0)
  {
    bGrant = true;
    szArg += 5;
  }
  else if (strncasecmp(szArg, "refuse", 6) == 0)
  {
    bGrant = false;
    szArg += 6;
  }

  // Get the input now
  winMain->fProcessInput = &CLicqConsole::InputAuthorize;
  winMain->state = STATE_MLE;
  DataMsg *data = new DataMsg(szArg, LICQ_PPID);
  data->bUrgent = bGrant;
  winMain->data = data;

  winMain->wprintf("%A%CEnter authorization message:\n",
                   m_cColorQuery->nAttr, m_cColorQuery->nColor);

  return;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuStatus
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuStatus(char *_szArg)
{
  unsigned short nStatus = ICQ_STATUS_ONLINE, i;
  bool bInvisible = false;

  if (_szArg == NULL)
  {
    winMain->wprintf("%CSpecify status.\n", 16);
    return;
  }

  // Check if we are going invisible or not
  if (_szArg[0] == '*')
  {
    bInvisible = true;
    //_szArg++;
  }
  // Find the status
  for (i = 0; i < NUM_STATUS; i++)
  {
    if (strcasecmp(_szArg, aStatus[i].szName) == 0)
    {
      nStatus = aStatus[i].nId;
      break;
    }
  }
  // Check that we found it
  if (i == NUM_STATUS)
  {
    winMain->wprintf("%CInvalid status: %A%s\n", 16, A_BOLD, _szArg);
    return;
  }

  //set same status for all protocols for now
  ProtoPluginsList p1;
  ProtoPluginsListIter it;
  licqDaemon->ProtoPluginList(p1);
  for (it = p1.begin(); it != p1.end(); it++)
  {
    unsigned long nPPID = (*it)->PPID();
    ICQOwner *o = gUserManager.FetchOwner(nPPID, LOCK_R);
    if (nStatus == ICQ_STATUS_OFFLINE)
    {
      gUserManager.DropOwner(nPPID);
      licqDaemon->ProtoLogoff(nPPID);
      continue;
    }
    if (bInvisible)
      nStatus |= ICQ_STATUS_FxPRIVATE;

    // call the right function
    bool b = o->StatusOffline();
    gUserManager.DropOwner(nPPID);
    if (b)
    {
       licqDaemon->ProtoLogon(nPPID, nStatus);
    }
    else
    {
       licqDaemon->ProtoSetStatus(nPPID, nStatus);
    }
  }
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuContactList
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuContactList(char *)
{
  CreateUserList();
  PrintUsers();
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuUins
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuUins(char *)
{
  list <SUser *>::iterator it;
  ICQUser *u;

  for (it = m_lUsers.begin(); it != m_lUsers.end(); it++)
  {
    u = gUserManager.FetchUser((*it)->szId, (*it)->nPPID, LOCK_R);
    winMain->wprintf("%s %A-%Z %s\n", u->GetAlias(), A_BOLD, A_BOLD, u->IdString());
    gUserManager.DropUser(u);
  }

}

/*---------------------------------------------------------------------------
 * CLicqConsole::GetUinFromArg
 *-------------------------------------------------------------------------*/
unsigned long CLicqConsole::GetUinFromArg(char **p_szArg)
{
  char *szAlias, *szCmd;
  unsigned long nUin = 0;
  bool bCheckUin = true;
  char *szArg = *p_szArg;

  if (szArg == NULL) {
    return 0;
  }

  // Check if the alias is quoted
  if (szArg[0] == '"')
  {
    bCheckUin = false;
    szAlias = &szArg[1];
    szCmd = strchr(&szArg[1], '"');
    if (szCmd == NULL)
    {
      winMain->wprintf("%CUnbalanced quotes.\n", 16);
      return (unsigned long)-1;
    }
    *szCmd++ = '\0';
    szCmd = strchr(szCmd, ' ');
  }
  else if (szArg[0] == '#')
  {
    *p_szArg = NULL;
    return gUserManager.OwnerUin();
  }
  else if (szArg[0] == '$')
  {
    *p_szArg = NULL;
    return winMain->nLastUin;
  }
  else
  {
    szAlias = szArg;
    szCmd = strchr(szArg, ' ');
  }

  if (szCmd != NULL)
  {
    *szCmd++ = '\0';
    STRIP(szCmd);
  }
  *p_szArg = szCmd;

  // Find the user
  // See if all the chars are digits
  if (bCheckUin)
  {
    char *sz = szAlias;
    while (isdigit(*sz)) sz++;
    if (*sz == '\0') nUin = atol(szAlias);
  }

  if (nUin == 0)
  {
    FOR_EACH_PROTO_USER_START(LICQ_PPID, LOCK_R)
    {
      if (strcasecmp(szAlias, pUser->GetAlias()) == 0)
      {
        nUin = pUser->Uin();
        FOR_EACH_USER_BREAK;
      }
    }
    FOR_EACH_PROTO_USER_END
    if (nUin == 0)
    {
      winMain->wprintf("%CInvalid user: %A%s\n", 16, A_BOLD, szAlias);
      return (unsigned long)-1;
    }
  }
  else
  {
    if (!gUserManager.IsOnList(nUin))
    {
      winMain->wprintf("%CInvalid uin: %A%lu\n", 16, A_BOLD, nUin);
      return (unsigned long)-1;
    }
  }

  // Save this as the last user
  if (winMain->nLastUin != nUin)
  {
    winMain->nLastUin = nUin;
    PrintStatus();
  }

  return nUin;
}

/*---------------------------------------------------------------------------
 * CLicqConsole::GetContactFromArg
 *-------------------------------------------------------------------------*/
struct SContact CLicqConsole::GetContactFromArg(char **p_szArg)
{
  char *szAlias, *szCmd;
  char *szArg = *p_szArg;
  unsigned long nPPID = 0;
  struct SContact scon;
  scon.szId = NULL;
  scon.nPPID = 0;

  if (szArg == NULL) {
    return scon;
  }
  string strArg(szArg);
  string::size_type nPos = strArg.find_last_of(".");
  if (nPos != string::npos)
  {
    string::size_type s = strArg.find_last_of(" ");
    string strProtocol(strArg, nPos + 1, (s == string::npos) ? strArg.size() : s - nPos - 1);
    ProtoPluginsList pl;
    ProtoPluginsListIter it;
    licqDaemon->ProtoPluginList(pl);
    for (it = pl.begin(); it != pl.end(); it++)
    {
      if (strcasecmp((*it)->Name(), strProtocol.c_str()) == 0)
      {
        nPPID = (*it)->PPID();
        szArg[strArg.find_last_of(".")] = '\0';
        string tmp(strArg, 0, nPos);
        tmp.append(strArg, s, strArg.size());
        szArg = (char *)tmp.c_str();
        break;
      }
    }
  }

  // Check if the alias is quoted
  if (szArg[0] == '"')
  {
    szAlias = &szArg[1];
    szCmd = strchr(&szArg[1], '"');
    if (szCmd == NULL)
    {
      winMain->wprintf("%CUnbalanced quotes.\n", 16);
      return scon;
    }
    *szCmd++ = '\0';
    szCmd = strchr(szCmd, ' ');
  }
  else if (szArg[0] == '#')
  {
    *p_szArg = NULL;
    ICQOwner *o = gUserManager.FetchOwner(LOCK_R);
    scon.szId = o->IdString();
    scon.nPPID = o->PPID(); 
    gUserManager.DropOwner();
    return scon;
  }
  else if (szArg[0] == '$')
  {
    *p_szArg = NULL;
    return winMain->sLastContact;
  }
  else
  {
    szAlias = szArg;
    szCmd = strchr(szArg, ' ');
  }

  if (szCmd != NULL)
  {
    *szCmd++ = '\0';
    STRIP(szCmd);
  }
  *p_szArg = szCmd;

  FOR_EACH_USER_START(LOCK_R)
  {
    if ((nPPID && pUser->PPID() == nPPID && strcasecmp(szAlias, pUser->GetAlias()) == 0) ||
        (!nPPID && strcasecmp(szAlias, pUser->GetAlias()) == 0))
    {
      scon.szId = pUser->IdString();
      scon.nPPID = pUser->PPID();
      FOR_EACH_PROTO_USER_BREAK;
    }
    else if ((nPPID && pUser->PPID() == nPPID && strcasecmp(szAlias, pUser->IdString()) == 0) ||
             (!nPPID && strcasecmp(szAlias, pUser->IdString()) == 0))
    {
      scon.szId = pUser->IdString();
      scon.nPPID = pUser->PPID();
      FOR_EACH_PROTO_USER_BREAK;
    }
  }
  FOR_EACH_USER_END
  if (scon.szId == NULL)
  {
    winMain->wprintf("%CInvalid user: %A%s\n", 16, A_BOLD, szAlias);
    scon.szId = NULL;
    scon.nPPID = (unsigned long)-1;
    return scon;
  }
  SaveLastUser(scon.szId, scon.nPPID);
  return scon;
}



/*---------------------------------------------------------------------------
 * CLicqConsole::MenuMessage
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuMessage(char *szArg)
{
  char *sz = szArg;
  struct SContact scon = GetContactFromArg(&sz);

  if (!scon.szId && scon.nPPID != (unsigned long)-1)
    winMain->wprintf("%CYou must specify a user to send a message to.\n", 16);
  else if (scon.nPPID != (unsigned long)-1)
    UserCommand_Msg(scon.szId, scon.nPPID, sz);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuInfo
 *-------------------------------------------------------------------------*/
//TODO: fix this for other protocols
void CLicqConsole::MenuInfo(char *szArg)
{
  char *sz = szArg;
  unsigned long nUin = GetUinFromArg(&sz);

  if (nUin == gUserManager.OwnerUin())
    winMain->wprintf("%CSetting personal info not implemented yet.\n", 16);
  else if (nUin == 0) {
    char szUin[24];
    sprintf(szArg, "%lu", gUserManager.OwnerUin());
    UserCommand_Info(szUin, LICQ_PPID, sz);
  } else if (nUin != (unsigned long)-1)
    sprintf(szArg, "%lu", nUin);
    UserCommand_Info(szArg, LICQ_PPID, sz);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuUrl
 *-------------------------------------------------------------------------*/
//TODO: fix this for other protocols
void CLicqConsole::MenuUrl(char *szArg)
{
  char *sz = szArg;
  unsigned long nUin = GetUinFromArg(&sz);

  if (nUin == gUserManager.OwnerUin())
    winMain->wprintf("%CYou can't send URLs to yourself!\n", 16);
  else if (nUin == 0)
    winMain->wprintf("%CYou must specify a user to send a URL to.\n", 16);
  else if (nUin != (unsigned long)-1)
    sprintf(szArg, "%lu", nUin);
    UserCommand_Url(szArg, LICQ_PPID, sz);

}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuSms
 *-------------------------------------------------------------------------*/
//TODO: fix this for other protocols
void CLicqConsole::MenuSms(char *szArg)
{
  char *sz = szArg;
  unsigned long nUin = GetUinFromArg(&sz);

  if (nUin == 0)
    winMain->wprintf("%CInvalid user\n", 16);
  else if (nUin != (unsigned long)-1)
  {
    sprintf(szArg, "%lu", nUin);
    UserCommand_Sms(szArg, LICQ_PPID, sz);
  }
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuView
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuView(char *szArg)
{

  char *sz = szArg;
  char *szId = NULL;
  unsigned long nPPID = (unsigned long)-1;
  struct SContact scon = GetContactFromArg(&sz);
  
  if (scon.szId == 0)
  {
    // Do nothing if there are no events pending
    if (ICQUser::getNumUserEvents() == 0) return;

    // Do icq system messages first
    ICQOwner *o = gUserManager.FetchOwner(LICQ_PPID, LOCK_R);
    unsigned short nNumMsg = o->NewMessages();
    gUserManager.DropOwner();
    if (nNumMsg > 0)
    {
      //TODO which owner?
      char szUin[24];
      sprintf(szUin, "%lu", gUserManager.OwnerUin());
      UserCommand_View(szUin, LICQ_PPID, NULL);
      return;
    }

    time_t t = time(NULL);
    FOR_EACH_USER_START(LOCK_R)
    {
      if (pUser->NewMessages() > 0 && pUser->Touched() <= t)
      {
        szId = pUser->IdString();
        nPPID = pUser->PPID();
        t = pUser->Touched();
      }
    }
    FOR_EACH_USER_END
    if (szId != NULL)
    {
      UserCommand_View(szId, nPPID, NULL);
    }
  }
  else if (scon.szId != NULL)
  {
    UserCommand_View(scon.szId, scon.nPPID, sz);
  }

}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuSecure
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuSecure(char *szArg)
{
  char *sz = szArg;
  unsigned long nUin = GetUinFromArg(&sz);

  if (nUin == gUserManager.OwnerUin())
    winMain->wprintf("%CYou can't establish a secure connection to yourself!\n", 16);
  else if (nUin == 0)
    winMain->wprintf("%CYou must specify a user to talk to.\n", 16);
  else if (nUin != (unsigned long)-1)
  {
    sprintf(szArg, "%lu", nUin);
    UserCommand_Secure(szArg, LICQ_PPID, sz);
  }
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuFile
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuFile(char *szArg)
{
  char *sz = szArg;
  unsigned long nUin = GetUinFromArg(&sz);

  if (nUin == gUserManager.OwnerUin())
    winMain->wprintf("%CYou can't send files to yourself!\n", 16);
  else if (nUin == 0)
  {
    bool bNum = false;

    // Go through the list and print out the info on each file
    list<CFileTransferManager *>::iterator iter;
    for(iter = m_lFileStat.begin(); iter != m_lFileStat.end(); iter++)
    {
      bNum = true;
      PrintFileStat(*iter);
    }

    if(!bNum)
    {
      winMain->wprintf("%A%CNo current file transfers.\n",
       m_cColorInfo->nAttr,
       m_cColorInfo->nColor);
    }
  }
  else if (nUin != (unsigned long)-1)
  {
    sprintf(szArg, "%lu", nUin);
    UserCommand_SendFile(szArg, LICQ_PPID, sz);
  }
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuAutoResponse
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuAutoResponse(char *szArg)
{
  char *sz = szArg;
  unsigned long nUin = GetUinFromArg(&sz);

  if (nUin == gUserManager.OwnerUin())
  {
    wattron(winMain->Win(), A_BOLD);
    for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
      waddch(winMain->Win(), ACS_HLINE);
    waddch(winMain->Win(), '\n');
    ICQOwner *o = gUserManager.FetchOwner(LICQ_PPID, LOCK_R);
    winMain->wprintf("%B%CAuto response:\n%b%s\n",
                     8, o->AutoResponse());
    gUserManager.DropOwner();
    wattron(winMain->Win(), A_BOLD);
    for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
      waddch(winMain->Win(), ACS_HLINE);
    waddch(winMain->Win(), '\n');
    winMain->RefreshWin();
    wattroff(winMain->Win(), A_BOLD);
  }
  else if (nUin == 0)
    UserCommand_SetAutoResponse(NULL, LICQ_PPID, sz);
  else if (nUin != (unsigned long)-1)
    UserCommand_FetchAutoResponse(NULL, LICQ_PPID, sz);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuRemove
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuRemove(char *szArg)
{
  char *sz = szArg;
  struct SContact scon = GetContactFromArg(&sz);

  if (gUserManager.FindOwner(scon.szId, scon.nPPID))
    winMain->wprintf("%CYou can't remove yourself!\n", 16);
  else if (!scon.szId && scon.nPPID != (unsigned long)-1)
    winMain->wprintf("%CYou must specify a user to remove.\n", 16);
  else
    UserCommand_Remove(scon.szId, scon.nPPID, sz);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuHistory
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuHistory(char *szArg)
{
  char *sz = szArg;
  struct SContact scon = GetContactFromArg(&sz);

  if (!scon.szId && scon.nPPID != (unsigned long)-1)
    winMain->wprintf("%CYou must specify a user to view history.\n", 16);
  else if (scon.nPPID != (unsigned long)-1)
    UserCommand_History(scon.szId, scon.nPPID, sz);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::MenuSet
 *-------------------------------------------------------------------------*/
void CLicqConsole::MenuSet(char *_szArg)
{
  char *szVariable, *szValue;
  unsigned short nVariable = 0, i;

  // If no argument then print all variables and return
  if (_szArg == NULL)
  {
    for (i = 0; i < NUM_VARIABLES; i++)
      PrintVariable(i);
    return;
  }

  szVariable = _szArg;
  szValue = strchr(_szArg, ' ');

  if (szValue != NULL)
  {
    *szValue++ = '\0';
    STRIP(szValue);
  }

  // Find the variable
  for (i = 0; i < NUM_VARIABLES; i++)
  {
    if (strcasecmp(szVariable, aVariables[i].szName) == 0)
    {
      nVariable = i;
      break;
    }
  }
  if (i == NUM_VARIABLES)
  {
    winMain->wprintf("%CNo such variable: %A%s\n", 16, A_BOLD, szVariable);
    return;
  }

  // If there is no value then print the current one
  if (szValue == NULL)
  {
    PrintVariable(nVariable);
    return;
  }

  // Set the variable
  switch(aVariables[nVariable].nType)
  {
  case BOOL:
    *(bool *)aVariables[nVariable].pData =
      ( strcasecmp(szValue, "yes") == 0 ||
        strcasecmp(szValue, "on") == 0 ||
        strcasecmp(szValue, "1") == 0 ||
        strcasecmp(szValue, "true") == 0 );
    break;

  case COLOR:
    for (i = 0; i < NUM_COLORMAPS; i++)
    {
      if (strcasecmp(szValue, aColorMaps[i].szName) == 0)
        break;
    }
    if (i == NUM_COLORMAPS)
    {
      winMain->wprintf("%CNo such color: %A%s\n", 16, A_BOLD, szValue);
      break;
    }

    switch(nVariable)
    {
      case 2: m_nColorOnline = i;    break;
      case 3: m_nColorAway = i;      break;
      case 4: m_nColorOffline = i;   break;
      case 5: m_nColorNew = i;       break;
      case 6: m_nColorGroupList = i; break;
      case 7: m_nColorQuery = i;     break;
      case 8: m_nColorInfo = i;      break;
      case 9: m_nColorError = i;     break;
      default: break;
    }

    *(const struct SColorMap **)aVariables[nVariable].pData = &aColorMaps[i];
    break;

  case STRING:
    if (szValue[0] != '"' || szValue[strlen(szValue) - 1] != '"')
    {
      winMain->wprintf("%CString values must be enclosed by double quotes (\").\n", 16);
      return;
    }
    szValue[strlen(szValue) - 1] = '\0';
    strncpy((char *)aVariables[nVariable].pData, &szValue[1], 30);
    break;

  case INT:
    *(int *)aVariables[nVariable].pData = atoi(szValue);
    break;
  }

  // Save it all
  DoneOptions();
}


/*-----------------------------------------------------------------------
 * CLicqConsole::MenuClear
 *---------------------------------------------------------------------*/
void CLicqConsole::MenuClear(char *)
{
  for (unsigned short i = 0; i < winMain->Rows(); i++)
    winMain->wprintf("\n");
}


/*-----------------------------------------------------------------------
 * CLicqConsole::MenuSearch
 *---------------------------------------------------------------------*/
void CLicqConsole::MenuSearch(char *)
{
  Command_Search();
}


