#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <cstring>
#include <cctype>
#include <vector>
#include <list>
#include <fstream>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
extern int errno
#endif

#include "console.h"
#include "licq_file.h"
#include "licq_filetransfer.h"
#include "licq_log.h"
#include "licq_icqd.h"
#include "event_data.h"

#include "licq_languagecodes.h"
#include "licq_countrycodes.h"

// Undefine what stupid ncurses defines as wclear(WINDOW *)
#undef clear

using namespace std;

extern "C" const char *LP_Version();

const unsigned short NUM_STATUS = 13;
const struct SStatus aStatus[NUM_STATUS] =
  {
    { "online", ICQ_STATUS_ONLINE },
    { "away", ICQ_STATUS_AWAY },
    { "na", ICQ_STATUS_NA },
    { "dnd", ICQ_STATUS_DND },
    { "occupied", ICQ_STATUS_OCCUPIED },
    { "ffc", ICQ_STATUS_FREEFORCHAT },
    { "offline", ICQ_STATUS_OFFLINE },
    { "*online", ICQ_STATUS_ONLINE },
    { "*away", ICQ_STATUS_AWAY },
    { "*na", ICQ_STATUS_NA },
    { "*dnd", ICQ_STATUS_DND },
    { "*occupied", ICQ_STATUS_OCCUPIED },
    { "*ffc", ICQ_STATUS_FREEFORCHAT }
  };

const unsigned short NUM_VARIABLES = 15;
struct SVariable aVariables[NUM_VARIABLES] =
  {
    { "show_offline_users", BOOL, NULL }
    ,           // 0
    { "show_dividers", BOOL, NULL }
    ,                // 1
    { "color_online", COLOR, NULL }
    ,                // 2
    { "color_away", COLOR, NULL }
    ,                  // 3
    { "color_offline", COLOR, NULL }
    ,               // 4
    { "color_new", COLOR, NULL }
    ,                   // 5
    { "color_group_list", COLOR, NULL }
    ,            // 6
    { "color_query", COLOR, NULL }
    ,                 // 7
    { "color_info", COLOR, NULL }
    ,                  // 8
    { "color_error", COLOR, NULL }
    ,                 // 9
    { "user_online_format", STRING, NULL }
    ,         // 10
    { "user_other_online_format", STRING, NULL }
    ,   // 11
    { "user_away_format", STRING, NULL }
    ,           // 12
    { "user_offline_format", STRING, NULL }
    ,        // 13
    { "command_character", STRING, NULL }           // 14
  };

const unsigned short NUM_COLORMAPS = 15;
const struct SColorMap aColorMaps[NUM_COLORMAPS] =
  {
    { "green", 24, A_NORMAL }
    ,          // 0
    { "red", 16, A_NORMAL }
    ,              // 1
    { "cyan", 56, A_NORMAL }
    ,            // 2
    { "white", 8, A_NORMAL }
    ,          // 3
    { "magenta", 48, A_NORMAL }
    ,      // 4
    { "blue", 40, A_NORMAL }
    ,            // 5
    { "yellow", 32, A_NORMAL }
    ,        // 6
    { "black", 8, A_NORMAL }
    ,          // 7
    { "bright_green", 24, A_BOLD }
    ,     // 8
    { "bright_red", 16, A_BOLD }
    ,         // 9
    { "bright_cyan", 56, A_BOLD }
    ,       // 10
    { "bright_white", 8, A_BOLD }
    ,     // 11
    { "bright_magenta", 48, A_BOLD }
    , // 12
    { "bright_blue", 40, A_BOLD }
    ,       // 13
    { "bright_yellow", 32, A_BOLD }    // 14
  };

const char MLE_HELP[] =
  "[ '.' send | '.d/s' force direct/server | '.u' send urgent | ',' abort ]";

/*---------------------------------------------------------------------------
 * CLicqConsole::Constructor
 *-------------------------------------------------------------------------*/
