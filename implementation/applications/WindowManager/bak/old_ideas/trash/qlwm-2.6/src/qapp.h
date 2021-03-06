/* qapp.h */

#ifndef QAPP_H
#define QAPP_H

#include "toolbar.h"
#include "pager.h"
#include "winlist.h"
#include "menu.h"
#include "procbar.h"
#include "apbar.h"
#include "winfo.h"
#include "rubber.h"
#include "xwindow.h"

extern QList <xwindow> clients;       // client list 

class qapp : public QApplication
{
protected:
	virtual bool x11EventFilter(XEvent *);

public:
	static Toolbar *tb;                   // Toolbar
	static QPixmap *leftwinpix;           // window button pixmap
	static QPixmap *rightwinpix;          // window button pixmap
	static QIntDict <xwindow> cwindows;   // child window lookup table
	static QIntDict <xwindow> pwindows;   // parent window lookup table
	static int next_x;                    // autoplacement counter
	static int next_y;                    // autoplacement counter
	static int adesk;                     // current desktop
	static Atom wm_protocols;             // WM_PROTOCOLS
	static Atom wm_delete;                // WM_DELETE property
	static Atom wm_change_state;          // message from client
	static Atom wm_state;                 // message to client 
	static Atom wm_take_focus;            // WM_TAKE_FOCUS property
	static Atom wm_resource_manager;      // Resource changed
	static Atom wm_colormaps;             // WM_COLORMAP_WINDOWS property
	static bool smode;                    // screen mode
	static QPalette *ipal;                // window inactive palette
	static QPalette *upal;                // window urgent palette
	static bool mrb;                      // move/resize active
	static xwindow *focusclient;          // currently active client
	static xwindow *tmaxclient;
	static bool tdesks[];                 // desks in tiled state
	static winfo *winf;                   // window info 
	static bool menu_open;                // toolbar menu popped up
	static QMap <QString, int> cprops;    // Property flags
	static QMap <QString, int> apclients; // Toolbar clients, position number
	static QDateTime lmtime;              // last defaults modification time
	static bool sighup;                   // SIGHUP received
	static int servershapes;              // server supports nonrectangular windows

	enum clflags { WindowListSkip = 0x01, Sticky = 0x02, SmallFrame = 0x04, NoResize = 0x08, 
	NoTile = 0x10, NoKey = 0x20, NoScreen = 0x40, ClassOnly = 0x80 };
	
	qapp(int &argc, char **argv);
	void setinactive(xwindow *);
	static void stopautofocus(void);
	static void run_client(Window);
	static void install_colormap(Colormap);
	static bool is_curdesk(xwindow *);
	static inline bool is_tileddesk(void) { return(tdesks[adesk]); }
	static void tile_order(xwindow *);
	static void tile_maximize(xwindow *);
	static void toggle_tiled(void);
	static void wm_restart();
	static void read_cprops(void);
	static QString get_cfile(char *);
	static void send_configurenotify(xwindow *);
};
#endif
