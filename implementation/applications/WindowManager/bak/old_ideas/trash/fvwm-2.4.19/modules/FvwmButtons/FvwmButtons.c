/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 */

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ------------------------------- includes -------------------------------- */
#include "config.h"
#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>

#ifdef I18N_MB
#include <X11/Xlocale.h>
#endif
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#ifdef XPM
#include <X11/xpm.h>
#endif

#include "libs/defaults.h"
#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/Grab.h"
#include "fvwm/fvwm.h"
#include "libs/Module.h"
#include "libs/fvwmsignal.h"
#include "libs/Colorset.h"
#include "libs/vpacket.h"
#include "FvwmButtons.h"
#include "misc.h" /* ConstrainSize() */
#include "parse.h" /* ParseConfiguration(), parse_window_geometry() */
#include "icons.h" /* CreateIconWindow(), ConfigureIconWindow() */
#include "draw.h"


#define MW_EVENTS   (ExposureMask |\
		     StructureNotifyMask |\
		     ButtonReleaseMask | ButtonPressMask |\
		     KeyReleaseMask | KeyPressMask | ButtonMotionMask)
/* SW_EVENTS are for swallowed windows... */
#define SW_EVENTS   (PropertyChangeMask | StructureNotifyMask |\
		     ResizeRedirectMask | SubstructureNotifyMask)
/* PA_EVENTS are for swallowed panels... */
#define PA_EVENTS   (StructureNotifyMask)


#ifdef DEBUG_FVWM
#define MySendText(a,b,c) {\
  fprintf(stderr,"%s: Sending text to fvwm: \"%s\"\n",MyName,(b));\
  SendText((a),(b),(c));}
#else
#define MySendText(a,b,c) SendText((a),(b),(c));
#endif

extern int nColorsets;	/* in libs/Colorsets.c */

/* --------------------------- external functions -------------------------- */
extern void DumpButtons(button_info*);
extern void SaveButtons(button_info*);

/* ------------------------------ prototypes ------------------------------- */

void DeadPipe(int nonsense) __attribute__((__noreturn__));
static void DeadPipeCleanup(void);
static RETSIGTYPE TerminateHandler(int sig);
void SetButtonSize(button_info*,int,int);
/* main */
void Loop(void);
void RedrawWindow(button_info*);
void RecursiveLoadData(button_info*,int*,int*);
void CreateUberButtonWindow(button_info*,int,int);
void nocolor(const char *a, const char *b) __attribute__((__noreturn__));
int My_XNextEvent(Display *dpy, XEvent *event);
void process_message(unsigned long type,unsigned long *body);
extern void send_clientmessage (Display *disp, Window w, Atom a,
				Time timestamp);
void CheckForHangon(unsigned long*);
static Window GetRealGeometry(
  Display*,Window,int*,int*,unsigned int*,unsigned int*, unsigned int*,
  unsigned int*);
static void GetPanelGeometry(
  Bool get_big, button_info *b, int lb, int tb, int rb, int bb,
  int *x, int *y, int *w, int *h);
void swallow(unsigned long*);
void AddButtonAction(button_info*,int,char*);
char *GetButtonAction(button_info*,int);
static void change_colorset(int colorset);

void DebugEvents(XEvent*);

static void HandlePanelPress(button_info *b);

/* -------------------------------- globals ---------------------------------*/

Display *Dpy;
Window Root;
Window MyWindow;
char *MyName;
static int screen;

static int x_fd;
static fd_set_size_t fd_width;

char *config_file = NULL;

static Atom _XA_WM_DEL_WIN;

char *imagePath = NULL;

static Pixel hilite_pix, back_pix, shadow_pix, fore_pix;
GC  NormalGC;
/* needed for relief drawing only */
GC  ShadowGC;
static int Width,Height;

static int x= -30000,y= -30000;
int w = -1;
int h = -1;
static int gravity = NorthWestGravity;
int new_desk = 0;
static int ready = 0;
static int xneg = 0;
static int yneg = 0;
int button_width = 0;
int button_height = 0;
int button_lborder = 0;
int button_rborder = 0;
int button_tborder = 0;
int button_bborder = 0;
Bool has_button_geometry = 0;
Bool is_transient = 0;
Bool is_transient_panel = 0;

button_info *CurrentButton = NULL;
Bool is_pointer_in_current_button = False;
int fd[2];

button_info *UberButton=NULL;

int dpw;
int dph;

int save_color_limit;                   /* Color limit, if any */

Bool do_allow_bad_access = False;
Bool was_bad_access = False;

/* ------------------------------ Misc functions ----------------------------*/

#ifdef DEBUG
char *mymalloc(int length)
{
  int i=length;
  char *p=safemalloc(length);
  while(i)
    p[--i]=255;
  return p;
}
#endif

#ifdef I18N_MB
char NoName[] = "Untitled";

/*
 * fake XFetchName() function
 */
static char **XFetchName_islist = NULL; /* XXX */
static Status MyXFetchName(Display *dpy, Window win, char **winname)
{
  XTextProperty text;
  char **list;
  int nitems;

  if (XGetWMName(dpy, win, &text))
  {
    if (text.value)
    {
      text.nitems = strlen(text.value);
      if (text.encoding == XA_STRING)
      {
        /* STRING encoding, use this as it is */
        *winname = (char *)text.value;
      }
      else
      {
        /* not STRING encoding, try to convert */
        if (XmbTextPropertyToTextList(dpy, &text, &list, &nitems) >= Success
            && nitems > 0 && *list)
	{
          XFree(text.value);
          *winname = *list;
          XFetchName_islist = list; /* XXX */
        }
	else
	{
          if (list)
	    XFreeStringList(list);
          XFree(text.value); /* return of XGetWMName() */
          if (XGetWMName(dpy, win, &text)) /* XXX: read again ? */
	  {
	    *winname = (char *)text.value;
	  }
	  else
	  {
	    *winname = NoName;
	  }
        }
      }
      return 1;
    }
  }
  return 0;
}
static int MyXFree(void *data)
{
  if (XFetchName_islist != NULL) {
    XFreeStringList(XFetchName_islist);
    XFetchName_islist = NULL;
    return 0;
  }
  return XFree(data);
}
#define XFetchName(x,y,z) MyXFetchName(x,y,z)
#define XFree(x) MyXFree(x)
#endif

/**
*** Some fancy routines straight out of the manual :-) Used in DeadPipe.
**/
Bool DestroyedWindow(Display *d,XEvent *e,char *a)
{
  if(e->xany.window == (Window)a)
    if((e->type == DestroyNotify && e->xdestroywindow.window == (Window)a)||
       (e->type == UnmapNotify && e->xunmap.window == (Window)a))
      return True;
  return False;
}

static Window SwallowedWindow(button_info *b)
{
  return (b->flags & b_Panel) ? b->PanelWin : b->IconWin;
}

int IsThereADestroyEvent(button_info *b)
{
  XEvent event;
  return XCheckIfEvent(Dpy,&event,DestroyedWindow,
		       (char*)SwallowedWindow(b));
}

/**
*** DeadPipe()
*** Externally callable function to quit! Note that DeadPipeCleanup
*** is an exit-procedure and so will be called automatically
**/
void DeadPipe(int whatever)
{
  exit(0);
}

/**
*** TerminateHandler()
*** Signal handler that will make the event-loop terminate
**/
static RETSIGTYPE
TerminateHandler(int sig)
{
  fvwmSetTerminate(sig);
}

/**
*** DeadPipeCleanup()
*** Remove all the windows from the Button-Bar, and close them as necessary
**/
static void DeadPipeCleanup(void)
{
  button_info *b,*ub=UberButton;
  int button=-1;
  Window swin;

  signal(SIGPIPE, SIG_IGN);/* Xsync may cause SIGPIPE */

  MyXGrabServer(Dpy); /* We don't want interference right now */
  while(NextButton(&ub,&b,&button,0))
  {
    swin = SwallowedWindow(b);
    /* delete swallowed windows */
    if((buttonSwallowCount(b)==3) && swin)
    {
#ifdef DEBUG_HANGON
      fprintf(stderr,"%s: Button 0x%06x window 0x%x (\"%s\") is ",
	      MyName,(ushort)b,(ushort)swin,b->hangon);
#endif
      if(!IsThereADestroyEvent(b))
      { /* Has someone destroyed it? */
	if(!(buttonSwallow(b)&b_NoClose))
	{
	  if(buttonSwallow(b)&b_Kill)
	  {
	    XKillClient(Dpy,swin);
#ifdef DEBUG_HANGON
	    fprintf(stderr,"now killed\n");
#endif
	  }
	  else
	  {
	    send_clientmessage(Dpy,swin,_XA_WM_DEL_WIN,CurrentTime);
#ifdef DEBUG_HANGON
	    fprintf(stderr,"now deleted\n");
#endif
	  }
	}
	else
	{
#ifdef DEBUG_HANGON
	  fprintf(stderr,"now unswallowed\n");
#endif
	  XReparentWindow(Dpy,swin,Root,b->x,b->y);
	  XMoveWindow(Dpy,swin,b->x,b->y);
	  XResizeWindow(Dpy,swin,b->w,b->h);
	  XSetWindowBorderWidth(Dpy,swin,b->bw);
	  if ((b->flags & b_Panel))
	  {
	    XMapWindow(Dpy, swin);
	  }
	}
      }
#ifdef DEBUG_HANGON
      else
	fprintf(stderr,"already handled\n");
#endif
    }
  }
  XSync(Dpy, 0);
  MyXUngrabServer(Dpy); /* We're through */

  /* Hey, we have to free the pictures too! */
  button=-1;
  ub=UberButton;
  while(NextButton(&ub,&b,&button,1))
  {
    /* The picture pointer is NULL if the pixmap came from a colorset. */
    if(b->flags&b_Icon && b->icon != NULL)
      DestroyPicture(Dpy, b->icon);
    if(b->flags&b_IconBack && b->backicon != NULL)
      DestroyPicture(Dpy, b->icon);
    if(b->flags&b_Container && b->c->flags&b_IconBack &&
       !(b->c->flags&b_TransBack) && b->c->backicon != NULL)
      DestroyPicture(Dpy, b->c->backicon);
  }
}

