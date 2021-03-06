#ifndef LICQRMS_H
#define LICQRMS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "licq_socket.h"

#include <list>

class CICQDaemon;
class TCPSocket;
class ICQUser;
class CUserEvent;
class CICQSignal;
class ICQEvent;
class CLogService_Plugin;

using namespace std;

const unsigned short MAX_LINE_LENGTH = 1024 * 1;
const unsigned short MAX_TEXT_LENGTH = 1024 * 8;

typedef list<class CRMSClient *> ClientList;
typedef list<unsigned long> TagList;

class CLicqRMS
{
public:
  CLicqRMS(bool, unsigned short);
  ~CLicqRMS();
  int Run(CICQDaemon *);
  void Shutdown();
  bool Enabled() { return m_bEnabled; }

protected:
  int m_nPipe;
  bool m_bExit, m_bEnabled;

  unsigned short m_nPort;

  TCPSocket *server;
  ClientList clients;
  CLogService_Plugin *log;

public:
  void ProcessPipe();
  void ProcessSignal(CICQSignal *);
  void ProcessEvent(ICQEvent *);
  void ProcessServer();
  void ProcessLog();
  void AddEventTag(const char *, unsigned long, unsigned long);

friend class CRMSClient;

};


class CRMSClient
{
public:
  CRMSClient(TCPSocket *);
  ~CRMSClient();

  int Activity();

  static CSocketManager sockman;

  int Process_QUIT();
  int Process_TERM();
  int Process_INFO();
  int Process_STATUS();
  int Process_HELP();
  int Process_GROUPS();
  int Process_LIST();
  int Process_MESSAGE();
  int Process_URL();
  int Process_SMS();
  int Process_LOG();
  int Process_VIEW();
  int Process_AR();
  int Process_ADDUSER();
  int Process_REMUSER();
  int Process_SECURE();
  int Process_NOTIFY();

protected:
  TCPSocket sock;
  FILE *fs;
  TagList tags;
  unsigned short m_nState;
  char data_line[MAX_LINE_LENGTH + 1];
  char *data_arg;
  unsigned short data_line_pos;
  unsigned long m_nCheckUin;
  char *m_szCheckId;
  unsigned long m_nLogTypes;
  bool m_bNotify;

  unsigned long m_nUin;
  char *m_szId;
  unsigned long m_nPPID;
  char m_szText[MAX_TEXT_LENGTH + 1];
  char m_szLine[MAX_LINE_LENGTH + 1];
  unsigned short m_nTextPos;
  char *m_szEventId;
  unsigned long m_nEventPPID;

  int StateMachine();
  int ProcessCommand();
  bool ProcessEvent(ICQEvent *);
  bool AddLineToText();
  unsigned long GetProtocol(const char *);
  void ParseUser(const char *);
  int ChangeStatus(unsigned long, unsigned long, const char *);
  void AddEventTag(const char *, unsigned long, unsigned long);
  
  int Process_MESSAGE_text();
  int Process_URL_url();
  int Process_URL_text();
  int Process_SMS_number();
  int Process_SMS_message();
  int Process_AR_text();


  static char buf[128];

friend class CLicqRMS;
};

extern CLicqRMS *licqRMS;


#endif