CLicqConsole::CLicqConsole(int argc, char **argv)
{
  CWindow::StartScreen();

  // oh yeah, add in a variable for the
  // status window text and colors.. that'd be cool
  char szFileName[MAX_FILENAME_LEN];
  sprintf(szFileName, "%s/licq_console.conf", BASE_DIR);
  CIniFile licqConf;
  if(!licqConf.LoadFile(szFileName))
  {
    FILE *f = fopen(szFileName, "w");
    fprintf(f, "[appearance]");
    fclose(f);
    licqConf.LoadFile(szFileName);
  }

  licqConf.SetSection("appearance");

  licqConf.ReadBool("ShowOfflineUsers", m_bShowOffline, true);
  licqConf.ReadBool("ShowDividers", m_bShowDividers, true);
  licqConf.ReadNum("CurrentGroup", m_nCurrentGroup, 0);
  unsigned short nGroupType;
  licqConf.ReadNum("GroupType", nGroupType, (unsigned short)GROUPS_USER);
  m_nGroupType = (GroupType)nGroupType;
  licqConf.ReadNum("ColorOnline", m_nColorOnline, 5);
  licqConf.ReadNum("ColorAway", m_nColorAway, 0);
  licqConf.ReadNum("ColorOffline", m_nColorOffline, 1);
  licqConf.ReadNum("ColorNew", m_nColorNew, 14);
  licqConf.ReadNum("ColorGroupList", m_nColorGroupList, 13);
  licqConf.ReadNum("ColorQuery", m_nColorQuery, 8);
  licqConf.ReadNum("ColorInfo", m_nColorInfo, 13);
  licqConf.ReadNum("ColorError", m_nColorError, 9);
  licqConf.ReadStr("OnlineFormat", m_szOnlineFormat, "%a");
  licqConf.ReadStr("OtherOnlineFormat", m_szOtherOnlineFormat, "%a [%S]");
  licqConf.ReadStr("AwayFormat", m_szAwayFormat, "%a [%S]");
  licqConf.ReadStr("OfflineFormat", m_szOfflineFormat, "%a");
  licqConf.ReadStr("CommandCharacter", m_szCommandChar, "/");
  licqConf.ReadNum("Backspace", m_nBackspace, KEY_BACKSPACE);

  if (licqConf.SetSection("macros"))
  {
    char sz[32];
    unsigned short n = 0;
    licqConf.ReadNum("NumMacros", n, 0);
    for (unsigned short i = 1; i <= n; i++)
    {
      SMacro *mac = new SMacro;
      sprintf(sz, "Macro.%d", i);
      licqConf.ReadStr(sz, mac->szMacro);
      sprintf(sz, "Command.%d", i);
      licqConf.ReadStr(sz, mac->szCommand);
      listMacros.push_back(mac);
    }
  }

  // Set the colors
  m_cColorOnline    = &aColorMaps[m_nColorOnline];
  m_cColorAway      = &aColorMaps[m_nColorAway];
  m_cColorOffline   = &aColorMaps[m_nColorOffline];
  m_cColorNew       = &aColorMaps[m_nColorNew];
  m_cColorGroupList = &aColorMaps[m_nColorGroupList];
  m_cColorQuery     = &aColorMaps[m_nColorQuery];
  m_cColorInfo      = &aColorMaps[m_nColorInfo];
  m_cColorError     = &aColorMaps[m_nColorError];

  m_lCmdHistoryIter = m_lCmdHistory.end();

  // Set the variable data pointers
  unsigned short i = 0;
  aVariables[i++].pData = &m_bShowOffline;
  aVariables[i++].pData = &m_bShowDividers;
  aVariables[i++].pData = &m_cColorOnline;
  aVariables[i++].pData = &m_cColorAway;
  aVariables[i++].pData = &m_cColorOffline;
  aVariables[i++].pData = &m_cColorNew;
  aVariables[i++].pData = &m_cColorGroupList;
  aVariables[i++].pData = &m_cColorQuery;
  aVariables[i++].pData = &m_cColorInfo;
  aVariables[i++].pData = &m_cColorError;
  aVariables[i++].pData = m_szOnlineFormat;
  aVariables[i++].pData = m_szOtherOnlineFormat;
  aVariables[i++].pData = m_szAwayFormat;
  aVariables[i++].pData = m_szOfflineFormat;
  aVariables[i++].pData = m_szCommandChar;

  m_bExit = false;
  cdkUserList = 0;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::Destructor
 *-------------------------------------------------------------------------*/
CLicqConsole::~CLicqConsole()
{
  for (unsigned short i = 0; i <= MAX_CON; i++)
    delete winCon[i];
  delete winConStatus;
  delete winStatus;
  delete winPrompt;
  CWindow::EndScreen();
}

/*---------------------------------------------------------------------------
 * CLicqConsole::Shutdown
 *-------------------------------------------------------------------------*/
void CLicqConsole::Shutdown()
{
  gLog.Info("%sShutting down console.\n", L_CONSOLExSTR);
  gLog.ModifyService(S_PLUGIN, 0);
  licqDaemon->UnregisterPlugin();
}


/*---------------------------------------------------------------------------
 * CLicqConsole::Run
 *-------------------------------------------------------------------------*/
int CLicqConsole::Run(CICQDaemon *_licqDaemon)
{
  // Register with the daemon, we want to receive all signals
  m_nPipe = _licqDaemon->RegisterPlugin(SIGNAL_ALL);
  m_bExit = false; 
  licqDaemon = _licqDaemon;
  m_nCurrentGroup = gUserManager.DefaultGroup();

  // Create the windows
  for (unsigned short i = 0; i <= MAX_CON; i++)
  {
    winCon[i] = new CWindow(LINES - 5, COLS - USER_WIN_WIDTH - 1, 2, USER_WIN_WIDTH + 1,
                            SCROLLBACK_BUFFER, true);
    scrollok(winCon[i]->Win(), true);
    winCon[i]->fProcessInput = &CLicqConsole::InputCommand;
    winCon[i]->data = NULL;
  }
  winCon[0]->fProcessInput = &CLicqConsole::InputLogWindow;
  winStatus = new CWindow(2, COLS, LINES - 3, 0, false);
  winPrompt = new CWindow(1, COLS, LINES - 1, 0, false);
  winConStatus = new CWindow(2, COLS, 0, 0, false);
  winStatus->SetActive(true);
  winPrompt->SetActive(true);
  winConStatus->SetActive(true);
  winBar = new CWindow(LINES - 5, 1, 2, COLS - USER_WIN_WIDTH - 1, false);
  winUsers = new CWindow(LINES - 5, USER_WIN_WIDTH, 2, 0, false, true);
  winBar->SetActive(true);
  winUsers->SetActive(true);

  log = new CPluginLog;
  unsigned long lt = L_MOST;
  if (gLog.ServiceLogTypes(S_STDERR) & L_PACKET)
    lt |= L_PACKET;
  gLog.AddService(new CLogService_Plugin(log, lt));
  gLog.ModifyService(S_STDERR, L_NONE);

  winMain = winCon[1];
  winLog = winCon[0];
  SwitchToCon(1);

  PrintStatus();
  PrintPrompt();
  CreateUserList();
  PrintUsers();

  if (gUserManager.NumOwners() == 0)
  {
    RegistrationWizard();
  }
  else
  {
    ICQOwner *o = gUserManager.FetchOwner(LICQ_PPID, LOCK_R);
    if (o->Password()[0] == '\0')
    {
      gUserManager.DropOwner();
      UserSelect();
    }
    else
      gUserManager.DropOwner();
  }

  //fd_set fdSet;
  int nResult;

  while (!m_bExit)
  {
    FD_ZERO(&fdSet);
    FD_SET(STDIN_FILENO, &fdSet);
    FD_SET(m_nPipe, &fdSet);
    FD_SET(log->Pipe(), &fdSet);

    int nNumDesc = log->Pipe() + 1;

    // Check to see if we want to add in the file xfer manager..
    list<CFileTransferManager *>::iterator iter;
    for (iter = m_lFileStat.begin(); iter != m_lFileStat.end(); iter++)
    {
      FD_SET((*iter)->Pipe(), &fdSet);
      nNumDesc += (*iter)->Pipe();
    }

    nResult = select(nNumDesc, &fdSet, NULL, NULL, NULL);
    if (nResult == -1)
    {
      if (errno != EINTR)
      {
        gLog.Error("Error in select(): %s.\n", strerror(errno));
        m_bExit = true;
      }
    }
    else
    {
      if (FD_ISSET(STDIN_FILENO, &fdSet))
      {
        ProcessStdin();
        continue;
      }
      else if (FD_ISSET(m_nPipe, &fdSet))
      {
        ProcessPipe();
        continue;
      }
      else if (FD_ISSET(log->Pipe(), &fdSet))
      {
        ProcessLog();
        continue;
      }

      list<CFileTransferManager *>::iterator iter;
      for (iter = m_lFileStat.begin(); iter != m_lFileStat.end(); iter++)
      {
        if (FD_ISSET((*iter)->Pipe(), &fdSet))
        {
          if (!ProcessFile(*iter))
          {
            delete *iter;
#undef erase
            m_lFileStat.erase(iter);
          }

          break;
        }
      }
    }
  }

  winMain->wprintf("Exiting\n\n");
  return 0;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::DoneOptions
 *-------------------------------------------------------------------------*/
void CLicqConsole::DoneOptions()
{
  char szFileName[MAX_FILENAME_LEN];
  sprintf(szFileName, "%s/licq_console.conf", BASE_DIR);
  CIniFile licqConf(INI_FxALLOWxCREATE);
  if(!licqConf.LoadFile(szFileName))
    return;

  licqConf.SetSection("appearance");
  licqConf.WriteBool("ShowOfflineUsers", m_bShowOffline);
  licqConf.WriteBool("ShowDividers", m_bShowDividers);
  licqConf.WriteNum("CurrentGroup", m_nCurrentGroup);
  licqConf.WriteNum("GroupType", (unsigned short)m_nGroupType);
  licqConf.WriteNum("ColorOnline", m_nColorOnline);
  licqConf.WriteNum("ColorAway", m_nColorAway);
  licqConf.WriteNum("ColorOffline", m_nColorOffline);
  licqConf.WriteNum("ColorNew", m_nColorNew);
  licqConf.WriteNum("ColorGroupList", m_nColorGroupList);
  licqConf.WriteNum("ColorQuery", m_nColorQuery);
  licqConf.WriteNum("ColorInfo", m_nColorInfo);
  licqConf.WriteNum("ColorError", m_nColorError);
  licqConf.WriteStr("OnlineFormat", m_szOnlineFormat);
  licqConf.WriteStr("OtherOnlineFormat", m_szOtherOnlineFormat);
  licqConf.WriteStr("AwayFormat", m_szAwayFormat);
  licqConf.WriteStr("OfflineFormat", m_szOfflineFormat);
  licqConf.WriteStr("CommandCharacter", m_szCommandChar);
  licqConf.WriteNum("Backspace", (unsigned long)m_nBackspace);

  licqConf.SetSection("macros");
  char sz[32];
  unsigned short i = 1;
  licqConf.WriteNum("NumMacros", (unsigned short)listMacros.size());
  for (MacroList::iterator iter = listMacros.begin(); iter != listMacros.end(); iter++, i++)
  {
    sprintf(sz, "Macro.%d", i);
    licqConf.WriteStr(sz, (*iter)->szMacro);
    sprintf(sz, "Command.%d", i);
    licqConf.WriteStr(sz, (*iter)->szCommand);
  }

  licqConf.FlushFile();
  licqConf.CloseFile();
}

/*---------------------------------------------------------------------------
 * CLicqConsole::ProcessLog
 *-------------------------------------------------------------------------*/
void CLicqConsole::ProcessLog()
{
  static char buf[2];
  read(log->Pipe(), buf, 1);

  short cp;
  switch (log->NextLogType())
  {
  case L_WARN:
    cp = 32;
    break;
  case L_ERROR:
    cp = 16;
    break;
  case L_PACKET:
    cp = 40;
    break;
  case L_UNKNOWN:
    cp = 48;
    break;
  case L_INFO:
  default:
    cp = 8;
    break;
  }
  char *p = log->NextLogMsg();
  char *l = &p[LOG_PREFIX_OFFSET];
  p[LOG_PREFIX_OFFSET - 1] = '\0';
  winLog->wprintf("%C%s %C%s", 24, p, cp, l);
  if (log->NextLogType() == L_ERROR)
  {
    winMain->wprintf("%C%s %C%s", 24, p, cp, l);
    winMain->RefreshWin();
  }
  log->ClearLog();
  winLog->RefreshWin();
}


/*---------------------------------------------------------------------------
 * CLicqConsole::ProcessPipe
 *-------------------------------------------------------------------------*/
void CLicqConsole::ProcessPipe()
{
  char buf[16];
  read(m_nPipe, buf, 1);
  switch (buf[0])
  {
  case 'S':  // A signal is pending
    {
      CICQSignal *s = licqDaemon->PopPluginSignal();
      ProcessSignal(s);
      break;
    }

  case 'E':  // An event is pending
    {
      ICQEvent *e = licqDaemon->PopPluginEvent();
      ProcessEvent(e);
      break;
    }

  case 'X':  // Shutdown
    {
      gLog.Info("%sExiting console.\n", L_CONSOLExSTR);
      m_bExit = true;
      break;
    }

  case '0':
  case '1':
    break;

  default:
    gLog.Warn("%sUnknown notification type from daemon: %c.\n", L_WARNxSTR, buf[0]);
  }
}


/*---------------------------------------------------------------------------
 * CLicqConsole::ProcessSignal
 *-------------------------------------------------------------------------*/
void CLicqConsole::ProcessSignal(CICQSignal *s)
{
  switch (s->Signal())
  {
  case SIGNAL_UPDATExLIST:
    if (s->SubSignal() == LIST_REMOVE)
    {
      for (unsigned short i = 0; i < MAX_CON; i++)
      {
        if (s->Uin() == winCon[i]->nLastUin)
          winCon[i]->nLastUin = 0;
      }
    }
    PrintStatus();
    CreateUserList();
    PrintUsers();
    break;
  case SIGNAL_UPDATExUSER:
    {
      if (gUserManager.FindOwner(s->Id(), s->PPID()) != NULL && s->SubSignal() == USER_STATUS
          || s->SubSignal() == USER_EVENTS)
        PrintStatus();
      ICQUser *u = gUserManager.FetchUser(s->Id(), s->PPID(), LOCK_R);
      if (u != NULL)
      {
        if (u->GetInGroup(m_nGroupType, m_nCurrentGroup))
        {
          CreateUserList();
          PrintUsers();
        }
        gUserManager.DropUser(u);
      }

      break;
    }
  case SIGNAL_LOGON:
  case SIGNAL_LOGOFF:
    PrintStatus();
    break;
  case SIGNAL_ADDxSERVERxLIST:
    licqDaemon->icqRenameUser(s->Uin());
    break;
  case SIGNAL_NEWxPROTO_PLUGIN:
    //ignore for now
    break;
  case SIGNAL_EVENTxID:
    AddEventTag(s->Id(), s->PPID(), s->Argument());
    break;
  default:
    gLog.Warn("%sInternal error: CLicqConsole::ProcessSignal(): Unknown signal command received from daemon: %ld.\n",
              L_WARNxSTR, s->Signal());
    break;
  }
  delete s;
}

/*---------------------------------------------------------------------------
 * CLicqConsole::AddEventTag
 *-------------------------------------------------------------------------*/
void CLicqConsole::AddEventTag(char *_szId, unsigned long _nPPID, unsigned long _nEventTag)
{
  if (!_szId || !_nPPID || !_nEventTag)
    return;

  CData *data;
  unsigned short i;
  for (i = 1; i <= MAX_CON; i++)
  {
    data = (CData *)winCon[i]->data;
    if (data && strcmp(data->szId, _szId) == 0 && data->nPPID == _nPPID)
    {
      winCon[i]->event = _nEventTag;
      break;
    }
  }
} 
    

/*---------------------------------------------------------------------------
 * CLicqConsole::ProcessEvent
 *-------------------------------------------------------------------------*/
void CLicqConsole::ProcessEvent(ICQEvent *e)
{
  if (e->Command() == ICQ_CMDxTCP_START) // direct connection check
  {
    ProcessDoneEvent(e);
    delete e;
    return;
  }

  if (e->SNAC() == 0) {
    // Not from ICQ
    ProcessDoneEvent(e); //FIXME
    return;
  }
  
  switch (e->SNAC())
  {
    // Event commands for a user
  case MAKESNAC(ICQ_SNACxFAM_MESSAGE, ICQ_SNACxMSG_SERVERxMESSAGE):
  case MAKESNAC(ICQ_SNACxFAM_MESSAGE, ICQ_SNACxMSG_SERVERxREPLYxMSG):
  case MAKESNAC(ICQ_SNACxFAM_MESSAGE, ICQ_SNACxMSG_SENDxSERVER):
    {
      ProcessDoneEvent(e);
      break;
    }

    // The all being meta snac
  case MAKESNAC(ICQ_SNACxFAM_VARIOUS, ICQ_SNACxMETA):
    {
      if (e->SubCommand() == ICQ_CMDxMETA_SEARCHxWPxLAST_USER ||
          e->SubCommand() == ICQ_CMDxMETA_SEARCHxWPxFOUND)
        ProcessDoneSearch(e);
      else
        ProcessDoneEvent(e);
      break;
    }

    // Commands related to the basic operation
  case MAKESNAC(ICQ_SNACxFAM_SERVICE, ICQ_SNACxSRV_SETxSTATUS):
  case MAKESNAC(ICQ_SNACxFAM_BUDDY, ICQ_SNACxBDY_ADDxTOxLIST):
  case ICQ_CMDxSND_LOGON:
    if (e->Result() != EVENT_SUCCESS)
      winMain->wprintf("%CLogon failed.  See the log console for details.\n", 16);
    break;

  case ICQ_CMDxSND_REGISTERxUSER:
    // Needs to be better dealt with...
    // How's this then?
    winMain->wprintf("Registration complete!\nYour UIN is %ld\n",
                     gUserManager.OwnerUin());
    winMain->fProcessInput = &CLicqConsole::InputCommand;
    PrintStatus();
    break;

  default:
    gLog.Warn("%sInternal error: CLicqConsole::ProcessEvent(): Unknown event SNAC received from daemon: 0x%08lX.\n",
              L_WARNxSTR, e->SNAC());
    break;
  }
  delete e;
}

/*---------------------------------------------------------------------------
 * CLicqConsole::ProcessFile
 *-------------------------------------------------------------------------*/
bool CLicqConsole::ProcessFile(CFileTransferManager *ftman)
{
  char buf[32];
  bool bCloseFT = false;
  read(ftman->Pipe(), buf, 32);

  CFileTransferEvent *e = NULL;

  while ((e = ftman->PopFileTransferEvent()) != NULL)
  {
    switch(e->Command())
    {
    case FT_ERRORxCONNECT:
      winMain->wprintf("%C%AFile transfer could not connect.  See network "
                       "window for details.%C%Z\n",
                       16, A_BOLD, 8, A_BOLD);
      bCloseFT = true;
      break;

    case FT_ERRORxBIND:
      winMain->wprintf("%C%AFile transfer could not bind to a port.  See "
                       "network window for details.%C%Z\n",
                       16, A_BOLD, 8, A_BOLD);
      bCloseFT = true;
      break;

    case FT_ERRORxRESOURCES:
      winMain->wprintf("%C%AFile transfer unable to create new thread.  See "
                       "network window for details.%C%Z\n",
                       16, A_BOLD, 8, A_BOLD);
      bCloseFT = true;
      break;
           
    case FT_DONExFILE:
      break;

    case FT_DONExBATCH:
      winMain->wprintf("%C%AFile transfer successfuly finished.%C%Z\n",
                       24, A_BOLD, 8, A_BOLD);
      bCloseFT = true;
      break;

    case FT_ERRORxCLOSED:
      winMain->wprintf("%C%AFile transfer closed.%C%Z\n",
                       16, A_BOLD, 8, A_BOLD);
      bCloseFT = true;
      break;

    case FT_ERRORxFILE:
      winMain->wprintf("%C%AFile transfer I/O error.%C%Z\n",
                       16, A_BOLD, 8, A_BOLD);
      bCloseFT = true;
      break;

    case FT_ERRORxHANDSHAKE:
      winMain->wprintf("%C%AFile transfer handshake error.%C%Z\n",
                       16, A_BOLD, 8, A_BOLD);
      bCloseFT = true;
      break;

    case FT_CONFIRMxFILE:
      ftman->StartReceivingFile(const_cast<char *>(ftman->FileName()));
      break;
    }

    if (bCloseFT)
    {
      ftman->CloseFileTransfer();
      delete e;
      return false;
    }

    delete e;
  }

  return true;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::ProcessDoneEvent
 *-------------------------------------------------------------------------*/
void CLicqConsole::ProcessDoneEvent(ICQEvent *e)
{
  CWindow *win = NULL;

  unsigned short i;
  for (i = 1; i <= MAX_CON; i++)
  {
    if (winCon[i]->event != 0 && e->Equals(winCon[i]->event))
    {
      win = winCon[i];
      break;
    }
  }
  if (win == NULL)
  {
    gLog.Warn("%sInternal error: CLicqConsole::ProcessDoneEvent(): Unknown event from daemon: %d.\n",
              L_WARNxSTR, e->SubCommand());
    return;
  }

  bool isOk = (e != NULL && (e->Result() == EVENT_ACKED || e->Result() == EVENT_SUCCESS));

  if (e == NULL)
  {
    win->wprintf("%A%Cerror\n", A_BOLD, 16);
  }
  else
  {
    switch (e->Result())
    {
    case EVENT_ACKED:
    case EVENT_SUCCESS:
      win->wprintf("%A%Cdone\n", m_cColorInfo->nAttr, m_cColorInfo->nColor);
      break;
    case EVENT_TIMEDOUT:
      win->wprintf("%A%Ctimed out\n", m_cColorError->nAttr, m_cColorError->nColor);
      break;
    case EVENT_FAILED:
      win->wprintf("%A%Cfailed\n", m_cColorError->nAttr, m_cColorError->nColor);
      break;
    case EVENT_ERROR:
      win->wprintf("%A%Cerror\n", m_cColorError->nAttr, m_cColorError->nColor);
      break;
    case EVENT_CANCELLED:
      win->wprintf("%A%Ccancelled\n", m_cColorInfo->nAttr, m_cColorInfo->nColor);
      break;
    }
  }
  win->event = 0;
  if (e == NULL)
    return;

  if (!isOk)
  {
    if (e->Command() == ICQ_CMDxTCP_START &&
        (e->SubCommand() == ICQ_CMDxSUB_MSG ||
         e->SubCommand() == ICQ_CMDxSUB_URL ||
         e->SubCommand() == ICQ_CMDxSUB_FILE) )
    {
      win->wprintf("%C%ADirect send failed, send through server (y/N)? %C%Z",
                   m_cColorQuery->nColor, m_cColorQuery->nAttr, 8,
                   A_BOLD);
      win->state = STATE_QUERY;
      win->data->nPos = 0;
      return;
    }
  }
  else
  {
    switch(e->Command())
    {
    case ICQ_CMDxTCP_START:
      {
        ICQUser *u = NULL;
        CUserEvent *ue = e->UserEvent();
        if (e->SubResult() == ICQ_TCPxACK_RETURN)
        {
          u = gUserManager.FetchUser(e->Uin(), LOCK_R);
          win->wprintf("%s is in %s mode:\n%s\n[Send \"urgent\" ('.u') to ignore]\n",
                       u->GetAlias(), u->StatusStr(), u->AutoResponse());
          gUserManager.DropUser(u);
        }
        else if (e->SubResult() == ICQ_TCPxACK_REFUSE)
        {
          u = gUserManager.FetchUser(e->Uin(), LOCK_R);
          win->wprintf("%s refused %s.\n",
                       u->GetAlias(), ue->Description());
          gUserManager.DropUser(u);
        }
        else if(e->SubCommand() == ICQ_CMDxSUB_FILE)
        {
          CExtendedAck *ea = e->ExtendedAck();

          if( ea == NULL || ue == NULL)
          {
            gLog.Error("%sInternal error: file request acknowledgement without extended result.\n", L_ERRORxSTR);
            return;
          }

          if(!ea->Accepted())
          {
            u = gUserManager.FetchUser(e->Uin(), LOCK_R);
            win->wprintf("%s refused file: %s\n",
                         u->GetAlias(), ea->Response());
            gUserManager.DropUser(u);
          }

          else
          {
            // For now don't check for a chat subcommand..
            // Invoke a file transfer manager here
            CEventFile *f = (CEventFile *)ue;
            CFileTransferManager *ftman = new CFileTransferManager(licqDaemon,
                                          e->Uin());
            m_lFileStat.push_back(ftman);

            // Now watch the file pipe
            ftman->SetUpdatesEnabled(1);
            FD_SET(ftman->Pipe(), &fdSet);

            ConstFileList fl;
            fl.push_back(f->Filename());
            ftman->SendFiles(fl, ea->Port());
          }
        }
        /*else if (e->m_nSubCommand == ICQ_CMDxSUB_CHAT || e->m_nSubCommand == ICQ_CMDxSUB_FILE)
            {
              struct SExtendedAck *ea = e->m_sExtendedAck;
              if (ea == NULL || ue == NULL)
              {
                gLog.Error("%sInternal error: ICQFunctions::doneFcn(): chat or file request acknowledgement without extended result.\n", L_ERRORxSTR);
                return;
              }
              if (!ea->bAccepted)
              {
                 u = gUserManager.FetchUser(m_nUin, LOCK_R);
                 QString result;
                 result.sprintf(tr("%s%1 with %2 refused:\n%s%3"), L_TCPxSTR, L_BLANKxSTR);
                 result.arg(EventDescription(ue)).arg(u->GetAlias()).arg(ea->szResponse);
                 gUserManager.DropUser(u);
                 InformUser(this, result);
              }
              else
              {
                switch (e->m_nSubCommand)
                {
                case ICQ_CMDxSUB_CHAT:
                {
                  ChatDlg *chatDlg = new ChatDlg(m_nUin, false, ea->nPort);
                  chatDlg->show();
                  break;
                }
                case ICQ_CMDxSUB_FILE:
                {
                  CFileDlg *fileDlg = new CFileDlg(m_nUin,
                                                   ((CEventFile *)ue)->Filename(),
                                                   ((CEventFile *)ue)->FileSize(),
                                                   false, ea->nPort);
                  fileDlg->show();
                  break;
                }
                default:
                  break;
                } // case
              } // if accepted
            } // if file or chat*/
        else
        {
          u = gUserManager.FetchUser(e->Uin(), LOCK_R);
          if (u != NULL && u->Away() && u->ShowAwayMsg())
          {
            win->wprintf("%s\n", u->AutoResponse());
          }
          gUserManager.DropUser(u);
        }

        break;
      } // case

    case ICQ_CMDxSND_THRUxSERVER:
    case ICQ_CMDxSND_USERxGETINFO:
    case ICQ_CMDxSND_USERxGETDETAILS:
    case ICQ_CMDxSND_UPDATExBASIC:
    case ICQ_CMDxSND_UPDATExDETAIL:
    case ICQ_CMDxSND_META:
    default:
      break;

    }
  }

  win->fProcessInput = &CLicqConsole::InputCommand;
  if (win->data != NULL)
  {
    delete win->data;
    win->data = NULL;
  }
  win->state = STATE_COMMAND;

}


/*---------------------------------------------------------------------------
* CLicqConsole::ProcessDoneSearch
 *-------------------------------------------------------------------------*/
void CLicqConsole::ProcessDoneSearch(ICQEvent *e)
{
  CWindow *win = NULL;

  unsigned short i;
  for (i = 1; i <= MAX_CON; i++)
  {
    if (winCon[i]->event != 0 && e->Equals(winCon[i]->event))
    {
      win = winCon[i];
      break;
    }
  }
  if (win == NULL)
  {
    gLog.Warn("%sInternal error: CLicqConsole::ProcessEvent(): Unknown event from daemon: %d.\n",
              L_WARNxSTR, e->Command());
    return;
  }

  if (e->SearchAck() != NULL && e->SearchAck()->Uin() != 0)
  {
    win->wprintf("%C%s%A,%Z %s %s %A(%Z%s%A) -%Z %lu %A(%Z%s%A)\n",
                 8,
                 e->SearchAck()->Alias(),
                 A_BOLD, A_BOLD,
                 e->SearchAck()->FirstName(),
                 e->SearchAck()->LastName(),
                 A_BOLD, A_BOLD,
                 e->SearchAck()->Email(),
                 A_BOLD, A_BOLD,
                 e->SearchAck()->Uin(),
                 A_BOLD, A_BOLD,
                 e->SearchAck()->Status() == SA_ONLINE ? "online" :
                 e->SearchAck()->Status() == SA_OFFLINE ? "offline" :
                 "disabled",
                 A_BOLD);
  }

  if (e->Result() == EVENT_ACKED)
    return;

  if (e->Result() == EVENT_SUCCESS)
  {
    if (e->SearchAck() == NULL || e->SearchAck()->More() == 0)
    {
      win->wprintf("%A%CSearch complete.\n", m_cColorInfo->nAttr, m_cColorInfo->nColor);
    }
    else if (e->SearchAck()->More() == -1)
    {
      win->wprintf("%A%CSearch complete.  More users found, narrow search.\n",
                   m_cColorInfo->nAttr, m_cColorInfo->nColor);
    }
    else if (e->SearchAck()->More() > 0)
    {
      win->wprintf("%A%CSearch complete.  %d more users found, narrow search.\n",
                   m_cColorInfo->nAttr, m_cColorInfo->nColor, e->SearchAck()->More());
    }
  }
  else
  {
    win->wprintf("%CSearch failed.\n", 16);
  }

  win->fProcessInput = &CLicqConsole::InputCommand;
  if (win->data != NULL)
  {
    delete win->data;
    win->data = NULL;
  }
  win->state = STATE_COMMAND;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::SwitchToCon
 *-------------------------------------------------------------------------*/
void CLicqConsole::SwitchToCon(unsigned short nCon)
{
  winMain->SetActive(false);
  winMain = winCon[nCon];
  winMain->SetActive(true);
  m_nCon = nCon;

  // Print the header
  wbkgdset(winConStatus->Win(), COLOR_PAIR(29));
  werase(winConStatus->Win());
  winConStatus->wprintf("%A[ %CLicq Console Plugin v%C%s%C (",
                        A_BOLD, 5,
                        53, LP_Version(), 29);
  if (m_nCon != 0)
    winConStatus->wprintf("%A%Cconsole %C%d", A_BOLD, 5,
                          53, m_nCon);
  else
    winConStatus->wprintf("%A%Clog console", A_BOLD, 5);
  winConStatus->wprintf("%A%C) ]", A_BOLD, 29);
  wclrtoeol(winConStatus->Win());
  wbkgdset(winConStatus->Win(), COLOR_PAIR(8));
  mvwhline(winConStatus->Win(), 1, 0, ACS_HLINE, COLS);
  //mvwaddch(winConStatus->Win(), 1, COLS - USER_WIN_WIDTH - 1, ACS_TTEE);
  winConStatus->RefreshWin();

  // Refresh the last user field
  PrintStatus();
}


/*---------------------------------------------------------------------------
 * CLicqConsole::ProcessStdin
 *-------------------------------------------------------------------------*/
void CLicqConsole::ProcessStdin()
{
  int cIn = wgetch(winPrompt->Win());

  // First check for a function key
  for (unsigned short i = 1; i <= MAX_CON; i++)
  {
    if (cIn == KEY_F(i))
    {
      SwitchToCon(i);
      return;
    }
  }
  if (cIn == KEY_F(MAX_CON + 1))
  {
    SwitchToCon(0);
    return;
  }
  if (cIn == KEY_F(MAX_CON + 2))
  {
    MenuList((char *)NULL);
    return;
  }

  // Some operating systems don't have a consistent set of settings for
  // what key should erase characters.  In particular, FC3 sets the terminal
  // erase character (kbs) for xterm to ^H instead of 127, even though the
  // rest of the operating system follows the Debian conventions.
  // This allows the user to override whatever the OS has chosen.
  if (cIn == m_nBackspace)
    cIn = KEY_BACKSPACE;

  (this->*(winMain->fProcessInput))(cIn);
}

/*---------------------------------------------------------------------------
 * CLicqConsole::InputLogWindow
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputLogWindow(int cIn)
{
  // Check for keys
  switch (cIn)
  {
  case KEY_PPAGE:
    winMain->ScrollUp();
    break;

  case KEY_NPAGE:
    winMain->ScrollDown();
    break;

  default:
    Beep();
    break;
  }
}


/*---------------------------------------------------------------------------
 * CLicqConsole::InputCommand
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputCommand(int cIn)
{
  static char szIn[1024];
  static int nPos = 0;
  static int nTabs = 0;
  static struct STabCompletion sTabCompletion;

  // Now check for keys
  switch (cIn)
  {
  case KEY_PPAGE:
    winMain->ScrollUp();
    break;

  case KEY_NPAGE:
    winMain->ScrollDown();
    break;

  case KEY_CLEAR:
  case '\014':
    {
      wrefresh(curscr);
      break;
    }


  case KEY_DOWN:
    if (m_lCmdHistoryIter == m_lCmdHistory.end())
    {
      Beep();
      break;
    }
    // Erase the old command
    while (nPos > 0)
    {
      int yp, xp;
      getyx(winPrompt->Win(), yp, xp);
      mvwdelch(winPrompt->Win(), yp, xp - 1);
      nPos--;
    }
    m_lCmdHistoryIter++;
    if (m_lCmdHistoryIter == m_lCmdHistory.end())
      szIn[0] = '\0';
    else
      strcpy(szIn, *m_lCmdHistoryIter);
    nPos = strlen(szIn);
    *winPrompt << szIn;
    break;

  case KEY_UP:
    if (m_lCmdHistoryIter == m_lCmdHistory.begin())
    {
      Beep();
      break;
    }
    // Erase the old command
    while (nPos > 0)
    {
      int yp, xp;
      getyx(winPrompt->Win(), yp, xp);
      mvwdelch(winPrompt->Win(), yp, xp - 1);
      nPos--;
    }
    m_lCmdHistoryIter--;
    strcpy(szIn, *m_lCmdHistoryIter);
    nPos = strlen(szIn);
    *winPrompt << szIn;
    break;

  case KEY_BACKSPACE:
  case KEY_DC:
  case KEY_LEFT:
  case '\b':
    {
      if (nPos == 0)
        return;
      int yp, xp;
      getyx(winPrompt->Win(), yp, xp);
      mvwdelch(winPrompt->Win(), yp, xp - 1);
      winPrompt->RefreshWin();
      nPos--;
      break;
    }

  case '\t':
    {
      // Check if this is the second tab in a row
      if (nTabs > 0)
      {
        unsigned short nCol = 0;
        vector<char *>::iterator iter;
        for (iter = sTabCompletion.vszPartialMatch.begin();
             iter != sTabCompletion.vszPartialMatch.end(); iter++)
        {
          // Add a newline if necessary
          if (nCol + strlen(*iter) + 5 >= (unsigned short)COLS)
          {
            *winMain << '\n';
            nCol = 0;
          }
          // Print out the partial matches
          winMain->wprintf("[ %A%s %Z] ", A_BOLD, *iter, A_BOLD);
          nCol += strlen(*iter) + 5;
        }
        *winMain << '\n';
        break;
      }

      // If we haven't typed anything, why should we tab?
      if (!nPos)
        break;

      szIn[nPos] = '\0';
      char *szArg = strchr(szIn, ' ');
      unsigned short nArgPos = 0;
      if (szIn[0] == m_szCommandChar[0] && szArg == NULL)
      { // Command completion
        TabCommand(szIn, sTabCompletion);
      }
      else if (szIn[0] != m_szCommandChar[0])
      { // User completion
        szArg = NULL;
        nArgPos = 0;
        TabUser(szIn, sTabCompletion);
      }
      else
      { // Argument completion
        *szArg = '\0';
        szArg++;
        while (*szArg == ' ')
          szArg++;
        nArgPos = szArg - szIn;
        // Figure out which command we have
        bool bTab = false;
        for (unsigned short i = 0; i < NUM_COMMANDS; i++)
        {
          char szTempCmd[20];
          snprintf(szTempCmd, 20, "%c%s", m_szCommandChar[0], aCommands[i].szName);
          if (strncasecmp(szIn, szTempCmd, strlen(szIn)) == 0)
          {
            if (!aCommands[i].fProcessTab)
              break;
            ;
            (this->*(aCommands[i].fProcessTab))(szArg, sTabCompletion);
            bTab = true;
            break;
          }
        }
        // Restore the space
        szIn[strlen(szIn)] = ' ';
        if (bTab == false)
        {
          Beep();
          break;
        }
      }

      // Process the tab results
      if (sTabCompletion.vszPartialMatch.size() == 0)
      { // No matches
        Beep();
        winMain->wprintf("%CNo matches.\n", 16);
        wprintw(winPrompt->Win(), "");
        break;
      }

      // Erase the old argument
      int yp, xp;
      while (nPos > nArgPos)
      {
        getyx(winPrompt->Win(), yp, xp);
        mvwdelch(winPrompt->Win(), yp, xp - 1);
        nPos--;
      }
      szIn[nPos] = '\0';
      char *szMatch = sTabCompletion.szPartialMatch;

      if (sTabCompletion.vszPartialMatch.size() == 1)
      {
        // Only one match
        // Check if there is a space in the match
        if (strchr(szMatch, ' ') != NULL)
        {
          wprintw(winPrompt->Win(), "\"%s\" ", szMatch);
          sprintf(&szIn[nPos], "\"%s\" ", szMatch);
          nPos += strlen(szMatch) + 3;
        }
        else
        {
          wprintw(winPrompt->Win(), "%s ", szMatch);
          sprintf(&szIn[nPos], "%s ", szMatch);
          nPos += strlen(szMatch) + 1;
        }
        // Set nTabs to 1 so the array is cleared
        nTabs = 1;
      }
      else
      { 
        if (szMatch == 0) break;
        // Multiple matches
        Beep();
        // Check if there is a space in the match
        if (strchr(szMatch, ' ') != NULL)
        {
          wprintw(winPrompt->Win(), "\"%s", szMatch);
          sprintf(&szIn[nPos], "\"%s", szMatch);
          nPos += strlen(szMatch) + 1;
        }
        else
        {
          wprintw(winPrompt->Win(), "%s", szMatch);
          sprintf(&szIn[nPos], "%s", szMatch);
          nPos += strlen(szMatch);
        }
        // Set nTabs to 2 so the array is saved for the next tab hit
        nTabs = 2;
      }
      free(szMatch);
      winPrompt->RefreshWin();
      break;
    }

  case '\r':
    {
      if (nPos == 0)
        break;
      szIn[nPos] = '\0';
      // Strip trailing spaces
      while (nPos != 0 && szIn[nPos - 1] == ' ')
      {
        szIn[nPos - 1] = '\0';
        nPos--;
      }
      // Save the command in the history
      if (m_lCmdHistory.size() == 0 || strcmp(m_lCmdHistory.back(), szIn) != 0)
      {
        m_lCmdHistory.push_back(strdup(szIn));
        while(m_lCmdHistory.size() > MAX_CMD_HISTORY)
        {
          char *sz = m_lCmdHistory.front();
          free(sz);
          m_lCmdHistory.pop_front();
        }
      }
      m_lCmdHistoryIter = m_lCmdHistory.end();

      bool valid = true;
      if (szIn[0] != m_szCommandChar[0])
      {
        valid = ParseMacro(szIn);
      }

      if (valid)
      { // Regular command
        char *szArg = strchr(szIn, ' ');
        if (szArg != NULL)
        {
          *szArg = '\0';
          szArg++;
          // Remove any leading spaces
          while (*szArg == ' ')
            szArg++;
          if (*szArg == '\0')
            szArg = NULL;
        }
        unsigned short i;
        for (i = 0; i < NUM_COMMANDS; i++)
        {
          char szTempCmd[20];
          snprintf(szTempCmd, 20, "%c%s", m_szCommandChar[0], aCommands[i].szName);

          if (strncasecmp(szIn, szTempCmd, strlen(szIn)) == 0)
          {
            (this->*(aCommands[i].fProcessCommand))(szArg);
            break;
          }
        }
        if (i == NUM_COMMANDS)
          PrintBadInput(szIn);
      }

      // We are done with the current command line
      szIn[0] = '\0';
      nPos = 0;
      PrintPrompt();
      break;
    }

  case ' ':
    {
      if (nPos == 0)
      {
        Beep();
        break;
      }
      // Fall through
    }

  default:
    if (isprint(cIn))
    {
      szIn[nPos++] = (unsigned char)cIn;
      *winPrompt << (unsigned char)cIn;
    }
    else
      Beep();

  } // switch

  // Erase the sTabCompletion structure if no longer needed
  if (nTabs == 1)
  {
    vector<char *>::iterator iter;
    for (iter = sTabCompletion.vszPartialMatch.begin();
         iter != sTabCompletion.vszPartialMatch.end();
         iter++)
      free(*iter);
    sTabCompletion.vszPartialMatch.clear();
  }
  if (nTabs > 0)
    nTabs--;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::ParseMacro
 *-------------------------------------------------------------------------*/
bool CLicqConsole::ParseMacro(char *szMacro)
{
  MacroList::iterator iter;
  for (iter = listMacros.begin(); iter != listMacros.end(); iter++)
  {
    if (strcmp((*iter)->szMacro, szMacro) == 0)
    {
      sprintf(szMacro, "%c%s", m_szCommandChar[0], (*iter)->szCommand);
      break;
    }
  }

  if (iter == listMacros.end())
  {
    winMain->wprintf("%CNo such macro \"%A%s%Z\"\n", 16, A_BOLD,
                     szMacro, A_BOLD);
    szMacro[0] = '\0';
    return false;
  }

  return true;
}

/*---------------------------------------------------------------------------
 * CLicqConsole::SaveLastUser
 *-------------------------------------------------------------------------*/
void CLicqConsole::SaveLastUser(const char *szId, unsigned long nPPID)
{
  // Save this as the last user
  if (winMain->sLastContact.szId == 0 ||
      !(strcmp(szId, winMain->sLastContact.szId) == 0 &&
      nPPID == winMain->sLastContact.nPPID))
  {
    if (winMain->sLastContact.szId)
      free(winMain->sLastContact.szId);
    winMain->sLastContact.nPPID = nPPID;
    winMain->sLastContact.szId = strdup(szId);
    PrintStatus();
  }
}

/*---------------------------------------------------------------------------
 * CLicqConsole::CurrentGroupName
 *-------------------------------------------------------------------------*/
char *CLicqConsole::CurrentGroupName()
{
  static char szGroupName[64];

  if (m_nGroupType == GROUPS_USER)
  {
    if (m_nCurrentGroup == 0)
      strcpy(szGroupName, "All Users");
    else
    {
      GroupList *g = gUserManager.LockGroupList(LOCK_R);
      if (m_nCurrentGroup > g->size())
        strcpy(szGroupName, "Invalid Group");
      else
        strcpy(szGroupName, (*g)[m_nCurrentGroup - 1]);
      gUserManager.UnlockGroupList();
    }
  }
  else
  {
    strcpy(szGroupName, GroupsSystemNames[m_nCurrentGroup]);
  }
  return szGroupName;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::UserCommand_Info
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserCommand_Info(const char *szId, unsigned long nPPID, char *)
{
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  if (u == NULL)
    return;

  // First put this console into edit mode
  winMain->fProcessInput = &CLicqConsole::InputInfo;
  winMain->state = STATE_QUERY;
  winMain->data = new CData(szId, nPPID);

  winMain->wprintf("%C%A"
                   "(G)eneral Info\n"
                   "(M)ore Info\n"
                   "(W)ork Info\n"
                   "(A)bout Info\n"
                   "(U)pdate Info\n"
                   "for %s (%s)? %C%Z",
                   m_cColorQuery->nColor, m_cColorQuery->nAttr,
                   u->GetAlias(), szId, 8, A_BOLD);
  winMain->RefreshWin();
  gUserManager.DropUser(u);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::InputInfo
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputInfo(int cIn)
{
  CData *data = (CData *)winMain->data;

  winMain->wprintf("\n");

  switch(winMain->state)
  {
  case STATE_QUERY:
    // The input is done
    switch(tolower(cIn))
    {
    case 'g':
      PrintInfo_General(data->szId, data->nPPID);
      break;
    case 'm':
      PrintInfo_More(data->szId, data->nPPID);
      break;
    case 'w':
      PrintInfo_Work(data->szId, data->nPPID);
      break;
    case 'a':
      PrintInfo_About(data->szId, data->nPPID);
      break;
    case 'u':
      winMain->wprintf("%C%AUpdate info...", m_cColorInfo->nColor,
                       m_cColorInfo->nAttr);
      winMain->event = licqDaemon->icqRequestMetaInfo(data->szId);
      winMain->state = STATE_PENDING;
      return;
    case '\r':
      break;
    default:
      winMain->wprintf("%CInvalid key.\n", 16);
    }

    winMain->fProcessInput = &CLicqConsole::InputCommand;
    if (winMain->data != NULL)
    {
      delete winMain->data;
      winMain->data = NULL;
    }
    winMain->state = STATE_COMMAND;
    break;

  case STATE_PENDING:
    break;

  default:
    winMain->wprintf("%CInvalid state: %A%d%Z.\n", 16, A_BOLD, A_BOLD);
  }

}



/*---------------------------------------------------------------------------
 * CLicqConsole::UserCommand_View
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserCommand_View(const char *szId, unsigned long nPPID, char *)
{
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_W);
  if (u == NULL)
    return;

  if (u->NewMessages() > 0)
  {
    // Fetch the most recent event
    CUserEvent *e = u->EventPop();
    wattron(winMain->Win(), A_BOLD);
    for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
      waddch(winMain->Win(), ACS_HLINE);
    waddch(winMain->Win(), '\n');
    time_t t = e->Time();
    char *szTime = ctime(&t);
    szTime[16] = '\0';
    winMain->wprintf("%B%s from %b%s%B (%b%s%B) [%b%c%c%c%B]:\n%b%s\n",
                     e->Description(),
                     u->User() ? u->GetAlias() : "Server",
                     szTime, e->IsDirect() ? 'D' : '-',
                     e->IsMultiRec() ? 'M' : '-', e->IsUrgent() ? 'U' : '-',
                     e->Text());
    wattron(winMain->Win(), A_BOLD);
    for (unsigned short i = 0; i < winMain->Cols() - 10; i++)
      waddch(winMain->Win(), ACS_HLINE);
    waddch(winMain->Win(), '\n');
    winMain->RefreshWin();
    wattroff(winMain->Win(), A_BOLD);

    // Do we want to accept the file transfer?
    if (e->SubCommand() == ICQ_CMDxSUB_FILE)
      FileChatOffer(e, szId, nPPID);

    delete e;
    gUserManager.DropUser(u);
    //PrintUsers();
    //PrintStatus();
    ProcessSignal(new CICQSignal(SIGNAL_UPDATExUSER, USER_EVENTS, szId, nPPID));
  }
  else
  {
    gUserManager.DropUser(u);
    winMain->wprintf("No new events.\n");
  }

}


/*---------------------------------------------------------------------------
 * CLicqConsole::UserCommand_Remove
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserCommand_Remove(const char *szId, unsigned long nPPID, char *)
{
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  if (u == NULL)
    return;

  // First put this console into edit mode
  winMain->fProcessInput = &CLicqConsole::InputRemove;
  winMain->state = STATE_QUERY;
  winMain->data = new CData(szId, nPPID);

  winMain->wprintf("%C%ARemove %s (%s) from contact list (y/N)? %C%Z",
                   m_cColorQuery->nColor, m_cColorQuery->nAttr,
                   u->GetAlias(), szId, 8, A_BOLD);
  winMain->RefreshWin();
  gUserManager.DropUser(u);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::InputRemove
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputRemove(int cIn)
{
  CData *data = (CData *)winMain->data;
  char *sz;

  switch(winMain->state)
  {
  case STATE_QUERY:
    if ((sz = Input_Line(data->szQuery, data->nPos, cIn)) == NULL)
      return;
    // The input is done
    if (strncasecmp(data->szQuery, "yes", strlen(data->szQuery)) == 0)
    {
      licqDaemon->RemoveUserFromList(data->szId, data->nPPID);
      winMain->wprintf("%C%AUser removed.\n", m_cColorInfo->nColor,
                       m_cColorInfo->nAttr);
    }
    else
    {
      winMain->wprintf("%C%ARemoval aborted.\n", m_cColorInfo->nColor,
                       m_cColorInfo->nAttr);
    }
    winMain->fProcessInput = &CLicqConsole::InputCommand;
    if (winMain->data != NULL)
    {
      delete winMain->data;
      winMain->data = NULL;
    }
    winMain->state = STATE_COMMAND;
    break;

  default:
    winMain->wprintf("%CInvalid state: %A%d%Z.\n", 16, A_BOLD, A_BOLD);
  }

}



/*---------------------------------------------------------------------------
 * CLicqConsole::UserCommand_FetchAutoResponse
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserCommand_FetchAutoResponse(const char *szId, unsigned long nPPID, char *)
{
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  winMain->wprintf("%C%AFetching auto-response for %s (%s)...",
                   m_cColorInfo->nColor, m_cColorInfo->nAttr,
                   u->GetAlias(), szId);
  winMain->RefreshWin();
  gUserManager.DropUser(u);

  winMain->event = licqDaemon->icqFetchAutoResponse(szId, nPPID);
  // InputMessage just to catch the cancel key
  winMain->fProcessInput = &CLicqConsole::InputMessage;
  winMain->data = NULL;
  winMain->state = STATE_PENDING;
}





int StrToRange(char *sz, int nLast, int nStart)
{
  int n;
  if (*sz == '$')
  {
    n = nLast;
    sz++;
  }
  else if (*sz == '+' || *sz == '-')
  {
    n = nStart;
  }
  else
  {
    n = atoi(sz);
    while (isdigit(*sz))
      sz++;
  }
  STRIP(sz);

  if (*sz == '+')
  {
    sz++;
    STRIP(sz);
    n += atoi(sz);
    while (isdigit(*sz))
      sz++;
  }
  else if (*sz == '-')
  {
    sz++;
    STRIP(sz);
    n -= atoi(sz);
    while (isdigit(*sz))
      sz++;
  }
  STRIP(sz);
  if (*sz != '\0')
  {
    return -1;
  }
  return n;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::UserCommand_History
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserCommand_History(const char *szId, unsigned long nPPID, char *szArg)
{
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  if (u == NULL)
    return;
  HistoryList lHistory;
  if (!u->GetHistory(lHistory))
  {
    winMain->wprintf("Error loading history.\n");
    gUserManager.DropUser(u);
    return;
  }
  char *szFrom;
  if (gUserManager.FindOwner(szId, nPPID))
    szFrom = strdup("Server");
  else
    szFrom = strdup(u->GetAlias());
  gUserManager.DropUser(u);

  unsigned short nLast = lHistory.size();

  // Process the argument
  char *szStart = szArg;
  if (szStart == NULL)
  {
    if (nLast > 0 )
    {
      winMain->wprintf("%CYou must specify an event number. (1-%d)\n",
                       16, nLast);
    }
    else
    {
      winMain->wprintf("%CNo System Events.\n", 8);
    }
    free(szFrom);
    return;
  }

  char *szEnd = strchr(szStart, ',');
  int nStart, nEnd;

  if (szEnd != NULL)
  {
    *szEnd++ = '\0';
    STRIP(szEnd);
  }
  nStart = StrToRange(szStart, nLast, winMain->nLastHistory);
  if (nStart == -1)
  {
    winMain->wprintf("%CInvalid start range: %A%s\n", 16,
                     A_BOLD, szStart);
    free(szFrom);
    return;
  }
  else if (nStart > nLast || nStart < 1)
  {
    winMain->wprintf("%CStart value out of range, history contains %d events.\n",
                     16, nLast);
    free(szFrom);
    return;
  }

  if (szEnd != NULL)
  {
    nEnd = StrToRange(szEnd, nLast, nStart);
    if (nEnd == -1)
    {
      winMain->wprintf("%CInvalid end range: %A%s\n", 16,
                       A_BOLD, szEnd);
      free(szFrom);
      return;
    }
    else if (nEnd > nLast || nEnd < 1)
    {
      winMain->wprintf("%CEnd value out of range, history contains %d events.\n",
                       16, nLast);
      free(szFrom);
      return;
    }
  }
  else
  {
    nEnd = nStart;
  }

  winMain->nLastHistory = nEnd;
  PrintHistory(lHistory, nStart - 1, nEnd - 1, szFrom);
  free(szFrom);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::UserCommand_Msg
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserCommand_Msg(const char *szId, unsigned long nPPID, char *)
{
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  if (u == NULL)
    return;

  // First put this console into edit mode
  winMain->fProcessInput = &CLicqConsole::InputMessage;
  winMain->state = STATE_MLE;
  winMain->data = new DataMsg(szId, nPPID);

  winMain->wprintf("%BEnter message to %b%s%B (%b%s%B):\n", u->GetAlias(),
                   szId);
  winMain->RefreshWin();
  gUserManager.DropUser(u);
}


/*---------------------------------------------------------------------------
 * SendDirect
 *-------------------------------------------------------------------------*/
static bool SendDirect(const char *szId, unsigned long nPPID, char c)
{
  bool bDirect = (c != 's');
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  if (u != NULL)
  {
    if (u->SocketDesc(ICQ_CHNxNONE) == -1 &&
        (u->Ip() == 0 || u->Port() == 0 || u->StatusOffline()))
      bDirect = false;
    else if (u->SendServer() && c != 'd' && c != 'u')
      bDirect = false;
  }
  else
  {
    bDirect = false;
  }
  gUserManager.DropUser(u);
  return bDirect;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::InputMessage
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputMessage(int cIn)
{
  DataMsg *data = (DataMsg *)winMain->data;
  char *sz;

  switch(winMain->state)
  {
  case STATE_PENDING:
    if (cIn == CANCEL_KEY)
      licqDaemon->CancelEvent(winMain->event);
    return;

  case STATE_MLE:
    {
      // Process the character as a multi-line edit window
      // If we get NULL back then we aren't done yet
      if ((sz = Input_MultiLine(data->szMsg, data->nPos, cIn)) == NULL)
        return;

      // The input is done, so process it, sz points to the '.'
      if (*sz == ',')
      {
        winMain->fProcessInput = &CLicqConsole::InputCommand;
        if (winMain->data != NULL)
        {
          delete winMain->data;
          winMain->data = NULL;
        }
        winMain->state = STATE_COMMAND;
        winMain->wprintf("%C%AMessage aborted.\n", m_cColorInfo->nColor,
                         m_cColorInfo->nAttr);
        return;
      }
      sz--;
      *sz = '\0';
      sz += 2;
      bool bDirect = SendDirect(data->szId, data->nPPID, *sz);
      winMain->wprintf("%C%ASending message %s...", m_cColorInfo->nColor,
                       m_cColorInfo->nAttr,
                       !bDirect ? "through the server" : "direct");
      winMain->event = licqDaemon->ProtoSendMessage(data->szId, data->nPPID, data->szMsg,
                       bDirect, *sz == 'u');
      winMain->state = STATE_PENDING;
      break;
    }

    // If we are here then direct failed and we asked if send through server
  case STATE_QUERY:
    if ((sz = Input_Line(data->szQuery, data->nPos, cIn)) == NULL)
      return;
    // The input is done
    if (strncasecmp(data->szQuery, "yes", strlen(data->szQuery)) == 0)
    {
      winMain->wprintf("%C%ASending message through the server...",
                       m_cColorInfo->nColor, m_cColorInfo->nAttr);
      winMain->event = licqDaemon->ProtoSendMessage(data->szId, data->nPPID, data->szMsg,
                       false, false);
      winMain->state = STATE_PENDING;
    }
    else
    {
      winMain->fProcessInput = &CLicqConsole::InputCommand;
      delete winMain->data;
      winMain->data = NULL;
      winMain->state = STATE_COMMAND;
    }
    break;

  default:
    winMain->wprintf("%CInvalid state: %A%d%Z.\n", 16, A_BOLD, A_BOLD);
  }

}

/*---------------------------------------------------------------------------
 * CLicqConsole::UserCommand_SendFile
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserCommand_SendFile(const char *szId, unsigned long nPPID, char *)
{
  // Get the file name
  winMain->fProcessInput = &CLicqConsole::InputSendFile;
  winMain->state = STATE_LE;
  winMain->data = new DataSendFile(szId, nPPID);

  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  winMain->wprintf("%BEnter file to send to %b%s%B (%b%s%B):\n",
                   u->GetAlias(), szId);
  winMain->RefreshWin();
  gUserManager.DropUser(u);
}

/*---------------------------------------------------------------------------
 * CLicqConsole::InputSendFile
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputSendFile(int cIn)
{
  DataSendFile *data = (DataSendFile *)winMain->data;
  char *sz;

  switch(winMain->state)
  {
  case STATE_PENDING:
    if(cIn == CANCEL_KEY)
      licqDaemon->CancelEvent(winMain->event);
    return;

  case STATE_LE:
    {
      // If we get NULL back, then we're not done yet
      if((sz = Input_Line(data->szFileName, data->nPos, cIn)) == NULL)
        return;

      // Check to make sure the file exists, if it doesn't then tell the
      // user it doesn't and quit sending the file.
      ifstream check_file(data->szFileName);

      if(!check_file)
      {
        winMain->fProcessInput = &CLicqConsole::InputCommand;

        if(winMain->data != NULL)
        {
          delete winMain->data;
          winMain->data = NULL;
        }

        winMain->state = STATE_COMMAND;
        winMain->wprintf("%C%AFile Transfer aborted: No such file\n",
                         m_cColorInfo->nColor, m_cColorInfo->nAttr);
        return;
      }

      check_file.close();

      // The input is done
      winMain->wprintf("%BEnter description:\n");
      winMain->state = STATE_MLE;
      data->nPos = 0;
      break;
    }
  case STATE_MLE:
    {
      // If we get NULL back, then we're not odne yet
      if((sz = Input_MultiLine(data->szDescription, data->nPos, cIn)) == NULL)
        return;

      // The input is done, so process it, sz points to '.'
      if(*sz == ',')
      {
        winMain->fProcessInput = &CLicqConsole::InputCommand;

        if(winMain->data != NULL)
        {
          delete winMain->data;
          winMain->data = NULL;
        }

        winMain->state = STATE_COMMAND;
        winMain->wprintf("%C%AFile Transfer aborted.\n",
                         m_cColorInfo->nColor, m_cColorInfo->nAttr);
        return;
      }

      *sz = '\0';
      sz++;
      bool bDirect = SendDirect(data->szId, data->nPPID, *sz);
      bDirect = true; // XXX hack
      winMain->wprintf("%C%ASending File %s...",
                       m_cColorInfo->nColor, m_cColorInfo->nAttr,
                       !bDirect ? "trhough the server" : "direct");

      ConstFileList lFileList;
      lFileList.push_back(strdup(data->szFileName));

      winMain->event = licqDaemon->icqFileTransfer(strtoul(data->szId, (char **)NULL, 10),
              data->szFileName, data->szDescription, lFileList, ICQ_TCPxMSG_NORMAL,
                       !bDirect);
      break;
    }
  case STATE_QUERY:
    break;
  case STATE_COMMAND:
    break;
  }
}

/*---------------------------------------------------------------------------
 * CLicqConsole::UserCommand_SetAutoResponse
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserCommand_SetAutoResponse(const char *szId, unsigned long nPPID, char *)
{
  // First put this console into edit mode
  winMain->fProcessInput = &CLicqConsole::InputAutoResponse;
  winMain->state = STATE_MLE;
  winMain->data = new DataAutoResponse();

  winMain->wprintf("%BEnter auto response:\n");
  winMain->RefreshWin();
}

/*---------------------------------------------------------------------------
 * CLicqConsole::InputAutoResponse
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputAutoResponse(int cIn)
{
  DataAutoResponse *data = (DataAutoResponse *)winMain->data;
  char *sz;

  switch(winMain->state)
  {
  case STATE_MLE:
    // Process the character as a multi-line edit window
    // If we get NULL back then we aren't done yet
    if ((sz = Input_MultiLine(data->szRsp, data->nPos, cIn)) == NULL)
      return;

    // The input is done, so process it, sz points to the '.'
    if (*sz == ',')
    {
      winMain->wprintf("%C%AAuto-response set aborted.\n",
                       m_cColorInfo->nColor, m_cColorInfo->nAttr);
    }
    else
    {
      *sz = '\0';
      ICQOwner *o = gUserManager.FetchOwner(LOCK_W);
      o->SetAutoResponse(data->szRsp);
      gUserManager.DropOwner();
      winMain->wprintf("%C%AAuto-response set.\n",
                       m_cColorInfo->nColor, m_cColorInfo->nAttr);
    }
    delete winMain->data;
    winMain->data = NULL;
    winMain->fProcessInput = &CLicqConsole::InputCommand;
    winMain->state = STATE_COMMAND;
    break;

  default:
    winMain->wprintf("%CInvalid state: %A%d%Z.\n", 16, A_BOLD, A_BOLD);
  }

}


/*---------------------------------------------------------------------------
 * CLicqConsole::UserCommand_Url
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserCommand_Url(const char *szId, unsigned long nPPID, char *)
{
  // First put this console into edit mode
  winMain->fProcessInput = &CLicqConsole::InputUrl;
  winMain->state = STATE_LE;
  winMain->data = new DataUrl(szId, nPPID);

  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  winMain->wprintf("%BEnter URL to %b%s%B (%b%ld%B): ",
                   u->GetAlias(), szId);
  winMain->RefreshWin();
  gUserManager.DropUser(u);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::InputUrl
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputUrl(int cIn)
{
  DataUrl *data = (DataUrl *)winMain->data;
  char *sz;

  switch(winMain->state)
  {
  case STATE_PENDING:
    if (cIn == CANCEL_KEY)
      licqDaemon->CancelEvent(winMain->event);
    return;

  case STATE_LE:
    // Process the character as a single-line edit window
    // If we get NULL back then we aren't done yet
    if ((sz = Input_Line(data->szUrl, data->nPos, cIn)) == NULL)
      return;
    // The input is done
    winMain->wprintf("%BEnter description:\n");
    winMain->state = STATE_MLE;
    data->nPos = 0;
    break;

  case STATE_MLE:
    {
      // Process the character as a multi-line edit window
      // If we get NULL back then we aren't done yet
      if ((sz = Input_MultiLine(data->szDesc, data->nPos, cIn)) == NULL)
        return;

      // The input is done, so process it, sz points to the '.'
      if (*sz == ',')
      {
        winMain->fProcessInput = &CLicqConsole::InputCommand;
        if (winMain->data != NULL)
        {
          delete winMain->data;
          winMain->data = NULL;
        }
        winMain->state = STATE_COMMAND;
        winMain->wprintf("%C%AURL aborted.\n",
                         m_cColorInfo->nColor, m_cColorInfo->nAttr);
        return;
      }
      *sz = '\0';
      sz++;
      bool bDirect = SendDirect(data->szId, data->nPPID, *sz);
      winMain->wprintf("%C%ASending URL %s...",
                       m_cColorInfo->nColor, m_cColorInfo->nAttr,
                       !bDirect ? "through the server" : "direct");
      winMain->event = licqDaemon->ProtoSendUrl(data->szId, data->nPPID, data->szUrl,
                                              data->szDesc,
                                              bDirect, *sz == 'u');
      winMain->state = STATE_PENDING;
      break;
    }

    // If we are here then direct failed and we asked if send through server
  case STATE_QUERY:
    if ((sz = Input_Line(data->szQuery, data->nPos, cIn)) == NULL)
      return;
    // The input is done
    if (strncasecmp(data->szQuery, "yes", strlen(data->szQuery)) == 0)
    {
      winMain->wprintf("%C%ASending URL through the server...",
                       m_cColorInfo->nColor, m_cColorInfo->nAttr);
      winMain->event = licqDaemon->ProtoSendUrl(data->szId, data->nPPID, data->szUrl,
                                              data->szDesc, false, false);
      winMain->state = STATE_PENDING;
    }
    else
    {
      winMain->fProcessInput = &CLicqConsole::InputCommand;
      delete winMain->data;
      winMain->data = NULL;
      winMain->state = STATE_COMMAND;
    }
    break;

  default:
    winMain->wprintf("%CInvalid state: %A%d%Z.\n", 16, A_BOLD, A_BOLD);
  }

}

/*---------------------------------------------------------------------------
 * CLicqConsole::UserCommand_Sms
 *-------------------------------------------------------------------------*/
void CLicqConsole::UserCommand_Sms(const char *szId, unsigned long nPPID, char *)
{
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);
  if (u == NULL)
    return;

  // First put this console into edit mode
  winMain->fProcessInput = &CLicqConsole::InputSms;
  winMain->state = STATE_MLE;
  winMain->data = new DataSms(szId, nPPID);
  winMain->wprintf("%BEnter SMS to %b%s%B (%b%s%B):\n", u->GetAlias(),
                   u->GetCellularNumber());
  winMain->RefreshWin();
  gUserManager.DropUser(u);
}

/*---------------------------------------------------------------------------
 * CLicqConsole::InputSms
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputSms(int cIn)
{
  DataSms *data = (DataSms *)winMain->data;
  char *sz;

  switch(winMain->state)
  {
  case STATE_PENDING:
    if (cIn == CANCEL_KEY)
      licqDaemon->CancelEvent(winMain->event);
    return;

  case STATE_MLE:
    {
      // Process the character as a multi-line edit window
      // If we get NULL back then we aren't done yet
      if ((sz = Input_MultiLine(data->szMsg, data->nPos, cIn)) == NULL)
        return;

      // The input is done, so process it, sz points to the '.'
      if (*sz == ',')
      {
        winMain->fProcessInput = &CLicqConsole::InputCommand;
        if (winMain->data != NULL)
        {
          delete winMain->data;
          winMain->data = NULL;
        }
        winMain->state = STATE_COMMAND;
        winMain->wprintf("%C%ASMS aborted.\n", m_cColorInfo->nColor,
                         m_cColorInfo->nAttr);
        return;
      }
      *sz = '\0';
      sz++;
      ICQUser *u = gUserManager.FetchUser(data->szId, data->nPPID, LOCK_R);
      winMain->wprintf("%C%ASending SMS to %s ...", m_cColorInfo->nColor,
                       m_cColorInfo->nAttr, u->GetCellularNumber());
      winMain->event = licqDaemon->icqSendSms(u->GetCellularNumber(),
                       data->szMsg, strtoul(data->szId, (char **)NULL, 0));
      gUserManager.DropUser(u);
      winMain->state = STATE_PENDING;
      break;
    }

  default:
    winMain->wprintf("%CInvalid state: %A%d%Z.\n", 16, A_BOLD, A_BOLD);
  }

}


/*---------------------------------------------------------------------------
 * CLicqConsole::InputAuthorize
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputAuthorize(int cIn)
{
  DataMsg *data = (DataMsg *)winMain->data;
  char *sz;

  switch(winMain->state)
  {
  case STATE_MLE:
    {
      // Process the character as a multi-line edit window
      // If we get NULL back then we aren't done yet
      if ((sz = Input_MultiLine(data->szMsg, data->nPos, cIn)) == NULL)
        return;

      // The input is done, so process it, sz points to the '.'
      if (*sz == ',')
      {
        winMain->fProcessInput = &CLicqConsole::InputCommand;
        if (winMain->data != NULL)
        {
          delete winMain->data;
          winMain->data = NULL;
        }
        winMain->state = STATE_COMMAND;
        winMain->wprintf("%C%AAuthorization aborted.\n", m_cColorInfo->nColor,
                         m_cColorInfo->nAttr);
        return;
      }
      *sz = '\0';
      if (data->bUrgent)
      {
        winMain->wprintf("%C%AGranting authorizing to %s...", m_cColorInfo->nColor,
                         m_cColorInfo->nAttr, data->szId);
        winMain->event = licqDaemon->ProtoAuthorizeGrant(data->szId, data->nPPID, data->szMsg);
      }
      else
      {
        winMain->wprintf("%C%ARefusing authorizing to %s...", m_cColorInfo->nColor,
                         m_cColorInfo->nAttr, data->szId);
        winMain->event = licqDaemon->ProtoAuthorizeRefuse(data->szId, data->nPPID, data->szMsg);
      }

      winMain->fProcessInput = &CLicqConsole::InputCommand;
      if (winMain->data != NULL)                           
      {
        delete winMain->data;
        winMain->data = NULL;
      }
      winMain->state = STATE_COMMAND;
      break;
    }

  default:
    winMain->wprintf("%CInvalid state: %A%d%Z.\n", 16, A_BOLD, A_BOLD);
  }

}



/*---------------------------------------------------------------------------
 * CLicqConsole::Input_Line
 *-------------------------------------------------------------------------*/
char *CLicqConsole::Input_Line(char *sz, unsigned short &n, int cIn,
                               bool bEcho)
{
  // Now check for keys
  switch (cIn)
  {
  case KEY_PPAGE:
  case KEY_NPAGE:
  case '\t':
    break;

  case KEY_BACKSPACE:
  case KEY_DC:
  case KEY_LEFT:
    {
      if (n == 0)
        break;
      int yp, xp;
      getyx(winMain->Win(), yp, xp);
      mvwdelch(winMain->Win(), yp, xp - 1);
      winMain->RefreshWin();
      n--;
      break;
    }

  case '\r':
    {
      // Print the new line
      *winMain << '\n';
      sz[n] = '\0';
      return sz;
      break;
    }

  default:
    if (isprint(cIn))
    {
      sz[n++] = (unsigned char)cIn;
      if (bEcho)
        *winMain << (unsigned char)cIn;
    }
    else
    {
      Beep();
    }

  } // switch

  return NULL;

}


/*---------------------------------------------------------------------------
 * CLicqConsole::Input_MultiLine
 *-------------------------------------------------------------------------*/
char *CLicqConsole::Input_MultiLine(char *sz, unsigned short &n, int cIn)
{
  static int nLinePos[MAX_CON + 1] = {0};

  // Now check for keys
  switch (cIn)
  {
  case KEY_PPAGE:
  case KEY_NPAGE:
  case '\t':
    break;

  case KEY_BACKSPACE:
  case KEY_DC:
  case KEY_LEFT:
    {
      if (n == 0)
        break;
      int yp, xp;
      getyx(winMain->Win(), yp, xp);
      if (xp == 0)
      {
        mvwdelch(winMain->Win(), yp - 1, nLinePos[m_nCon]);
      }
      else
        mvwdelch(winMain->Win(), yp, xp - 1);
      winMain->RefreshWin();
      n--;
      break;
    }

  case '\r':
    {
      // Print the new line
      int yp;
      getyx(winMain->Win(), yp, nLinePos[m_nCon]);
      *winMain << '\n';
      // Check if the line only contains a '.'
      sz[n] = '\0';
      char *szNL = strrchr(sz, '\n');
      if (szNL == NULL)
        szNL = &sz[0];
      else
        szNL++;
      if ( (( *szNL == '.' || *szNL == ',' ) && strlen( szNL ) == 1) ||
           !strcmp(szNL, ".s") || !strcmp(szNL, ".d") || !strcmp(szNL, ".u"))
        return szNL;

      sz[n++] = '\n';
      break;
    }

  default:
    if (isprint(cIn))
    {
      sz[n++] = (unsigned char)cIn;
      *winMain << (unsigned char)cIn;
    }
    else
      Beep();
    break;

  } // switch

  return NULL;

}


/*---------------------------------------------------------------------------
 * CLicqConsole::Command_Search
 *-------------------------------------------------------------------------*/
void CLicqConsole::Command_Search()
{
  // Get the input now
  winMain->fProcessInput = &CLicqConsole::InputSearch;
  winMain->state = STATE_LE;
  winMain->data = new DataSearch();

  winMain->wprintf("%A%CSearch for User (leave field blank if unknown)\n"
                   "Enter uin: ",
                   m_cColorQuery->nAttr, m_cColorQuery->nColor);

  return;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::InputSearch
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputSearch(int cIn)
{
  DataSearch *data = (DataSearch *)winMain->data;
  char *sz;

  switch(winMain->state)
  {
  case STATE_PENDING:
    {
      if (cIn == CANCEL_KEY)
      {
        if (winMain->event != 0)
          licqDaemon->CancelEvent(winMain->event);
      }
      return;
    }

  case STATE_LE:
    {
      switch (data->nState)
      {
        // UIN
      case 0:
        {
          // If we get NULL back, then we're not done yet
          if ((sz = Input_Line(data->szQuery, data->nPos, cIn)) == NULL)
            return;

          // Back to 0 for you!
          data->nPos = 0;

          data->szId = sz;

          if (data->szId != 0)
          {
            winMain->wprintf("%C%ASearching:\n",
                             m_cColorInfo->nColor, m_cColorInfo->nAttr);

            winMain->event = licqDaemon->icqSearchByUin(strtoul(data->szId, (char **)NULL, 10));
            winMain->state = STATE_PENDING;

            return;
          }

          winMain->wprintf("%A%CAlias: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 1;

          return;
        }

      case 1:
        {
          // If we get NULL back, then we're not done yet
          if ((sz = Input_Line(data->szAlias, data->nPos, cIn)) == NULL)
            return;

          // Back to 0 for you!
          data->nPos = 0;

          winMain->wprintf("%A%CFirst Name: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 2;

          return;
        }

        // First Name
      case 2:
        {
          // If we get NULL back, then we're not done yet
          if ((sz = Input_Line(data->szFirstName, data->nPos, cIn)) == NULL)
            return;

          // Back to 0 for you!
          data->nPos = 0;

          winMain->wprintf("%A%CLast Name: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 3;

          return;
        }

        // Last Name
      case 3:
        {
          // If we get NULL back, then we're not done yet
          if ((sz = Input_Line(data->szLastName, data->nPos, cIn)) == NULL)
            return;

          // Back to 0 for you!
          data->nPos = 0;

          winMain->wprintf("%A%CEmail: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 4;

          return;
        }

        // Email
      case 4:
        {
          if ((sz = Input_Line(data->szEmail, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;
          winMain->wprintf("%A%CMinimum Age: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 5;
          return;
        }

        // Min age
      case 5:
        {
          if ((sz = Input_Line(data->szQuery, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;
          data->nMinAge = atol(data->szQuery);
          winMain->wprintf("%A%CMaximum Age: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 6;
          return;
        }

        // Max age
      case 6:
        {
          if ((sz = Input_Line(data->szQuery, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;
          data->nMaxAge = atol(data->szQuery);
          winMain->wprintf("%A%CGender [?]: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 7;
          return;
        }

      case 7:
        {
          if ((sz = Input_Line(data->szQuery, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;
          if (data->szQuery[0] == '?')
          {
            winMain->wprintf("%C0 %A-%Z Unknown\n"
                             "1 %A-%Z Female\n"
                             "2 %A-%Z Male\n"
                             "%A%CGender: ",
                             8, A_BOLD, A_BOLD, A_BOLD, A_BOLD,
                             A_BOLD, A_BOLD, m_cColorQuery->nAttr, m_cColorQuery->nColor);
            return;
          }
          data->nGender = atol(data->szQuery);
          winMain->wprintf("%A%CLanguage [?]: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 8;
          return;
        }

      case 8:
        {
          if ((sz = Input_Line(data->szQuery, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;
          const SLanguage *l = NULL;
          if (data->szQuery[0] != '?' && data->szQuery[0] != '\0')
          {
            if (isdigit(data->szQuery[0]))
              l = GetLanguageByCode(atol(data->szQuery));
            else
              l = GetLanguageByName(data->szQuery);
          }

          // Print out list of languages
          if (l == NULL && data->szQuery[0] != '\0')
          {
            for (unsigned short i = 0; i < NUM_LANGUAGES; i++)
            {
              winMain->wprintf("%C%s %A(%Z%d%A)%s%Z",
                               8, gLanguages[i].szName,
                               A_BOLD, A_BOLD, gLanguages[i].nCode,
                               A_BOLD,
                               i == NUM_LANGUAGES - 1 ? "\n" : ", ",
                               A_BOLD);
            }
            winMain->wprintf("%A%CLanguage [?]: ",
                             m_cColorQuery->nAttr, m_cColorQuery->nColor);
            return;
          }

          data->nLanguage = (l == NULL ? LANGUAGE_UNSPECIFIED : l->nCode);
          winMain->wprintf("%A%CCity: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 9;
          return;
        }

      case 9:
        {
          if ((sz = Input_Line(data->szCity, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;
          winMain->wprintf("%A%CState: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 10;
          return;
        }

      case 10:
        {
          if ((sz = Input_Line(data->szState, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;
          winMain->wprintf("%A%CCountry [?]: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 11;
          return;
        }

      case 11:
        {
          if ((sz = Input_Line(data->szQuery, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;
          const SCountry
          *c = NULL;
          if (data->szQuery[0] != '?' && data->szQuery[0] != '\0')
          {
            if (isdigit(data->szQuery[0]))
              c = GetCountryByCode(atol(data->szQuery));
            else
              c = GetCountryByName(data->szQuery);
          }

          // Print out list of countries
          if (c == NULL && data->szQuery[0] != '\0')
          {
            for (unsigned short i = 0; i < NUM_COUNTRIES; i++)
            {
              winMain->wprintf("%C%s %A(%Z%d%A)%s%Z",
                               8, gCountries[i].szName,
                               A_BOLD, A_BOLD, gCountries[i].nCode,
                               A_BOLD,
                               i == NUM_COUNTRIES - 1 ? "\n" : ", ",
                               A_BOLD);
            }
            winMain->wprintf("%A%CCountry [?]: ",
                             m_cColorQuery->nAttr, m_cColorQuery->nColor);
            return;
          }

          data->nCountryCode = (c == NULL ? COUNTRY_UNSPECIFIED : c->nCode);
          winMain->wprintf("%A%CCompany Name: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 12;
          return;
        }

      case 12:
        {
          if ((sz = Input_Line(data->szCoName, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;
          winMain->wprintf("%A%CCompany Department: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 13;
          return;
        }

      case 13:
        {
          if ((sz = Input_Line(data->szCoDept, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;
          winMain->wprintf("%A%CCompany Position: ", m_cColorQuery->nAttr, m_cColorQuery->nColor);
          data->nState = 14;
          return;
        }

      case 14:
        {
          // If we get NULL back, then we're not done yet
          if ((sz = Input_Line(data->szCoPos, data->nPos, cIn)) == NULL)
            return;

          // Back to 0 for you!
          data->nPos = 0;

          winMain->wprintf("%A%COnline Only [y/n]: ", m_cColorQuery->nAttr,
                           m_cColorQuery->nColor);
          data->nState = 15;
          return;
        }

      case 15:
        {
          if ((sz = Input_Line(data->szQuery, data->nPos, cIn)) == NULL)
            return;
          data->nPos = 0;

          if (data->szQuery[0] == 'y' || data->szQuery[0] == 'Y')
            data->bOnlineOnly = true;

          /*if (data->szAlias[0] == '\0' && data->szFirstName[0] == '\0' &&
              data->szLastName[0] =='\0' && data->szEmail[0] == '\0')
          {
            winMain->fProcessInput = &CLicqConsole::InputCommand;
            if (winMain->data != NULL)
            {
              delete winMain->data;
              winMain->data = NULL;
            }
            winMain->state = STATE_COMMAND;
            winMain->wprintf("%C%ASearch aborted.\n",
                             m_cColorInfo->nColor, m_cColorInfo->nAttr);
            return;
        }*/

          winMain->wprintf("%C%ASearching:\n",
                           m_cColorInfo->nColor, m_cColorInfo->nAttr);

          /*winMain->event = licqDaemon->icqSearchByInfo(data->szAlias, data->szFirstName,
           data->szLastName, data->szEmail);*/
          winMain->event = licqDaemon->icqSearchWhitePages(data->szFirstName,
                           data->szLastName, data->szAlias, data->szEmail,
                           data->nMinAge, data->nMaxAge, data->nGender, data->nLanguage,
                           data->szCity, data->szState, data->nCountryCode,
                           data->szCoName, data->szCoDept, data->szCoPos, "", data->bOnlineOnly);
          winMain->state = STATE_PENDING;

          return;
        }

      }
    }

  default:
    break;
  }
}


/*---------------------------------------------------------------------------
 * CLicqConsole::RegistrationWizard
 *-------------------------------------------------------------------------*/
void CLicqConsole::RegistrationWizard()
{

  // Get the input now
  winMain->fProcessInput = &CLicqConsole::InputRegistrationWizard;
  winMain->state = STATE_LE;
  winMain->data = new DataRegWizard();

  winMain->wprintf("%A%CWelcome to the Licq Console Registration Wizard\n\nPress 1 to register a new UIN\nPress 2 if you have a UIN and password\n\n",
                   A_BOLD, 24);

  return;
}


/*---------------------------------------------------------------------------
 * CLicqConsole::InputRegistrationWizard
 *-------------------------------------------------------------------------*/
void CLicqConsole::InputRegistrationWizard(int cIn)
{
  DataRegWizard *data = (DataRegWizard *)winMain->data;
  char *sz;

  switch(winMain->state)
  {
  case STATE_PENDING:
    {
      if(cIn == CANCEL_KEY)
      {
        if(winMain->event != 0)
          licqDaemon->CancelEvent(winMain->event);
      }
      return;
    }

  case STATE_LE:
    {
      // Make sure we need to get this
      if (data->nState == 0)
      {
        // If we get NULL back, then we're not done yet
        if ((sz = Input_Line(data->szOption, data->nPos, cIn)) == NULL)
          return;

        // Back to 0 for you!
        data->nPos = 0;
        if (data->szOption[0] == '1')
        {
          winMain->wprintf("Please enter your password: ");
          data->nState = 1;
        }
        else if(data->szOption[0] == '2')
        {
          winMain->wprintf("Please enter your UIN: ");
          data->nState = 10;
        }

        return;
      }

      // The option to register a new UIN or use an existing is in szOption now
      switch(data->szOption[0])
      {
      case '1':
        {
          // Register a new UIN
          if (data->nState == 1)
          {
            if ((sz = Input_Line(data->szPassword1, data->nPos, cIn, false)) == NULL)
              return;

            // Time to go on to the next state
            data->nState = 2;
            data->nPos = 0;

            winMain->wprintf("Verify Password: ");
            break;
          }

          if(data->nState == 2)
          {
            if((sz = Input_Line(data->szPassword2, data->nPos, cIn, false))
                == NULL)
              return;

            if(strcasecmp(data->szPassword1, data->szPassword2) != 0)
            {
              winMain->wprintf("Passwords do not match!\nPlease enter your password: ");
              data->nState = 1;
              data->nPos = 0;
              return;
            }

            winMain->state = STATE_QUERY;
            winMain->wprintf("\nSave password? (y/N) ");
          }
          break;
        }

      case '2':
        {
          // Use an existing
          if (data->nState == 10)
          {
            if ((sz = Input_Line(data->szUin, data->nPos, cIn)) == NULL)
              return;

            data->nState = 11;
            data->nPos = 0;

            winMain->wprintf("Enter your password: ");
            return;
          }

          if(data->nState == 11)
          {
            if ((sz = Input_Line(data->szPassword1, data->nPos, cIn, false)) == NULL)
              return;

            data->nState = 12;
            data->nPos = 0;

            winMain->wprintf("Verify your password: ");
            return;
          }

          if (data->nState == 12)
          {
            if ((sz = Input_Line(data->szPassword2, data->nPos, cIn, false)) == NULL)
              return;

            // Compare the 2 passwords
            if (strcasecmp(data->szPassword1, data->szPassword2) != 0)
            {
              winMain->wprintf("Passwords do not match!\nPlease enter your password: ");
              data->nState = 11;
              data->nPos = 0;
              return;
            }

            // Passwords match if we are this far, now set up the new user
            winMain->wprintf("Registration complete for user %s\n",data->szUin);
            gUserManager.SetOwnerUin(atol(data->szUin));
            ICQOwner *owner = gUserManager.FetchOwner(LOCK_W);
            owner->SetPassword(data->szPassword1);
            gUserManager.DropOwner();

            winMain->wprintf("Save password? (y/N) ");
            winMain->state = STATE_QUERY;
          }
          break;
        }

      default:
        winMain->wprintf("Invalid option: %c\n", data->szOption[0]);
      }
      break;
    }
  
  case STATE_QUERY:
  {
    ICQOwner *o = gUserManager.FetchOwner(LOCK_W);
    o->SetSavePassword(tolower(cIn) == 'y');
    gUserManager.DropOwner();
    
    if (data->szOption[0] == '1')
    {
      winMain->wprintf("\nRegistering you as a new user...\n");
      licqDaemon->icqRegister(data->szPassword1);
      winMain->state = STATE_PENDING;
    }
    else
    {
      winMain->wprintf("\n%ADone. Awaiting commands.%Z\n", A_BOLD, A_BOLD);
      winMain->state = STATE_COMMAND;
      winMain->fProcessInput = &CLicqConsole::InputCommand;
      PrintStatus();
    }
  }
  
  default:
    break;
  }
}

/*---------------------------------------------------------------------------
 * CLicqConsole::FileChatOffer
 *-------------------------------------------------------------------------*/
void CLicqConsole::FileChatOffer(CUserEvent *e, const char *_szId, unsigned long _nPPID)
{
  CEventFile *f = (CEventFile *)e;
  // Get y or n
  winMain->fProcessInput = &CLicqConsole::InputFileChatOffer;
  winMain->state = STATE_QUERY;
  winMain->data = new DataFileChatOffer(f, _szId, _nPPID);
  winMain->wprintf("%C%ADo you wish to accept this request? (y/N) %C%Z",
                   m_cColorQuery->nColor, m_cColorQuery->nAttr, 8, A_BOLD);
  winMain->RefreshWin();

  return;
}

/*--------------------------------------------------------------------------
 * CLicqConsole::InputFileChatOffer
 *------------------------------------------------------------------------*/
void CLicqConsole::InputFileChatOffer(int cIn)
{
  DataFileChatOffer *data = (DataFileChatOffer *)winMain->data;
  char *sz;
  CEventFile *f = data->f;

  switch(winMain->state)
  {
  case STATE_QUERY:
    {
      switch(tolower(cIn))
      {
      case 'y':
        {
          winMain->wprintf("%C%A\nAccepting file\n", 24, A_BOLD);

          // Make the ftman
          CFileTransferManager *ftman = new CFileTransferManager(licqDaemon,
                                        strtoul(data->szId, (char **)NULL, 10));
          ftman->SetUpdatesEnabled(1);
          m_lFileStat.push_back(ftman);

          // Now watch the file pipe
          FD_SET(ftman->Pipe(), &fdSet);


          // Accept the file
          const char *home = getenv("HOME");
          ftman->ReceiveFiles(home);
          licqDaemon->icqFileTransferAccept(strtoul(data->szId, (char **)NULL, 10), ftman->LocalPort(),
                                            f->Sequence(), f->MessageID(), f->IsDirect(),
                                            f->FileDescription(), f->Filename(), f->FileSize());
          winMain->fProcessInput = &CLicqConsole::InputCommand;

          if(winMain->data)
            delete winMain->data;

          break;
        }

      case 'n':
      default:
        {
          winMain->state = STATE_MLE;
          winMain->wprintf("\n%BEnter a refusal reason:\n");
          return;
        }
      }

      break;
    }

  case STATE_MLE:
    {
      if((sz = Input_MultiLine(data->szReason, data->nPos, cIn)) == NULL)
        return;

      // Don't send the "."
      data->szReason[data->nPos - 1] = '\0';

      // XXX hack
      unsigned long dummy[2] = { 0, 0 };
      licqDaemon->icqFileTransferRefuse(strtoul(data->szId, (char **)NULL, 10), data->szReason,
                                        f->Sequence(), dummy, true);

      // We are done now
      winMain->wprintf("%ARefusing file from %s with reason: %Z%s\n",
                       A_BOLD, data->szId, A_BOLD, data->szReason);
      winMain->fProcessInput = &CLicqConsole::InputCommand;

      if(winMain->data)
        delete winMain->data;

      break;
    }

  default:
    break;
  }
}

/*-------------------------------------------------------------------------
 * CLicqConsole::UserCommand_Secure
 *-----------------------------------------------------------------------*/
void CLicqConsole::UserCommand_Secure(const char *szId, unsigned long nPPID, char *szStatus)
{
  ICQUser *u = gUserManager.FetchUser(szId, nPPID, LOCK_R);

  if(!licqDaemon->CryptoEnabled())
  {
    winMain->wprintf("%CYou need to recompile Licq with OpenSSL for this "
                     "feature to work!\n", 16);
    return;
  }

  if(u->SecureChannelSupport() != SECURE_CHANNEL_SUPPORTED)
  {
    winMain->wprintf("%CThe remote end is not using a supported client.  "
                     "This may not work!\n", 16);
  }

  bool bOpen = u->Secure();

  if(szStatus == NULL)
  {
    winMain->wprintf("%ASecure channel is %s to %s\n", A_BOLD,
                     bOpen ? "open" : "closed", u->GetAlias());
  }
  else if(strcasecmp(szStatus, "open") == 0 && bOpen)
  {
    winMain->wprintf("%ASecure channel already open to %s\n", A_BOLD,
                     u->GetAlias());
  }
  else if(strcasecmp(szStatus, "close") == 0 && !bOpen)
  {
    winMain->wprintf("%ASecure channel already closed to %s\n", A_BOLD,
                     u->GetAlias());
  }
  else if(strcasecmp(szStatus, "open") == 0)
  {
    winMain->wprintf("%ARequest secure channel with %s ... ", A_BOLD,
                     u->GetAlias());
    gUserManager.DropUser(u);
    char *p;
    unsigned long uin = strtoul(szId, &p,10);
    if( (p == 0 || (p && !*p)) && nPPID == LICQ_PPID )
      winMain->event = licqDaemon->icqOpenSecureChannel(uin);
  }
  else if(strcasecmp(szStatus, "close") == 0)
  {
    winMain->wprintf("%AClose secure channel with %s ... ", A_BOLD,
                     u->GetAlias());
    gUserManager.DropUser(u);
    char *p;
    unsigned long uin = strtoul(szId, &p,10);
    if( (p == 0 || (p && !*p)) && nPPID == LICQ_PPID )
      winMain->event = licqDaemon->icqCloseSecureChannel(uin);
  }
  else
  {
    winMain->wprintf("%C<user> secure <open | close | (blank)>\n", 16);
  }

  if(u)
    gUserManager.DropUser(u);
}

/*------------------------------------------------------------------------
 * CLicqConsole::UserSelect
 *----------------------------------------------------------------------*/
void CLicqConsole::UserSelect()
{
  // Get the input now
  winMain->fProcessInput = &CLicqConsole::InputUserSelect;
  winMain->state = STATE_LE;

  char sz[20];
  //TODO which owner
  sprintf(sz, "%lu", gUserManager.OwnerUin());
  
  winMain->data = new DataUserSelect(sz, LICQ_PPID); 
  
  ICQOwner *o = gUserManager.FetchOwner(LOCK_R);
  winMain->wprintf("%A%CEnter your password for %s (%lu):%C%Z\n", A_BOLD,
                   24, o->GetAlias(), o->Uin(), 8, A_BOLD);
  gUserManager.DropOwner();
}

/*------------------------------------------------------------------------
 * CLicqConsole::InputUserSelect
 *----------------------------------------------------------------------*/
void CLicqConsole::InputUserSelect(int cIn)
{
  DataUserSelect *data = (DataUserSelect *)winMain->data;
  char *sz;

  switch (winMain->state)
  {
    case STATE_LE:
      if ((sz = Input_Line(data->szPassword, data->nPos, cIn, false)) == NULL)
        return;

      // Go back to the beginning
      data->nPos = 0;

      // Next stage is saving the password possibly
      winMain->wprintf("%C%ASave password? (y/N) %C%Z", 24, A_BOLD,
                       8, A_BOLD);
      winMain->state = STATE_QUERY;
      break;
    
    case STATE_QUERY:
    {
      ICQOwner *o = gUserManager.FetchOwner(LOCK_W);
      o->SetSavePassword(tolower(cIn) == 'y');
      o->SetPassword(data->szPassword);
      gUserManager.DropOwner();

      if (winMain->data)
      {
        delete winMain->data;
        winMain->data = 0;
      }

      winMain->wprintf("%A\nDone. Awaiting commands.%A\n", A_BOLD, A_BOLD);
      winMain->fProcessInput = &CLicqConsole::InputCommand;
      winMain->state = STATE_COMMAND;
      break;
    }

    case STATE_PENDING:
    case STATE_COMMAND:
    case STATE_MLE:
    default:
      break;
  }
}