/**
*** SetButtonSize()
*** Propagates global geometry down through the button hierarchy.
**/
void SetButtonSize(button_info *ub, int w, int h)
{
  int i = 0;
  int dx;
  int dy;

  if(!ub || !(ub->flags & b_Container))
  {
    fprintf(stderr,"%s: BUG: Tried to set size of noncontainer\n",MyName);
    exit(2);
  }
  if(ub->c->num_rows==0 || ub->c->num_columns==0)
  {
    fprintf(stderr,"%s: BUG: Set size when rows/cols was unset\n",MyName);
    exit(2);
  }

  if (ub->parent)
  {
    i = buttonNum(ub);
    ub->c->xpos = buttonXPos(ub,i);
    ub->c->ypos = buttonYPos(ub,i);
  }
  dx = buttonXPad(ub) + buttonFrame(ub);
  dy = buttonYPad(ub) + buttonFrame(ub);
  ub->c->xpos += dx;
  ub->c->ypos += dy;
  w -= 2 * dx;
  h -= 2 * dy;
  ub->c->width = w;
  ub->c->height = h;

  i = 0;
  while (i < ub->c->num_buttons)
  {
    if (ub->c->buttons[i] && ub->c->buttons[i]->flags & b_Container)
    {
      SetButtonSize(ub->c->buttons[i], buttonWidth(ub->c->buttons[i]),
		    buttonHeight(ub->c->buttons[i]));
    }
    i++;
  }
}


/**
*** AddButtonAction()
**/
void AddButtonAction(button_info *b,int n,char *action)
{
  int l;
  char *s;
  char *t;

  if(!b || n < 0 || n > NUMBER_OF_MOUSE_BUTTONS || !action)
  {
    fprintf(stderr, "%s: BUG: AddButtonAction failed\n", MyName);
    exit(2);
  }
  if(b->flags&b_Action)
  {
    if(b->action[n])
      free(b->action[n]);
  }
  else
  {
    int i;
    b->action=(char**)mymalloc((NUMBER_OF_MOUSE_BUTTONS + 1) * sizeof(char*));
    for (i = 0; i <= NUMBER_OF_MOUSE_BUTTONS; b->action[i++] = NULL)
      ;
    b->flags|=b_Action;
  }

  while (*action && isspace((unsigned char)*action))
    action++;
  l = strlen(action);
  if (l > 1)
  {
    switch (action[0])
    {
    case '\"':
    case '\'':
    case '`':
      s = SkipQuote(action, NULL, "", "");
      /* Strip outer quotes */
      if (*s == 0)
      {
	action++;
	l -= 2;
      }
      break;
    default:
      break;
    }
  }
  t = (char *)mymalloc(l + 1);
  memmove(t, action, l);
  t[l] = 0;
  b->action[n] = t;
}


/**
*** GetButtonAction()
**/
char *GetButtonAction(button_info *b,int n)
{
  if(!b || !(b->flags&b_Action) || !(b->action) || n < 0 ||
     n > NUMBER_OF_MOUSE_BUTTONS)
    return NULL;
  if (!b->action[n])
    return NULL;

  return expand_action(b->action[n], b);
}

/**
*** SetTransparentBackground()
*** use the Shape extension to create a transparent background.
***   Patrice Fortier
**/
void SetTransparentBackground(button_info *ub,int w,int h)
{
  if (FShapesSupported)
  {
    Pixmap pmap_mask;
    button_info *b;
    Window root_return;
    int x_return, y_return;
    unsigned int width_return, height_return;
    unsigned int border_width_return;
    unsigned int depth_return;
    int number, i;
    XFontStruct *font;
    Region shape_r;
    static GC trans_gc = NULL;

    pmap_mask = XCreatePixmap(Dpy,MyWindow,w,h,1);
    if (pmap_mask == None)
      return;
    shape_r = XCreateRegion();
    if (trans_gc == NULL)
    {
      XGCValues gcv;

      /* create a GC for doing transparency */
      trans_gc = fvwmlib_XCreateGC(Dpy,pmap_mask,(unsigned long)0,&gcv);
      if (trans_gc == NULL)
	return;
    }
    XSetClipMask(Dpy, trans_gc, None);
    XSetForeground(Dpy, trans_gc, 0);
    XFillRectangle(Dpy, pmap_mask, trans_gc, 0, 0, w, h);
    XSetForeground(Dpy, trans_gc, 1);

    /*
     * if button has an icon, draw a rect with the same size as the icon
     * (or the mask of the icon),
     * else draw a rect with the same size as the button.
     */

    i = -1;
    while (NextButton(&ub,&b,&i,0))
    {
      number=buttonNum(b);
      if (b->flags&b_Icon &&
	  XGetGeometry(Dpy,b->IconWin,&root_return,&x_return,&y_return,
		       &width_return,&height_return,
		       &border_width_return,&depth_return))
      {
	if (b->icon->mask == None)
	{
	  XFillRectangle(Dpy,pmap_mask,trans_gc,x_return, y_return,
			 b->icon->width,b->icon->height);
	}
	else
	{
	  XCopyArea(Dpy,b->icon->mask,pmap_mask,trans_gc,0,0,
		    b->icon->width,b->icon->height,x_return,y_return);
	}
      }
      else if (buttonSwallowCount(b) == 3 && (b->flags & b_Swallow))
      {
	Window swin = SwallowedWindow(b);
	XRectangle *r;
	int count;
	int ordering;
	XRectangle s;
	Bool got_geometry = False;

	if (XGetGeometry(
	      Dpy, swin, &root_return, &x_return, &y_return, &width_return,
	      &height_return, &border_width_return, &depth_return))
	{
	  got_geometry = True;
	  s.x = x_return;
	  s.y = y_return;
	  s.width = width_return;
	  s.height = height_return;
	}
	else
	{
	  s.x = buttonXPos(b, number);
	  s.y = buttonYPos(b, number);
	  s.width = buttonWidth(b);
	  s.height = buttonHeight(b);
	}

	/* get set of shape rectangles - this is all very inefficient, but I do
	 * not see another way to do it properly */
	if (got_geometry &&
	    (r = FShapeGetRectangles(
	      Dpy, swin, FShapeBounding, &count, &ordering)))
	{
	  int i;

	  /* merge the rectangles with the shape region */
	  for (i = 0; i < count ; i++)
	  {
	    r[i].x += s.x;
	    r[i].y += s.y;
	    XUnionRectWithRegion(&(r[i]), shape_r, shape_r);
	  }
	  XFree(r);
	}
	else
	{
	  XUnionRectWithRegion(&s, shape_r, shape_r);
	  /* suppress compiler warning w/o shape extension */
	  ordering = 0;
	}
      }
      else
      {
	XFillRectangle(Dpy,pmap_mask,trans_gc,buttonXPos(b,number),
		       buttonYPos(b,number),
		       buttonWidth(b),buttonHeight(b));
      }

      /* handle button's title */
      font=buttonFont(b);
      if(b->flags&b_Title && font)
      {
	XSetFont(Dpy,trans_gc,font->fid);
	DrawTitle(b,pmap_mask,trans_gc);
      }
    }
    XSetRegion(Dpy, trans_gc, shape_r);
    XFillRectangle(Dpy, pmap_mask, trans_gc, 0, 0, w, h);
    FShapeCombineMask(
      Dpy, MyWindow, FShapeBounding, 0, 0, pmap_mask, FShapeSet);
    XFreePixmap(Dpy, pmap_mask);
    XDestroyRegion(shape_r);
    XFlush(Dpy);
  }

  return;
}

/**
*** myErrorHandler()
*** Shows X errors made by FvwmButtons.
**/
XErrorHandler oldErrorHandler=NULL;
int myErrorHandler(Display *dpy, XErrorEvent *event)
{
  /* some errors are acceptable, mostly they're caused by
   * trying to update a lost  window */
  if((event->error_code == BadAccess) && do_allow_bad_access)
  {
    was_bad_access = 1;
    return 0;
  }
  if((event->error_code == BadWindow) || (event->error_code == BadDrawable)
     || (event->error_code == BadMatch) || (event->request_code==X_GrabButton)
     || (event->request_code == X_GetGeometry)
     || (event->error_code == BadPixmap))
    return 0;

  PrintXErrorAndCoredump(dpy, event, MyName);

  /* return (*oldErrorHandler)(dpy,event); */
  return 0;
}

/* ---------------------------------- main ----------------------------------*/

/**
*** main()
**/
int main(int argc, char **argv)
{
  int i;
  Window root;
  int x,y,maxx,maxy,border_width;
  unsigned int depth;
  button_info *b,*ub;
  int geom_option_argc = 0;

#ifdef I18N_MB
  setlocale(LC_CTYPE, "");
#endif
  MyName = GetFileNameFromPath(argv[0]);

#ifdef HAVE_SIGACTION
  {
    struct sigaction  sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGHUP);
    sigaddset(&sigact.sa_mask, SIGQUIT);
    sigaddset(&sigact.sa_mask, SIGTERM);
#ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
# else
    sigact.sa_flags = 0;
#endif
    sigact.sa_handler = TerminateHandler;

    sigaction(SIGPIPE, &sigact, NULL);
    sigaction(SIGINT,  &sigact, NULL);
    sigaction(SIGHUP,  &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
  }
#else
  /* We don't have sigaction(), so fall back to less robust methods.  */
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGPIPE) |
                     sigmask(SIGINT)  |
                     sigmask(SIGHUP)  |
                     sigmask(SIGQUIT) |
                     sigmask(SIGTERM) );
#endif

  signal(SIGPIPE, TerminateHandler);
  signal(SIGINT,  TerminateHandler);
  signal(SIGHUP,  TerminateHandler);
  signal(SIGQUIT, TerminateHandler);
  signal(SIGTERM, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGINT, 1);
  siginterrupt(SIGHUP, 1);
  siginterrupt(SIGQUIT, 1);
  siginterrupt(SIGTERM, 1);
#endif
#endif

  if(argc<6 || argc>10)
  {
    fprintf(stderr, "%s version %s should only be executed by fvwm!\n",
	    MyName, VERSION);
    exit(1);
  }

  for (i = 6; i < argc && i < 11; i++)
  {
    static Bool has_name = 0;
    static Bool has_file = 0;
    static Bool has_geometry = 0;

    if (!has_geometry && strcmp(argv[i], "-g") == 0)
    {
      has_geometry = 1;
      if (++i < argc)
      {
	/* can't do that now because the UberButton has not been set up */
	geom_option_argc = i;
      }
    }
    else if (!is_transient && !is_transient_panel &&
	     (strcmp(argv[i], "-transient") == 0 ||
	      strcmp(argv[i], "transient") == 0))
    {
      is_transient = 1;
    }
    else if (!is_transient && !is_transient_panel &&
	     (strcmp(argv[i], "-transientpanel") == 0 ||
	      strcmp(argv[i], "transientpanel") == 0))
    {
      is_transient_panel = 1;
    }
    else if (!has_name) /* There is a naming argument here! */
    {
      free(MyName);
      MyName=safestrdup(argv[i]);
      has_name = 1;
    }
    else if (!has_file) /* There is a config file here! */
    {
      config_file=safestrdup(argv[i]);
      has_file = 1;
    }
  }

  fd[0]=atoi(argv[1]);
  fd[1]=atoi(argv[2]);
  if (!(Dpy = XOpenDisplay(NULL)))
  {
    fprintf(stderr,"%s: Can't open display %s", MyName,
	    XDisplayName(NULL));
    exit (1);
  }
  InitPictureCMap(Dpy);
  FScreenInit(Dpy);
  /* Initialise default colorset */
  AllocColorset(0);
  FShapeInit(Dpy);

  x_fd=XConnectionNumber(Dpy);
  fd_width=GetFdWidth();

  screen=DefaultScreen(Dpy);
  Root=RootWindow(Dpy, screen);
  if(Root==None)
  {
    fprintf(stderr,"%s: Screen %d is not valid\n",MyName,screen);
    exit(1);
  }

  oldErrorHandler=XSetErrorHandler(myErrorHandler);

  UberButton=(button_info*)mymalloc(sizeof(button_info));
  memset(UberButton, 0, sizeof(button_info));
  UberButton->BWidth=1;
  UberButton->BHeight=1;
  MakeContainer(UberButton);

  dpw = DisplayWidth(Dpy,screen);
  dph = DisplayHeight(Dpy,screen);

#ifdef DEBUG_INIT
  fprintf(stderr,"%s: Parsing...",MyName);
#endif

  UberButton->title   = MyName;
  UberButton->swallow = 1; /* the panel is shown */

  /* parse module options */
  ParseConfiguration(UberButton);
  /* we can't set the size if it was specified in pixels per button here;
   * delay until after call to RecursiveLoadData. */
  /* parse the geometry string */
  if (geom_option_argc != 0)
  {
    parse_window_geometry(argv[geom_option_argc], 0);
    has_button_geometry = 0;
  }

  /* Don't quit if only a subpanel is empty */
  if(UberButton->c->num_buttons==0)
  {
    fprintf(stderr,"%s: No buttons defined. Quitting\n", MyName);
    exit(0);
  }

#ifdef DEBUG_INIT
  fprintf(stderr,"OK\n%s: Shuffling...",MyName);
#endif

  ShuffleButtons(UberButton);
  NumberButtons(UberButton);

#ifdef DEBUG_INIT
  fprintf(stderr,"OK\n%s: Loading data...\n",MyName);
#endif

  /* Load fonts and icons, calculate max buttonsize */
  maxx=0;
  maxy=0;
  RecursiveLoadData(UberButton,&maxx,&maxy);

  /* now we can size the main window if pixels per button were specified */
  if (has_button_geometry && button_width > 0 && button_height > 0)
  {
    w = button_width * UberButton->c->num_columns;
    h = button_height * UberButton->c->num_rows;
  }

#ifdef DEBUG_INIT
  fprintf(stderr,"%s: Creating main window...",MyName);
#endif

  CreateUberButtonWindow(UberButton,maxx,maxy);

#ifdef DEBUG_INIT
  fprintf(stderr,"OK\n%s: Creating icon windows...",MyName);
#endif

  i=-1;
  ub=UberButton;
  while(NextButton(&ub,&b,&i,0))
  {
    if(b->flags&b_Icon)
    {
#ifdef DEBUG_INIT
      fprintf(stderr,"0x%06x...",(ushort)b);
#endif
      CreateIconWindow(b);
    }
  }

#ifdef DEBUG_INIT
  fprintf(stderr,"OK\n%s: Configuring windows...",MyName);
#endif

  if (!XGetGeometry(
    Dpy, MyWindow, &root, &x, &y, (unsigned int *)&Width,
    (unsigned int *)&Height, (unsigned int *)&border_width, &depth))
  {
    fprintf(stderr, "%s: Failed to get window geometry\n",MyName);
    exit(0);
  }
  SetButtonSize(UberButton,Width,Height);
  i=-1;
  ub=UberButton;
  while(NextButton(&ub,&b,&i,0))
    ConfigureIconWindow(b);

  if (FShapesSupported)
  {
    if (UberButton->c->flags&b_TransBack)
      SetTransparentBackground(UberButton,Width,Height);
  }

  i=-1;
  ub=UberButton;
  while(NextButton(&ub,&b,&i,0))
    MakeButton(b);

#ifdef DEBUG_INIT
  fprintf(stderr,"OK\n%s: Mapping windows...",MyName);
#endif

  XMapSubwindows(Dpy,MyWindow);
  XMapWindow(Dpy,MyWindow);

  SetMessageMask(fd, M_NEW_DESK | M_END_WINDOWLIST | M_MAP | M_WINDOW_NAME
		 | M_RES_CLASS | M_CONFIG_INFO | M_END_CONFIG_INFO | M_RES_NAME
		 | M_SENDCONFIG | M_ADD_WINDOW | M_CONFIGURE_WINDOW);

  /* request a window list, since this triggers a response which
   * will tell us the current desktop and paging status, needed to
   * indent buttons correctly */
  MySendText(fd,"Send_WindowList",0);

#ifdef DEBUG_INIT
  fprintf(stderr,"OK\n%s: Startup complete\n",MyName);
#endif

  /*
  ** Now that we have finished initialising everything,
  ** it is safe(r) to install the clean-up handlers ...
  */
  atexit(DeadPipeCleanup);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);

  Loop();

  return 0;
}

/* -------------------------------- Main Loop -------------------------------*/

/**
*** Loop
**/
void Loop(void)
{
  XEvent Event;
  KeySym keysym;
  char buffer[10],*tmp,*act;
  int i,i2,button;
  button_info *ub,*b;
#ifndef OLD_EXPOSE
  int ex=10000,ey=10000,ex2=0,ey2=0;
#endif

  while( !isTerminated )
  {
    if(My_XNextEvent(Dpy,&Event))
    {
      if (FShapesSupported)
      {
	if (Event.type == FShapeEventBase + FShapeNotify)
	{
	  FShapeEvent *sev = (FShapeEvent *) &Event;

	  if (sev->kind != FShapeBounding)
	    return;
	  if (UberButton->c->flags & b_TransBack)
	  {
	    SetTransparentBackground(UberButton, Width, Height);
	  }
	  continue;
	}
      }
      switch(Event.type)
      {
      case Expose:
#ifdef OLD_EXPOSE
	if(Event.xexpose.count == 0)
	{
	  button=-1;ub=UberButton;
	  while(NextButton(&ub,&b,&button,1))
	  {
	    if(!ready && !(b->flags&b_Container))
	      MakeButton(b);
	    RedrawButton(b,1);
	  }
	  if(!ready)
	    ready++;
	}
#else
	ex=min(ex,Event.xexpose.x);
	ey=min(ey,Event.xexpose.y);
	ex2=max(ex2,Event.xexpose.x+Event.xexpose.width);
	ey2=max(ey2,Event.xexpose.y+Event.xexpose.height);

	if(Event.xexpose.count==0)
	{
	  button=-1;ub=UberButton;
	  while(NextButton(&ub,&b,&button,1))
	  {
	    if(b->flags&b_Container)
	    {
	      x=buttonXPos(b,buttonNum(b));
	      y=buttonYPos(b,buttonNum(b));
	    }
	    else
	    {
	      x=buttonXPos(b,button);
	      y=buttonYPos(b,button);
	    }
	    if(!(ex > x + buttonWidth(b) || ex2 < x ||
		 ey > y + buttonHeight(b) || ey2 < y))
	    {
	      if(ready<1 && !(b->flags&b_Container))
		MakeButton(b);
	      RedrawButton(b,1);
	    }
	  }
	  if(ready<1)
	    ready++;

	  ex=ey=10000;ex2=ey2=0;
	}
#endif
	break;

      case ConfigureNotify:
      {
	unsigned int depth,tw,th,border_width;
	Window root;

	if (!XGetGeometry(Dpy, MyWindow, &root, &x, &y, &tw, &th, &border_width,
			  &depth))
	{
	  break;
	}
	if(tw!=Width || th!=Height)
	{
	  Width=tw;
	  Height=th;
	  SetButtonSize(UberButton,Width,Height);
	  button=-1;
	  ub=UberButton;
	  for (i = 0; i < nColorsets; i++)
	    change_colorset(i);
	  while(NextButton(&ub,&b,&button,0))
	    MakeButton(b);
	  if (FShapesSupported && UberButton->c->flags & b_TransBack)
	  {
	    SetTransparentBackground(UberButton, Width, Height);
	  }
	  RedrawWindow(NULL);
	}
      }
      break;

      case MotionNotify:
	{
	  Bool f = is_pointer_in_current_button;

	  is_pointer_in_current_button =
	    (CurrentButton && CurrentButton ==
	     select_button(UberButton, Event.xmotion.x, Event.xmotion.y));
	  if (CurrentButton && is_pointer_in_current_button != f)
	  {
	    RedrawButton(b, 0);
	  }
	}
	break;

      case KeyPress:
	XLookupString(&Event.xkey,buffer,10,&keysym,0);
	if(keysym!=XK_Return && keysym!=XK_KP_Enter && keysym!=XK_Linefeed)
	  break;	                /* fall through to ButtonPress */

      case ButtonPress:
	if (CurrentButton)
	{
	  b = CurrentButton;
	  CurrentButton = 0;
	  RedrawButton(b, 0);
	  break;
	}
	if (Event.xbutton.state & DEFAULT_ALL_BUTTONS_MASK)
	{
	  break;
	}
	CurrentButton = b =
	  select_button(UberButton,Event.xbutton.x,Event.xbutton.y);
	is_pointer_in_current_button = True;

	act = NULL;
	if (!(b->flags & b_Panel) &&
	    (!b || !(b->flags&b_Action) ||
	     ((act=GetButtonAction(b,Event.xbutton.button)) == NULL &&
	      (act=GetButtonAction(b,0)) == NULL)))
	{
	  CurrentButton=NULL;
	  break;
	}
	RedrawButton(b,0);
	if (!act)
	{
	  break;
	}
	if (act && strncasecmp(act,"popup",5) != 0)
	{
	  if (act)
	  {
	    free(act);
	    act = NULL;
	  }
	  break;
	}
	else /* i.e. action is Popup */
	{
	  XUngrabPointer(Dpy,CurrentTime); /* And fall through */
	}
	free(act);
	act = NULL;
	/* fall through */

      case KeyRelease:
      case ButtonRelease:
	if (CurrentButton == NULL || !is_pointer_in_current_button)
	{
	  CurrentButton = NULL;
	  break;
	}
	b = select_button(UberButton,Event.xbutton.x,Event.xbutton.y);
	act = GetButtonAction(b,Event.xbutton.button);
	if (b && !act && (b->flags & b_Panel))
	{
	  HandlePanelPress(b);
	  if (b->newflags.panel_mapped == 0)
	  {
	    if (is_transient)
	    {
	      /* terminate if transient and panel has been unmapped */
	      exit(0);
	    }
	    else if (is_transient_panel)
	    {
	      XWithdrawWindow(Dpy, MyWindow, screen);
	    }
	  }
	} /* panel */
	else
	{
	  if (!act)
	    act=GetButtonAction(b,0);
	  if (b && b == CurrentButton && act)
	  {
	    if (strncasecmp(act,"Exec",4) == 0 &&
		isspace(act[4]))
	    {

	      /* Look for Exec "identifier", in which case the button
		 stays down until window "identifier" materializes */
	      i = 4;
	      while(isspace((unsigned char)act[i]))
		i++;
	      if(act[i] == '"')
	      {
		i2=i+1;
		while(act[i2]!=0 && act[i2]!='"')
		  i2++;

		if(i2-i>1)
		{
		  b->flags|=b_Hangon;
		  b->hangon = mymalloc(i2-i);
		  strncpy(b->hangon,&act[i+1],i2-i-1);
		  b->hangon[i2-i-1] = 0;
		}
		i2++;
	      }
	      else
		i2=i;

	      tmp=mymalloc(strlen(act)+1);
	      strcpy(tmp,"Exec ");
	      while(act[i2]!=0 && isspace((unsigned char)act[i2]))
		i2++;
	      strcat(tmp,&act[i2]);
	      if (is_transient)
	      {
		/* delete the window before continuing */
		XDestroyWindow(Dpy, MyWindow);
		XSync(Dpy, 0);
	      }
	      MySendText(fd,tmp,0);
	      if (is_transient)
	      {
		/* and exit */
		exit(0);
	      }
	      if (is_transient_panel)
	      {
		XWithdrawWindow(Dpy, MyWindow, screen);
	      }
	      free(tmp);
	    } /* exec */
	    else if(strncasecmp(act,"DumpButtons",11)==0)
	      DumpButtons(UberButton);
	    else if(strncasecmp(act,"SaveButtons",11)==0)
	      SaveButtons(UberButton);
	    else
	    {
	      if (is_transient)
	      {
		/* delete the window before continuing */
		XDestroyWindow(Dpy, MyWindow);
		XSync(Dpy, 0);
	      }
	      MySendText(fd,act,0);
	      if (is_transient)
	      {
		/* and exit */
		exit(0);
	      }
	      if (is_transient_panel)
	      {
		XWithdrawWindow(Dpy, MyWindow, screen);
	      }
	    }
	  } /* act */
	} /* !panel */

	if (act != NULL)
	{
	  free(act);
	  act = NULL;
	}
	b = CurrentButton;
	CurrentButton=NULL;
	if (b)
	  RedrawButton(b, 0);
	break;

      case ClientMessage:
	if(Event.xclient.format == 32 &&
	   Event.xclient.data.l[0]==_XA_WM_DEL_WIN)
	{
	  DeadPipe(1);
	}
	break;

      case PropertyNotify:
	if(Event.xany.window==None)
	  break;
	ub=UberButton;button=-1;
	while(NextButton(&ub,&b,&button,0))
	{
	  Window swin = SwallowedWindow(b);

	  if((buttonSwallowCount(b)==3) && Event.xany.window == swin)
	  {
	    if(Event.xproperty.atom==XA_WM_NAME &&
	       buttonSwallow(b)&b_UseTitle)
	    {
	      if(b->flags&b_Title)
		free(b->title);
	      b->flags|=b_Title;
	      if (XFetchName(Dpy,swin,&tmp))
	      {
		CopyString(&b->title,tmp);
		XFree(tmp);
	      }
	      else
	      {
		CopyString(&b->title, "");
	      }
	      MakeButton(b);
	    }
	    else if((Event.xproperty.atom==XA_WM_NORMAL_HINTS) &&
		    (!(buttonSwallow(b)&b_NoHints)))
	    {
	      long supp;
	      if(!XGetWMNormalHints(Dpy,swin,b->hints,&supp))
		b->hints->flags = 0;
	      MakeButton(b);
	      if (FShapesSupported)
	      {
		if (UberButton->c->flags & b_TransBack)
		{
		  SetTransparentBackground(UberButton, Width, Height);
		}
	      }
	    }
	    RedrawButton(b,1);
	  }
	}
	break;

      case MapNotify:
      case UnmapNotify:
	ub=UberButton;
	button=-1;
	while(NextButton(&ub,&b,&button,0))
	{
	  if (b->flags & b_Panel && Event.xany.window == b->PanelWin)
	  {
	    /* A panel has been unmapped, update the button */
	    b->newflags.panel_mapped = (Event.type == MapNotify);
	    RedrawButton(b, 1);
	    break;
	  }
	}
	break;

      case DestroyNotify:
	ub=UberButton;button=-1;
	while(NextButton(&ub,&b,&button,0))
	{
	  Window swin = SwallowedWindow(b);

	  if((buttonSwallowCount(b)==3) && Event.xany.window == swin)
	  {
#ifdef DEBUG_HANGON
	    fprintf(stderr,
		    "%s: Button 0x%06x lost its window 0x%x (\"%s\")",
		    MyName,(ushort)b,(ushort)swin,b->hangon);
#endif
	    b->swallow&=~b_Count;
	    if (b->flags & b_Panel)
	      b->PanelWin=None;
	    else
	      b->IconWin=None;
	    if(buttonSwallow(b)&b_Respawn && b->hangon && b->spawn)
	    {
	      char *p;

#ifdef DEBUG_HANGON
	      fprintf(stderr,", respawning\n");
#endif
	      if (b->newflags.is_panel && is_transient)
	      {
		/* terminate if transient and a panel has been killed */
		exit(0);
	      }
	      b->swallow|=1;
	      if (!b->newflags.is_panel)
		b->flags |= (b_Swallow | b_Hangon);
	      else
	      {
		b->flags |= (b_Panel | b_Hangon);
		b->newflags.panel_mapped = 0;
	      }
	      p = expand_action(b->spawn, NULL);
	      if (p)
	      {
		MySendText(fd, p, 0);
		free(p);
	      }
	      RedrawButton(b,1);
	      if (is_transient_panel)
	      {
		XWithdrawWindow(Dpy, MyWindow, screen);
	      }
	    }
	    else
	    {
	      b->flags &= ~(b_Swallow | b_Panel);
	      RedrawButton(b,2);
#ifdef DEBUG_HANGON
	      fprintf(stderr,"\n");
#endif
	    }
	    break;
	  }
	}
	break;

      default:
#ifdef DEBUG_EVENTS
	fprintf(stderr,"%s: Event fell through unhandled\n",MyName);
#endif
	break;
      }
    }
  }
}

/**
*** RedrawWindow()
*** Draws the window by traversing the button tree, draws all if NULL is given,
*** otherwise only the given button.
**/
void RedrawWindow(button_info *b)
{
  int button;
  XEvent dummy;
  button_info *ub;
  static Bool initial_redraw = True;
  Bool clear_buttons;

  if(ready<1)
    return;

  /* Flush expose events */
  while (XCheckTypedWindowEvent (Dpy, MyWindow, Expose, &dummy))
    ;

  if(b)
  {
    RedrawButton(b,0);
    return;
  }

  /* Clean out the entire window first */
  if (initial_redraw == False)
  {
    XClearWindow(Dpy,MyWindow);
    clear_buttons = False;
  }
  else
  {
    initial_redraw = False;
    clear_buttons = True;
  }

  button=-1;
  ub=UberButton;
  while(NextButton(&ub,&b,&button,1))
    RedrawButton(b,1);
}

/**
*** LoadIconFile()
**/
int LoadIconFile(char *s,Picture **p)
{
  *p=CachePicture(Dpy,Root,imagePath,s, save_color_limit);
  if(*p)
    return 1;
  return 0;
}

/**
*** RecursiveLoadData()
*** Loads colors, fonts and icons, and calculates buttonsizes
**/
void RecursiveLoadData(button_info *b,int *maxx,int *maxy)
{
  int i,j,x=0,y=0;
  XFontStruct *font;
#ifdef I18N_MB
  char **ml;
  int mc;
  char *ds;
  XFontStruct **fs_list;
  XFontSet fontset = None;
#endif

  if (!b)
    return;

#ifdef DEBUG_LOADDATA
  fprintf(stderr,"%s: Loading: Button 0x%06x: colors",MyName,(ushort)b);
#endif

  /* initialise button colours and background */
  if(b->flags & b_Colorset)
  {
    int cset = b->colorset;

    /* override normal icon options */
    if (b->flags & b_IconBack && !(b->flags & b_TransBack))
    {
      free(b->back);
    }
    b->flags &= ~(b_IconBack | b_TransBack);

    /* fetch the colours from the colorset */
    b->fc = Colorset[cset].fg;
    b->bc = Colorset[cset].bg;
    b->hc = Colorset[cset].hilite;
    b->sc = Colorset[cset].shadow;
    if (Colorset[cset].pixmap != None)
    {
      /* we have a pixmap */
      b->backicon = NULL;
      b->flags |= b_IconBack;
    }
  }
  else /* no colorset */
  {
    if(b->flags&b_Fore)
      b->fc=GetColor(b->fore);
    if(b->flags&b_Back)
    {
      b->bc = GetColor(b->back);
      b->hc = GetHilite(b->bc);
      b->sc = GetShadow(b->bc);
      if(b->flags&b_IconBack)
      {
	if(!LoadIconFile(b->back,&b->backicon))
	  b->flags&=~b_Back;
      }
    } /* b_Back */
    else
    {
    }
  } /* !b_Colorset */

  /* initialise container colours and background */
  if (b->flags & b_Container)
  {
    if(b->c->flags & b_Colorset)
    {
      int cset = b->c->colorset;

      /* override normal icon options */
      if (b->c->flags & b_IconBack && !(b->c->flags & b_TransBack))
      {
	free(b->c->back_file);
      }
      b->c->flags &= ~(b_IconBack | b_TransBack);

      /* fetch the colours from the colorset */
      b->c->fc = Colorset[cset].fg;
      b->c->bc = Colorset[cset].bg;
      b->c->hc = Colorset[cset].hilite;
      b->c->sc = Colorset[cset].shadow;
      if (Colorset[cset].pixmap != None)
      {
	/* we have a pixmap */
	b->c->backicon = NULL;
	b->c->flags |= b_IconBack;
      }
    }
    else /* no colorset */
    {
#ifdef DEBUG_LOADDATA
      fprintf(stderr,", colors2");
#endif
      if(b->c->flags&b_Fore)
	b->c->fc=GetColor(b->c->fore);
      if(b->c->flags&b_Back)
      {
	b->c->bc = GetColor(b->c->back);
	b->c->hc = GetHilite(b->c->bc);
	b->c->sc = GetShadow(b->c->bc);
	if(b->c->flags&b_IconBack && !(b->c->flags&b_TransBack))
	{
	  if(!LoadIconFile(b->c->back_file,&b->c->backicon))
	    b->c->flags&=~b_IconBack;
	}
      }
    } /* !b_Colorset */
  } /* b_Container */



  /* Load the font */
  if(b->flags&b_Font)
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr,", font \"%s\"",b->font_string);
#endif

    if(strncasecmp(b->font_string,"none",4)==0)
      b->font=NULL;
#ifdef I18N_MB
    else if(!(b->fontset=XCreateFontSet(Dpy,b->font_string,&ml,&mc,&ds)))
    {
      b->font = NULL;
      b->flags&=~b_Font;
      fprintf(stderr,"%s: Couldn't load fontset %s\n",MyName,
	      b->font_string);
    }
    else
    {
      /* fontset found */
      XFontsOfFontSet(fontset, &fs_list, &ml);
      b->font = fs_list[0];
    }
#else
    else if(!(b->font=XLoadQueryFont(Dpy,b->font_string)))
    {
      b->flags&=~b_Font;
      fprintf(stderr,"%s: Couldn't load font %s\n",MyName,
	      b->font_string);
    }
#endif
  }

  if(b->flags&b_Container && b->c->flags&b_Font)
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr,", font2 \"%s\"",b->c->font_string);
#endif
    if(strncasecmp(b->c->font_string,"none",4)==0)
      b->c->font=NULL;
#ifdef I18N_MB
    else if(!(b->c->fontset=XCreateFontSet(Dpy,b->c->font_string,&ml,&mc,&ds)))
    {
      fprintf(stderr,"%s: Couldn't load fontset %s\n",MyName,
	      b->c->font_string);
      if(b==UberButton)
      {
#ifdef STRICTLY_FIXED
	if(!(b->c->fontset=XCreateFontSet(Dpy,"fixed",&ml,&mc,&ds)))
	{
#else
	if(!(b->c->fontset=XCreateFontSet(
	  Dpy,"-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",&ml,&mc,&ds)))
	{
#endif
	  fprintf(stderr,"%s: Couldn't load fontset fixed\n",MyName);
	  b->c->font = NULL;
	}
      }
      else
      {
	b->c->font = NULL;
	b->c->flags&=~b_Font;
      }
    }
    else
    {
      /* fontset found */
      XFontsOfFontSet(b->c->fontset, &fs_list, &ml);
      b->c->font = fs_list[0];
    }
#else
    else if(!(b->c->font=XLoadQueryFont(Dpy,b->c->font_string)))
    {
      fprintf(stderr,"%s: Couldn't load font %s\n",MyName,
	      b->c->font_string);
      if(b==UberButton)
      {
	if(!(b->c->font=XLoadQueryFont(Dpy,"fixed")))
	  fprintf(stderr,"%s: Couldn't load font fixed\n",MyName);
      }
      else
	b->c->flags&=~b_Font;
    }
#endif
  }

  /* Calculate subbutton sizes */
  if(b->flags&b_Container && b->c->num_buttons)
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr,", entering container\n");
#endif
    for(i=0;i<b->c->num_buttons;i++)
      if(b->c->buttons[i])
	RecursiveLoadData(b->c->buttons[i],&x,&y);

    if(b->c->flags&b_Size)
    {
      x=b->c->minx;
      y=b->c->miny;
    }
#ifdef DEBUG_LOADDATA
    fprintf(stderr,"%s: Loading: Back to container 0x%06x",MyName,(ushort)b);
#endif

    x*=b->c->num_columns;
    y*=b->c->num_rows;
    b->c->width=x;
    b->c->height=y;
  }


  i=0;
  j=0;

  /* Load the icon */
  if(b->flags&b_Icon && LoadIconFile(b->icon_file,&b->icon))
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr,", icon \"%s\"",b->icon_file);
#endif
    i=b->icon->width;
    j=b->icon->height;
  }
  else
    b->flags&=~b_Icon;

#ifdef I18N_MB
  if(b->flags&b_Title && (fontset = buttonFontSet(b)) && (font=buttonFont(b)))
#else
  if(b->flags&b_Title && (font=buttonFont(b)))
#endif
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr,", title \"%s\"",b->title);
#endif
    if(buttonJustify(b)&b_Horizontal)
    {
      i+=buttonXPad(b)+XTextWidth(font,b->title,strlen(b->title));
      j=max(j,font->ascent+font->descent);
    }
    else
    {
      i=max(i,XTextWidth(font,b->title,strlen(b->title)));
      j+=font->ascent+font->descent;
    }
  }

  x+=i;
  y+=j;

  if(b->flags&b_Size)
  {
    x=b->minx;
    y=b->miny;
  }

  x+=2*(buttonFrame(b)+buttonXPad(b));
  y+=2*(buttonFrame(b)+buttonYPad(b));

  x/=b->BWidth;
  y/=b->BHeight;

  *maxx=max(x,*maxx);
  *maxy=max(y,*maxy);
#ifdef DEBUG_LOADDATA
  fprintf(stderr,", size %ux%u, done\n",x,y);
#endif
}


static void HandlePanelPress(button_info *b)
{
  int x1, y1, w1, h1;
  int x2, y2, w2, h2;
  int px, py, pw, ph, pbw;
  int ax, ay, aw, ah, abw;
  int tb, bb, lb, rb;
  int steps = b->slide_steps;
  int i;
  unsigned int d;
  Window JunkW;
  Window ancestor;
  XSizeHints mysizehints;
  long supplied;
  Bool is_mapped;
  XWindowAttributes xwa;
  char cmd[64];
  button_info *tmp;

  if (XGetWindowAttributes(Dpy, b->PanelWin, &xwa))
    is_mapped = (xwa.map_state == IsViewable);
  else
    is_mapped = False;

  if (is_mapped &&
      XGetGeometry(Dpy, b->PanelWin, &JunkW, &b->x, &b->y, &b->w, &b->h,
		   &b->bw, &d))
  {
    /* get the current geometry */
    XTranslateCoordinates(
      Dpy, b->PanelWin, Root, b->x, b->y, &b->x, &b->y, &JunkW);
  }
  else
  {
    /* Make sure the icon is unmapped first. Needed to work properly with
     * shaded and iconified windows. */
    XWithdrawWindow(Dpy, b->PanelWin, screen);

    /* now request mapping in the void and wait until it gets mapped */
    mysizehints.flags = 0;
    XGetWMNormalHints(Dpy, b->PanelWin, &mysizehints, &supplied);
    mysizehints.flags |= USPosition;
    mysizehints.x = 32767;
    mysizehints.y = 32767;
    XSetWMNormalHints(Dpy, b->PanelWin, &mysizehints);

    /* map the window in the void */
    XMoveWindow(Dpy, b->PanelWin, 32767, 32767);
    XMapWindow(Dpy, b->PanelWin);
    XSync(Dpy, 0);

    /* give the X server the CPU to do something */
    usleep(1000);

    /* wait until it appears or one second has passed */
    for (i = 0; i < 10; i++)
    {
      if (!XGetWindowAttributes(Dpy, b->PanelWin, &xwa))
      {
	/* the window has been destroyed */
	XUnmapWindow(Dpy, b->PanelWin);
	XSync(Dpy, 0);
	RedrawButton(b, 1);
	return;
      }
      if (xwa.map_state == IsViewable)
      {
	/* okay, it's mapped */
	break;
      }
      /* still unmapped wait 0.1 seconds and try again */
      XSync(Dpy, 0);
      usleep(100000);
    }
    if (xwa.map_state != IsViewable)
    {
      /* give up after one second */
      XUnmapWindow(Dpy, b->PanelWin);
      XSync(Dpy, 0);
      RedrawButton(b, 1);
      return;
    }
  }

  /* We're sure that the window is mapped and decorated now. Find the top level
   * ancestor now. */
  MyXGrabServer(Dpy);
  lb = 0;
  tb = 0;
  rb = 0;
  bb = 0;
  if (!b->panel_flags.ignore_lrborder || !b->panel_flags.ignore_tbborder)
  {
    ancestor = GetTopAncestorWindow(Dpy, b->PanelWin);
    if (ancestor)
    {
      if (
	XGetGeometry(Dpy, ancestor, &JunkW, &ax, &ay, (unsigned int *)&aw,
		     (unsigned int *)&ah, (unsigned int *)&abw, &d) &&
	XGetGeometry(Dpy, b->PanelWin, &JunkW, &px, &py, (unsigned int *)&pw,
		     (unsigned int *)&ph, (unsigned int *)&pbw, &d) &&
	XTranslateCoordinates(
	  Dpy, b->PanelWin, ancestor, 0, 0, &px, &py, &JunkW))
      {
	/* calculate the 'border' width in all four directions */
	if (!b->panel_flags.ignore_lrborder)
	{
	  lb = max(px, 0) + abw;
	  rb = max((aw + abw) - (px + pw), 0) + abw;
	}
	if (!b->panel_flags.ignore_tbborder)
	{
	  tb = max(py, 0) + abw;
	  bb = max((ah + abw) - (py + ph), 0) + abw;
	}
      }
    }
  }
  /* now find the source and destination positions for the slide operation */
  GetPanelGeometry(is_mapped, b, lb, tb, rb, bb, &x1, &y1, &w1, &h1);
  GetPanelGeometry(!is_mapped, b, lb, tb, rb, bb, &x2, &y2, &w2, &h2);
  MyXUngrabServer(Dpy);

  /* to force fvwm to map the window where we want */
  if (is_mapped)
  {
    /* don't slide the window if it has been moved or resized */
    if (b->x != x1 || b->y != y1 || b->w != w1 || b->h != h1)
    {
      steps = 0;
    }
  }

  /* redraw our button */
  b->newflags.panel_mapped = !is_mapped;

  /* make sure the window maps on the current desk */
  sprintf(cmd, "Silent WindowId 0x%08x MoveToDesk 0", (int)b->PanelWin);
  SendInfo(fd, cmd, b->PanelWin);
  SlideWindow(Dpy, b->PanelWin,
	      x1, y1, w1, h1,
	      x2, y2, w2, h2,
	      steps, b->slide_delay_ms, NULL, b->panel_flags.smooth,
	      (b->swallow&b_NoHints) ? 0 : 1);
  if (is_mapped)
  {
    XUnmapWindow(Dpy, b->PanelWin);
  }
  tmp = CurrentButton;
  CurrentButton = NULL;
  RedrawButton(b, 1);
  CurrentButton = tmp;
  XSync(Dpy, 0);
  /* Give fvwm a chance to update the window.  Otherwise the window may end up
   * too small.  This doesn't prevent this completely, but makes it much less
   * likely. */
  usleep(250000);

  return;
}


/**
*** CreateUberButtonWindow()
*** Sizes and creates the window
**/
void CreateUberButtonWindow(button_info *ub,int maxx,int maxy)
{
  XSizeHints mysizehints;
  XGCValues gcv;
  unsigned long gcm;
  XClassHint myclasshints;
  XSetWindowAttributes xswa;

  x = UberButton->x; /* Geometry x where to put the panel */
  y = UberButton->y; /* Geometry y where to put the panel */
  xneg = UberButton->w;
  yneg = UberButton->h;

  if(maxx<16)
    maxx=16;
  if(maxy<16)
    maxy=16;

#ifdef DEBUG_INIT
  fprintf(stderr,"making atoms...");
#endif

  _XA_WM_DEL_WIN = XInternAtom(Dpy,"WM_DELETE_WINDOW",0);

#ifdef DEBUG_INIT
  fprintf(stderr,"sizing...");
#endif

  mysizehints.flags = PWinGravity | PBaseSize;
  mysizehints.base_width  = 0;
  mysizehints.base_height = 0;
  mysizehints.width  = mysizehints.base_width + maxx;
  mysizehints.height = mysizehints.base_height + maxy;

  if(w>-1) /* from geometry */
  {
#ifdef DEBUG_INIT
    fprintf(stderr,"constraining (w=%i)...",w);
#endif
    ConstrainSize(&mysizehints,&w,&h);
    mysizehints.width = w;
    mysizehints.height = h;
    mysizehints.flags |= USSize;
  }

#ifdef DEBUG_INIT
  fprintf(stderr,"gravity...");
#endif
  mysizehints.x=0;
  mysizehints.y=0;
  if(x > -100000)
  {
    if (xneg)
    {
      mysizehints.x = DisplayWidth(Dpy,screen) + x - mysizehints.width;
      gravity = NorthEastGravity;
    }
    else
      mysizehints.x = x;
    if (yneg)
    {
      mysizehints.y = DisplayHeight(Dpy,screen) + y - mysizehints.height;
      gravity = SouthWestGravity;
    }
    else
      mysizehints.y = y;
    if(xneg && yneg)
      gravity = SouthEastGravity;
    mysizehints.flags |= USPosition;
  }
  mysizehints.win_gravity = gravity;

#ifdef DEBUG_INIT
  if(mysizehints.flags&USPosition)
    fprintf(stderr,"create(%i,%i,%u,%u,1,%u,%u)...",
	    mysizehints.x,mysizehints.y,
	    mysizehints.width,mysizehints.height,
	    (ushort)fore_pix,(ushort)back_pix);
  else
    fprintf(stderr,"create(-,-,%u,%u,1,%u,%u)...",
	    mysizehints.width,mysizehints.height,
	    (ushort)fore_pix,(ushort)back_pix);
#endif

  xswa.colormap = Pcmap;
  xswa.border_pixel = 0;
  xswa.background_pixmap = None;
  MyWindow = XCreateWindow(Dpy,Root,mysizehints.x,mysizehints.y,
			   mysizehints.width,mysizehints.height,0,Pdepth,
			   InputOutput,Pvisual,
			   CWColormap|CWBackPixmap|CWBorderPixel,&xswa);
  if (is_transient)
  {
    XSetTransientForHint(Dpy, MyWindow, Root);
  }

#ifdef DEBUG_INIT
  fprintf(stderr,"colors...");
#endif
  if(Pdepth < 2)
  {
    back_pix = GetColor("white");
    fore_pix = GetColor("black");
    hilite_pix = back_pix;
    shadow_pix = fore_pix;
  }
  else
  {
    fore_pix = ub->c->fc;
    back_pix = ub->c->bc;
    hilite_pix = ub->c->hc;
    shadow_pix = ub->c->sc;
  }


#ifdef DEBUG_INIT
  fprintf(stderr,"properties...");
#endif
  XSetWMProtocols(Dpy,MyWindow,&_XA_WM_DEL_WIN,1);

  myclasshints.res_name=safestrdup(MyName);
  myclasshints.res_class=safestrdup("FvwmButtons");

  {
    XTextProperty mynametext;
    char *list[]={NULL,NULL};
    list[0] = MyName;
    if(!XStringListToTextProperty(list,1,&mynametext))
    {
      fprintf(stderr,"%s: Failed to convert name to XText\n",MyName);
      exit(1);
    }
    XSetWMProperties(Dpy,MyWindow,&mynametext,&mynametext,
		     NULL,0,&mysizehints,NULL,&myclasshints);
    XFree(mynametext.value);
  }

  XSelectInput(Dpy,MyWindow,MW_EVENTS);

#ifdef DEBUG_INIT
  fprintf(stderr,"GC...");
#endif
  gcm = GCForeground|GCBackground;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  if(ub && ub->c && ub->c->font && ub->font)
  {
    gcv.font = ub->c->font->fid;
    gcm |= GCFont;
  }
  NormalGC = fvwmlib_XCreateGC(Dpy, MyWindow, gcm, &gcv);
  gcv.foreground = shadow_pix;
  gcv.background = fore_pix;
  ShadowGC = fvwmlib_XCreateGC(Dpy, MyWindow, gcm, &gcv);

  if (ub->c->flags&b_Colorset)
  {
    SetWindowBackground(
      Dpy, MyWindow, mysizehints.width, mysizehints.height,
      &Colorset[ub->c->colorset], Pdepth, NormalGC, True);
  }
  else if (ub->c->flags&b_IconBack && !(ub->c->flags&b_TransBack))
  {
    XSetWindowBackgroundPixmap(Dpy,MyWindow,ub->c->backicon->picture);
  }
  else
  {
    XSetWindowBackground(Dpy,MyWindow,back_pix);
  }

  free(myclasshints.res_class);
  free(myclasshints.res_name);
}

/* ----------------------------- color functions --------------------------- */

#ifdef XPM
#  define MyAllocColor(a,b,c) PleaseAllocColor(c)
#else
#  define MyAllocColor(a,b,c) XAllocColor(a,b,c)
#endif

/**
*** PleaseAllocColor()
*** Allocs a color through XPM, which does it's closeness thing if out of
*** space.
**/
#ifdef XPM
int PleaseAllocColor(XColor *color)
{
  char *xpm[] = {"1 1 1 1",NULL,"x"};
  XpmAttributes attr;
  XImage *dummy1=None,*dummy2=None;
  static char buf[20];

  sprintf(buf,"x c #%04x%04x%04x",color->red,color->green,color->blue);
  xpm[1]=buf;
  attr.valuemask=XpmCloseness|XpmVisual|XpmColormap|XpmDepth;
  attr.closeness=40000; /* value used by fvwm and fvwmlib */
  attr.visual = Pvisual;
  attr.colormap = Pcmap;
  attr.depth = Pdepth;

  if(XpmCreateImageFromData(Dpy,xpm,&dummy1,&dummy2,&attr)!=XpmSuccess)
  {
    fprintf(stderr,"%s: Unable to get similar color\n",MyName);
    exit(1);
  }
  color->pixel=XGetPixel(dummy1,0,0);
  if(dummy1!=None)XDestroyImage(dummy1);
  if(dummy2!=None)XDestroyImage(dummy2);
  return 1;
}
#endif

/**
*** nocolor()
*** Complain
**/
void nocolor(const char *a, const char *b)
{
  fprintf(stderr,"%s: Can't %s %s, quitting, sorry...\n", MyName, a,b);
  exit(1);
}


/* --------------------------------------------------------------------------*/

#ifdef DEBUG_EVENTS
void DebugEvents(XEvent *event)
{
  char *event_names[] =
  {
    NULL,
    NULL,
    "KeyPress",
    "KeyRelease",
    "ButtonPress",
    "ButtonRelease",
    "MotionNotify",
    "EnterNotify",
    "LeaveNotify",
    "FocusIn",
    "FocusOut",
    "KeymapNotify",
    "Expose",
    "GraphicsExpose",
    "NoExpose",
    "VisibilityNotify",
    "CreateNotify",
    "DestroyNotify",
    "UnmapNotify",
    "MapNotify",
    "MapRequest",
    "ReparentNotify",
    "ConfigureNotify",
    "ConfigureRequest",
    "GravityNotify",
    "ResizeRequest",
    "CirculateNotify",
    "CirculateRequest",
    "PropertyNotify",
    "SelectionClear",
    "SelectionRequest",
    "SelectionNotify",
    "ColormapNotify",
    "ClientMessage",
    "MappingNotify"
  };
  fprintf(stderr,"%s: Received %s event from window 0x%x\n",
	  MyName,event_names[event->type],(ushort)event->xany.window);
}
#endif

#ifdef DEBUG_FVWM
void DebugFvwmEvents(unsigned long type)
{
  char *events[] =
  {
    "M_NEW_PAGE",
    "M_NEW_DESK",
    "M_ADD_WINDOW",
    "M_RAISE_WINDOW",
    "M_LOWER_WINDOW",
    "M_CONFIGURE_WINDOW",
    "M_FOCUS_CHANGE",
    "M_DESTROY_WINDOW",
    "M_ICONIFY",
    "M_DEICONIFY",
    "M_WINDOW_NAME",
    "M_ICON_NAME",
    "M_RES_CLASS",
    "M_RES_NAME",
    "M_END_WINDOWLIST",
    "M_ICON_LOCATION",
    "M_MAP",
    "M_ERROR",
    "M_CONFIG_INFO",
    "M_END_CONFIG_INFO",
    "M_ICON_FILE",
    "M_DEFAULTICON",
    "M_ADD_WINDOW",
    "M_CONFIGURE_WINDOW",NULL};
  int i=0;
  while(events[i])
  {
    if(type&1<<i)
      fprintf(stderr,"%s: Received %s message from fvwm\n",MyName,events[i]);
    i++;
  }
}
#endif

/**
*** My_XNextEvent()
*** Waits for next X event, or for an auto-raise timeout.
**/
int My_XNextEvent(Display *Dpy, XEvent *event)
{
  fd_set in_fdset;
  static int miss_counter = 0;

  if(XPending(Dpy))
  {
    XNextEvent(Dpy,event);
#ifdef DEBUG_EVENTS
    DebugEvents(event);
#endif
    return 1;
  }

  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
  FD_SET(fd[1],&in_fdset);

  if (fvwmSelect(fd_width,SELECT_FD_SET_CAST &in_fdset, 0, 0, NULL) > 0)
  {

    if(FD_ISSET(x_fd, &in_fdset))
    {
      if(XPending(Dpy))
      {
	XNextEvent(Dpy,event);
	miss_counter = 0;
#ifdef DEBUG_EVENTS
	DebugEvents(event);
#endif
	return 1;
      }
      else
	miss_counter++;
      if(miss_counter > 100)
	DeadPipe(0);
    }

    if(FD_ISSET(fd[1], &in_fdset))
    {
      FvwmPacket* packet = ReadFvwmPacket(fd[1]);
      if ( packet == NULL )
	DeadPipe(0);
      else
	process_message( packet->type, packet->body );
    }

  }
  return 0;
}

/**
*** SpawnSome()
*** Is run after the windowlist is checked. If any button hangs on UseOld,
*** it has failed, so we try to spawn a window for them.
**/
void SpawnSome(void)
{
  static char first=1;
  button_info *b,*ub=UberButton;
  int button=-1;
  char *p;

  if(!first)
    return;
  first=0;
  while(NextButton(&ub,&b,&button,0))
    if((buttonSwallowCount(b)==1) && b->flags&b_Hangon &&
       buttonSwallow(b)&b_UseOld)
      if(b->spawn)
      {
#ifdef DEBUG_HANGON
	fprintf(stderr,"%s: Button 0x%06x did not find a \"%s\" window, %s",
		MyName,(ushort)b,b->hangon,"spawning own\n");
#endif
	p = expand_action(b->spawn, NULL);
	if (p)
	{
	  MySendText(fd, p, 0);
	  free(p);
	}
      }
}

/* This function is a real hack. It forces our nice background upon the
 * windows of the swallowed application! */
void change_swallowed_window_colorset(button_info *b, Bool do_clear)
{
  Window w = SwallowedWindow(b);
  Window *children = None;
  int nchildren;

  nchildren = GetEqualSizeChildren(
    Dpy, w, Pdepth, XVisualIDFromVisual(Pvisual), Pcmap, &children);
  SetWindowBackground(
    Dpy, w, buttonWidth(b), buttonHeight(b),
    &Colorset[b->colorset], Pdepth, NormalGC, do_clear);
  while (nchildren-- > 0)
  {
    SetWindowBackground(
      Dpy, children[nchildren], buttonWidth(b), buttonHeight(b),
      &Colorset[b->colorset], Pdepth, NormalGC, do_clear);
  }
  if (children)
    XFree(children);

  return;
}

static void recursive_change_colorset(container_info *c, int colorset)
{
  int i;

  for (i = c->num_buttons; --i >= 0; )
  {
    button_info *b = c->buttons[i];

    if (!b)
      continue;
    if (b->flags & b_Container)
    {
      if (buttonColorset(b) == colorset)
      {
	/* re-apply colorset to button */
	RedrawButton(b, True);
      }
      /* recursively update containers */
      recursive_change_colorset(b->c, colorset);
    }
    else if (b->flags & b_Swallow)
    {
      /* swallowed window */
      if ((buttonSwallowCount(b) == 3) && (b->flags & b_Swallow) &&
	  (b->flags & b_Colorset) && (b->IconWin != None) &&
          (buttonColorset(b) == colorset))
      {
	/* re-apply colorset to window background */
	change_swallowed_window_colorset(b, True);
      }
    }
    else
    {
      /* simple button */
      if (buttonColorset(b) == colorset)
      {
	/* re-apply colorset to button */
	RedrawButton(b, True);
      }
    }
  }

  return;
}

static void change_colorset(int colorset)
{
  if (UberButton->c->flags & b_Colorset && colorset == UberButton->c->colorset)
  {
    SetWindowBackground(
      Dpy, MyWindow, UberButton->c->width, UberButton->c->height,
      &Colorset[colorset], Pdepth, NormalGC, True);
  }

  recursive_change_colorset(UberButton->c, colorset);

  return;
}

static void handle_config_info_packet(unsigned long *body)
{
  char *tline, *token;
  int colorset;

  tline = (char*)&(body[3]);
  token = PeekToken(tline, &tline);
  if (StrEquals(token, "Colorset"))
  {
    colorset = LoadColorset(tline);
    change_colorset(colorset);
  }
  else if (StrEquals(token, XINERAMA_CONFIG_STRING))
  {
    FScreenConfigureModule(tline);
  }

  return;
}


/**
*** process_message()
*** Process window list messages
**/
void process_message(unsigned long type,unsigned long *body)
{
  struct ConfigWinPacket  *cfgpacket;
#ifdef DEBUG_FVWM
  DebugFvwmEvents(type);
#endif

  switch(type)
  {
  case M_NEW_DESK:
    new_desk = body[0];
    RedrawWindow(NULL);
    break;
  case M_END_WINDOWLIST:
    SpawnSome();
    break;
  case M_MAP:
    swallow(body);
    break;
  case M_RES_NAME:
  case M_RES_CLASS:
  case M_WINDOW_NAME:
    CheckForHangon(body);
    break;
  case M_CONFIG_INFO:
    handle_config_info_packet((unsigned long*)body);
    break;
  case M_ADD_WINDOW:
  case M_CONFIGURE_WINDOW:
    cfgpacket = (void *) body;
    if (cfgpacket->w == MyWindow)
    {
      button_lborder = button_rborder = button_tborder = button_bborder
	= cfgpacket->border_width;
      if (HAS_BOTTOM_TITLE(cfgpacket))
	button_bborder += cfgpacket->title_height;
      else
	button_tborder += cfgpacket->title_height;
    }
    break;
  default:
    break;
  }
}


/* --------------------------------- swallow code -------------------------- */

/**
*** CheckForHangon()
*** Is the window here now?
**/
void CheckForHangon(unsigned long *body)
{
  button_info *b,*ub=UberButton;
  int button=-1;
  ushort d;
  char *cbody;
  cbody = (char *)&body[3];

  while(NextButton(&ub,&b,&button,0))
  {
    if(b->flags&b_Hangon && strcmp(cbody,b->hangon)==0)
    {
      /* Is this a swallowing button in state 1? */
      if(buttonSwallowCount(b)==1)
      {
	b->swallow&=~b_Count;
	b->swallow|=2;

	/* must grab the server here to make sure the window is not swallowed
	 * by some other application. */
	if (b->flags & b_Panel)
	  b->PanelWin=(Window)body[0];
	else
	  b->IconWin=(Window)body[0];
	b->flags&=~b_Hangon;

	/* We get the parent of the window to compare with later... */
	b->IconWinParent=
	  GetRealGeometry(Dpy, SwallowedWindow(b), &b->x, &b->y,
			  &b->w, &b->h, &b->bw, &d);

#ifdef DEBUG_HANGON
	fprintf(stderr,"%s: Button 0x%06x %s 0x%lx \"%s\", parent 0x%lx\n",
		MyName,(ushort)b,"will swallow window",body[0],cbody,
		b->IconWinParent);
#endif

	if(buttonSwallow(b)&b_UseOld)
	  swallow(body);
      }
      else
      {
	/* Else it is an executing button with a confirmed kill */
#ifdef DEBUG_HANGON
	fprintf(stderr,"%s: Button 0x%06x %s 0x%lx \"%s\", released\n",
		MyName,(int)b,"hung on window",body[0],cbody);
#endif
	b->flags&=~b_Hangon;
	free(b->hangon);
	b->hangon=NULL;
	RedrawButton(b,0);
      }
      break;
    }
    else if(buttonSwallowCount(b)>=2 && (Window)body[0] == SwallowedWindow(b))
      break;      /* This window has already been swallowed by someone else! */
  }
}

/**
*** GetRealGeometry()
*** Traverses window tree to find the real x,y of a window. Any simpler?
*** Returns parent window, or None if failed.
**/
Window GetRealGeometry(
  Display *dpy, Window win, int *x, int *y, unsigned int *w, unsigned int *h,
  unsigned int *bw, unsigned int *d)
{
  Window root;
  Window rp=None;
  Window *children = None;
  unsigned int n;

  if(!XGetGeometry(dpy,win,&root,x,y,w,h,bw,d))
    return None;

  /* Now, x and y are not correct. They are parent relative, not root... */
  /* Hmm, ever heard of XTranslateCoordinates!? */
  XTranslateCoordinates(dpy,win,root,*x,*y,x,y,&rp);
  if (!XQueryTree(dpy,win,&root,&rp,&children,&n))
    return None;
  if (children)
    XFree(children);

  return rp;
}

static void GetPanelGeometry(
  Bool get_big, button_info *b, int lb, int tb, int rb, int bb,
  int *x, int *y, int *w, int *h)
{
  int bx, by, bw, bh;
  int  ftb = 0, fbb = 0, frb = 0, flb = 0;
  Window JunkWin;

  /* take in acount or not the FvwmButtons borders width */
  if (b->panel_flags.buttons_tbborder)
  {
    ftb = button_tborder;
    fbb = button_bborder;
  }
  if (b->panel_flags.buttons_lrborder)
  {
    frb = button_rborder;
    flb = button_lborder;
  }

  switch (b->slide_context)
  {
  case SLIDE_CONTEXT_PB: /* slide relatively to the panel button */
    bx = buttonXPos(b, b->n);
    by = buttonYPos(b, b->n);
    bw = buttonWidth(b);
    bh = buttonHeight(b);
    XTranslateCoordinates(Dpy, MyWindow, Root, bx, by, &bx, &by, &JunkWin);
    break;
  case SLIDE_CONTEXT_MODULE: /* slide relatively to FvwmButtons */
    {
      unsigned int dum, dum2;

      if (XGetGeometry(
	Dpy, MyWindow, &JunkWin, &bx, &by, (unsigned int *)&bw,
	(unsigned int *)&bh, (unsigned int *)&dum, &dum2))
      {
	XTranslateCoordinates(Dpy, MyWindow, Root, bx, by, &bx, &by, &JunkWin);
	break;
      }
      else
      {
	/* fall thorugh to SLIDE_CONTEXT_ROOT */
      }
    }
  case SLIDE_CONTEXT_ROOT: /* slide relatively to the root windows */
    switch (b->slide_direction)
    {
    case SLIDE_UP:
      bx = 0;
      by = dph;
      break;
    case SLIDE_DOWN:
      bx = 0;
      by = -dph;
      break;
    case SLIDE_RIGHT:
      bx = -dpw;
      by = 0;
      break;
    case SLIDE_LEFT:
      bx = dpw;
      by = 0;
      break;
    }
    bw = dpw;
    bh = dph;
    break;
  }

  /* relative corrections in % or pixel */
  *x = 0;
  *y = 0;
  if (b->relative_x != 0)
  {
    if (b->panel_flags.relative_x_pixel)
      *x = b->relative_x;
    else
      *x = (int)(b->relative_x * bw) / 100;
  }
  if (b->relative_y != 0)
  {
    if (b->panel_flags.relative_y_pixel)
      *y = b->relative_y;
    else
      *y = (int)(b->relative_y * bh) / 100;
  }

  switch (b->slide_direction)
  {
  case SLIDE_UP:
    switch (b->slide_position)
    {
    case SLIDE_POSITION_CENTER:
      *x += bx + (int)(bw - (b->w + (rb - lb)) + (frb - flb)) / 2;
      break;
    case SLIDE_POSITION_LEFT_TOP:
      *x += bx + lb - flb;
      break;
    case SLIDE_POSITION_RIGHT_BOTTOM:
      *x += bx + bw - b->w - rb + frb;
      break;
    }
    *w = b->w ;
    if (get_big)
    {
      *y += by - (int)b->h - bb - ftb;
      *h = b->h;
    }
    else
    {
      *y += by - bb - ftb;
      *h = 0;
    }
    break;
  case SLIDE_DOWN:
   switch (b->slide_position)
    {
    case SLIDE_POSITION_CENTER:
      *x += bx + (int)(bw - (b->w + (rb - lb)) + (frb - flb)) / 2;
      break;
    case SLIDE_POSITION_LEFT_TOP:
      *x += bx + lb - flb;
      break;
    case SLIDE_POSITION_RIGHT_BOTTOM:
      *x += bx + bw - b->w - rb + frb;
      break;
    }
    *y += by + (int)bh + tb + fbb;
    *w = b->w;
    if (get_big)
    {
      *h = b->h;
    }
    else
    {
      *h = 0;
    }
    break;
  case SLIDE_LEFT:
    switch (b->slide_position)
    {
    case SLIDE_POSITION_CENTER:
      *y += by + (int)(bh - b->h + (tb - bb) + (fbb - ftb)) / 2;
      break;
    case SLIDE_POSITION_LEFT_TOP:
      *y += by + tb - ftb;
      break;
    case SLIDE_POSITION_RIGHT_BOTTOM:
      *y += by + bh - b->h - bb + fbb;
      break;
    }
    *h = b->h;
    if (get_big)
    {
      *x += bx - (int)b->w - rb - flb;
      *w = b->w;
    }
    else
    {
      *x += bx - rb - flb;
      *w = 0;
    }
    break;
  case SLIDE_RIGHT:
    switch (b->slide_position)
    {
    case SLIDE_POSITION_CENTER:
      *y += by + (int)(bh - b->h + (tb - bb) + (fbb - ftb)) / 2;
      break;
    case SLIDE_POSITION_LEFT_TOP:
      *y += by + tb - ftb;
      break;
    case SLIDE_POSITION_RIGHT_BOTTOM:
      *y += by + bh - b->h - bb + fbb;
      break;
    }
    *x += bx + (int)bw + lb + frb;
    *h = b->h;
    if (get_big)
    {
      *w = b->w;
    }
    else
    {
      *w = 0;
    }
    break;
  }

  return;
}


/**
*** swallow()
*** Executed when swallowed windows get mapped
**/
void swallow(unsigned long *body)
{
  char *temp;
  button_info *ub=UberButton,*b;
  int button=-1;
  unsigned int d;
  Window p;

  while(NextButton(&ub,&b,&button,0))
  {
    Window swin = SwallowedWindow(b);

    if((swin == (Window)body[0]) && (buttonSwallowCount(b)==2))
    {
      /* Store the geometry in case we need to unswallow. Get parent */
      p = GetRealGeometry(Dpy,swin,&b->x,&b->y,&b->w,&b->h,&b->bw,&d);
#ifdef DEBUG_HANGON
      fprintf(stderr,"%s: Button 0x%06x %s 0x%lx, with parent 0x%lx\n",
	      MyName,(ushort)b,"trying to swallow window",body[0],p);
#endif

      if(p==None) /* This means the window is no more */ /* NO! wrong */
      {
	fprintf(stderr,"%s: Window 0x%lx (\"%s\") disappeared %s\n",
		MyName,swin,b->hangon,"before swallow complete");
	/* Now what? Nothing? For now: give up that button */
	b->flags&=~(b_Hangon|b_Swallow|b_Panel);
	return;
      }

      if (p != b->IconWinParent)
      {
	/* The window has been reparented */
	fprintf(stderr,"%s: Window 0x%lx (\"%s\") was %s (window 0x%lx)\n",
		MyName,swin,b->hangon,"grabbed by someone else",p);

	/* Request a new windowlist, we might have ignored another
	   matching window.. */
	SendText(fd,"Send_WindowList",0);

	/* Back one square and lose one turn */
	b->swallow&=~b_Count;
	b->swallow|=1;
	b->flags|=b_Hangon;
	return;
      }

#ifdef DEBUG_HANGON
      fprintf(stderr,"%s: Button 0x%06x swallowed window 0x%lx\n",
	      MyName,(ushort)b,body[0]);
#endif

      if (b->flags & b_Swallow)
      {
	/* "Swallow" the window! Place it in the void so we don't see it
	 * until it's MoveResize'd */
	MyXGrabServer(Dpy);
	do_allow_bad_access = True;
	XSelectInput(Dpy,swin,SW_EVENTS);
	XSync(Dpy, 0);
	do_allow_bad_access = False;
	if (was_bad_access)
	{
	  /* A BadAccess error means that the window is already swallowed by
	   * someone else. Wait for another window. */
	  was_bad_access = False;
	  /* Back one square and lose one turn */
	  b->swallow&=~b_Count;
	  b->swallow|=1;
	  b->flags|=b_Hangon;
	  MyXUngrabServer(Dpy);
	  return;
	}
	/*error checking*/
	XReparentWindow(Dpy,swin,MyWindow,-32768,-32768);
	XSync(Dpy, 0);
	MyXUngrabServer(Dpy);
	XSync(Dpy, 0);
	usleep(50000);
	b->swallow &= ~b_Count;
	b->swallow |= 3;
	if (buttonSwallow(b) & b_UseTitle)
	{
	  if (b->flags & b_Title)
	    free(b->title);
	  b->flags |= b_Title;
	  if (XFetchName(Dpy, swin, &temp))
	  {
	    CopyString(&b->title, temp);
	    XFree(temp);
	  }
	  else
	  {
	    CopyString(&b->title, "");
	  }
	}

	MakeButton(b);
	XMapWindow(Dpy, swin);
	XSync(Dpy, 0);

	if (b->flags & b_Colorset)
	{
	  /* A short delay to give the application the chance to set the
	   * background itself, so we can override it. If we don't do this, the
	   * application may override our background. On the other hand it may
	   * still override our background, but our chances are a bit better.
	   */
	  usleep(250000);
	  XSync(Dpy, 0);
	  change_swallowed_window_colorset(b, True);
	}
	if (FShapesSupported)
	{
	  if (UberButton->c->flags & b_TransBack)
	  {
	    SetTransparentBackground(UberButton, Width, Height);
	    FShapeSelectInput(Dpy, swin, FShapeNotifyMask);
	  }
	}
	/* Redraw and force cleaning the background to erase the old button
	 * title. */
	RedrawButton(b, 2);
      }
      else /* (b->flags & b_Panel) */
      {
	b->swallow &= ~b_Count;
	b->swallow |= 3;
	XSelectInput(Dpy, swin, PA_EVENTS);
	if (!XWithdrawWindow(Dpy, swin, screen))
	{
	  /* oops. what can we do now? let's pretend the unmap worked. */
	}
	b->newflags.panel_mapped = 0;
      }
      break;
    }
  }
}
