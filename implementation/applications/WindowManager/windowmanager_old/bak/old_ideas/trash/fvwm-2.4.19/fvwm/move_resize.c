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

/*****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/************************************************************************
 *
 * code for moving and resizing windows
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/Colorset.h"
#include "libs/FScreen.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "move_resize.h"
#include "module_interface.h"
#include "focus.h"
#include "borders.h"
#include "geometry.h"
#include "gnome.h"
#include "colormaps.h"
#include "virtual.h"
#include "decorations.h"
#include "events.h"
#include <X11/keysym.h>

/* ----- move globals ----- */
extern XEvent Event;

#define SNAP_NONE    0x00
#define SNAP_WINDOWS 0x01
#define SNAP_ICONS   0x02
#define SNAP_SAME    0x04
#define SNAP_SCREEN  0x08

#define MOVE_NORMAL  0x00
#define MOVE_PAGE    0x01
#define MOVE_SCREEN  0x02

/* Animated move stuff added by Greg J. Badros, gjb@cs.washington.edu */

float rgpctMovementDefault[32] =
{
  -.01, 0, .01, .03,.08,.18,.3,.45,.60,.75,.85,.90,.94,.97,.99,1.0
  /* must end in 1.0 */
};
int cmsDelayDefault = 10; /* milliseconds */

/* current geometry of size window */
static rectangle sizew_g =
{
  -1,
  -1,
  -1,
  -1
};

/* ----- end of move globals ----- */

/* ----- resize globals ----- */

/* DO NOT USE (STATIC) GLOBALS IN THIS MODULE!
 * Since some functions are called from other modules unwanted side effects
 * (i.e. bugs.) would be created */

extern Window PressedW;

static void DoResize(
  int x_root, int y_root, FvwmWindow *tmp_win, rectangle *drag,
  rectangle *orig, int *xmotionp, int *ymotionp, Bool do_resize_opaque);
static void DisplaySize(
  FvwmWindow *tmp_win, XEvent *eventp, int width, int height, Bool Init,
  Bool resetLast);
static void draw_move_resize_grid(int x, int  y, int  width, int height);

/* ----- end of resize globals ----- */

/* The vars are named for the x-direction, but this is used for both x and y */
static int GetOnePositionArgument(
  char *s1,int x,int w,int *pFinalX,float factor, int max, Bool is_x)
{
  int val;
  int cch = strlen(s1);
  Bool add_pointer_position = False;

  if (cch == 0)
    return 0;
  if (s1[cch-1] == 'p')
  {
    factor = 1;  /* Use pixels, so don't multiply by factor */
    s1[cch-1] = '\0';
  }
  if (strcmp(s1,"w") == 0)
  {
    *pFinalX = x;
  }
  else if (sscanf(s1,"w-%d",&val) == 1)
  {
    *pFinalX = x - (int)(val*factor);
  }
  else if (sscanf(s1,"w+%d",&val) == 1 || sscanf(s1,"w%d",&val) == 1 )
  {
    *pFinalX = x + (int)(val*factor);
  }
  else if (sscanf(s1,"m-%d",&val) == 1)
  {
    add_pointer_position = True;
    *pFinalX = -(int)(val*factor);
  }
  else if (sscanf(s1,"m+%d",&val) == 1 || sscanf(s1,"m%d",&val) == 1 )
  {
    add_pointer_position = True;
    *pFinalX = (int)(val*factor);
  }
  else if (sscanf(s1,"-%d",&val) == 1)
  {
    *pFinalX = max-w - (int)(val*factor);
  }
  else if (sscanf(s1,"%d",&val) == 1)
  {
    *pFinalX = (int)(val*factor);
  }
  else
  {
    return 0;
  }
  if (add_pointer_position)
  {
    int x = 0;
    int y = 0;

    XQueryPointer(
      dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX, &JunkY, &x, &y, &JunkMask);
    *pFinalX += (is_x) ? x : y;
  }

  return 1;
}

/* GetMoveArguments is used for Move & AnimatedMove
 * It lets you specify in all the following ways
 *   20  30          Absolute percent position, from left edge and top
 *  -50  50          Absolute percent position, from right edge and top
 *   10p 5p          Absolute pixel position
 *   10p -0p         Absolute pixel position, from bottom
 *  w+5  w-10p       Relative position, right 5%, up ten pixels
 *  w+5  w-10p       Pointer relative position, right 5%, up ten pixels
 * Returns 2 when x & y have parsed without error, 0 otherwise
 */
static int GetMoveArguments(
  char **paction, int w, int h, int *pFinalX, int *pFinalY,
  Bool *fWarp, Bool *fPointer)
{
  char *s1 = NULL;
  char *s2 = NULL;
  char *warp = NULL;
  char *action;
  char *naction;
  int scrWidth = Scr.MyDisplayWidth;
  int scrHeight = Scr.MyDisplayHeight;
  int retval = 0;

  if (!paction)
    return 0;
  action = *paction;
  action = GetNextToken(action, &s1);
  if (s1 && fPointer && StrEquals(s1, "pointer"))
  {
      *fPointer = True;
      *paction = action;
      free(s1);
      return 0;
  }
  else
  {
    action = GetNextToken(action, &s2);
    if (fWarp)
    {
      warp = PeekToken(action, &naction);
      if (StrEquals(warp, "Warp"))
      {
	*fWarp = True;
	action = naction;
      }
    }
  }

  if (s1 != NULL && s2 != NULL)
  {
    retval = 0;
    if (StrEquals(s1, "keep"))
    {
      retval++;
    }
    else if (GetOnePositionArgument(
      s1, *pFinalX, w, pFinalX, (float)scrWidth/100, scrWidth, True))
    {
      retval++;
    }
    if (StrEquals(s2, "keep"))
    {
      retval++;
    }
    else if (GetOnePositionArgument(
      s2, *pFinalY, h, pFinalY, (float)scrHeight/100, scrHeight, False))
    {
      retval++;
    }
    if (retval == 0)
      *fWarp = False; /* make sure warping is off for interactive moves */
  }
  else
  {
    /* not enough arguments, switch to current page. */
    while (*pFinalX < 0)
    {
      *pFinalX = Scr.MyDisplayWidth + *pFinalX;
    }
    while (*pFinalY < 0)
    {
      *pFinalY = Scr.MyDisplayHeight + *pFinalY;
    }
  }

  if (s1)
    free(s1);
  if (s2)
    free(s2);

  *paction = action;
  return retval;
}

static int ParseOneResizeArgument(
  char *arg, int scr_size, int base_size, int size_inc, int bw, int title_size,
  int *ret_size)
{
  int unit_table[3];
  int value;
  int suffix;

  if (StrEquals(arg, "keep"))
  {
    /* do not change width */
  }
  else
  {
    if (GetSuffixedIntegerArguments(arg, NULL, &value, 1, "pc", &suffix) < 1)
    {
      return 0;
    }
    else
    {
      /* convert the value/suffix pairs to pixels */
      unit_table[0] = scr_size;
      unit_table[1] = 100;
      unit_table[2] = 100 * size_inc;
      *ret_size = SuffixToPercentValue(value, suffix, unit_table);
      if (*ret_size < 0)
	*ret_size += scr_size;
      else
      {
	if (suffix == 2)
	{
	  /* account for base width */
	  *ret_size += base_size;
	}
	*ret_size += title_size + 2 * bw;
      }
    }
  }

  return 1;
}

static int GetResizeArguments(
  char **paction, int x, int y, int w_base, int h_base, int w_inc, int h_inc,
  int bw, int h_title, int *pFinalW, int *pFinalH)
{
  int n;
  char *naction;
  char *token;
  char *s1;
  char *s2;

  if (!paction)
    return 0;

  token = PeekToken(*paction, &naction);
  if (!token)
    return 0;
  if (StrEquals(token, "bottomright") || StrEquals(token, "br"))
  {
    int nx = x + *pFinalW - 1;
    int ny = y + *pFinalH - 1;

    n = GetMoveArguments(&naction, 0, 0, &nx, &ny, NULL, NULL);
    if (n < 2)
      return 0;
    *pFinalW = nx - x + 1;
    *pFinalH = ny - y + 1;
    *paction = naction;
  }
  else
  {
    s1 = safestrdup(token);
    naction = GetNextToken(naction, &s2);
    if (!s2)
    {
      free(s1);
      return 0;
    }
    *paction = naction;

    n = 0;
    n += ParseOneResizeArgument(
      s1, Scr.MyDisplayWidth, w_base, w_inc, bw, 0, pFinalW);
    n += ParseOneResizeArgument(
      s2, Scr.MyDisplayHeight, h_base, h_inc, bw, h_title, pFinalH);
    free(s1);
    free(s2);
    if (n < 2)
      n = 0;
  }

  return n;
}

static int GetResizeMoveArguments(
  char **paction, int w_base, int h_base, int w_inc, int h_inc, int bw,
  int h_title, int *pFinalX, int *pFinalY, int *pFinalW, int *pFinalH,
  Bool *fWarp, Bool *fPointer)
{
  char *action = *paction;

  if (!paction)
    return 0;
  if (GetResizeArguments(
    &action, *pFinalX, *pFinalY, w_base, h_base, w_inc, h_inc, bw, h_title,
    pFinalW, pFinalH) < 2)
  {
    return 0;
  }
  if (GetMoveArguments(
    &action, *pFinalW, *pFinalH, pFinalX, pFinalY, fWarp, NULL) < 2)
  {
    return 0;
  }
  *paction = action;

  return 4;
}

/* Positions the SizeWindow on the current ("moused") xinerama-screen */
static void position_geometry_window(XEvent *eventp)
{
  int x;
  int y;
  fscreen_scr_arg fscr;

  fscr.mouse_ev = eventp;
  /* Probably should remove this positioning code from {builtins,fvwm}.c? */
  if (Scr.gs.EmulateMWM)
  {
    FScreenCenterOnScreen(
      &fscr, FSCREEN_CURRENT, &x, &y, sizew_g.width, sizew_g.height);
  }
  else
  {
    FScreenGetScrRect(&fscr, FSCREEN_CURRENT, &x, &y, NULL, NULL);
  }
  if (x != sizew_g.x || y != sizew_g.y)
  {
    switch_move_resize_grid(False);
    XMoveWindow(dpy, Scr.SizeWindow, x, y);
    switch_move_resize_grid(True);
    sizew_g.x = x;
    sizew_g.y = y;
  }

  return;
}

void resize_geometry_window(void)
{
  int w;
  int h;
  int cset = Scr.DefaultColorset;

  Scr.SizeStringWidth =
    XTextWidth(Scr.DefaultFont.font, GEOMETRY_WINDOW_STRING,
	       sizeof(GEOMETRY_WINDOW_STRING) - 1);
  w = Scr.SizeStringWidth + 2 * GEOMETRY_WINDOW_BW;
  h = Scr.DefaultFont.height + 2 * GEOMETRY_WINDOW_BW;
  if (w != sizew_g.width || h != sizew_g.height)
  {
    XResizeWindow(dpy, Scr.SizeWindow, w, h);
    sizew_g.width = w;
    sizew_g.height = h;
  }
  if (cset >= 0)
  {
    SetWindowBackground(
      dpy, Scr.SizeWindow, w, h, &Colorset[cset], Pdepth, Scr.StdGC,
      False);
  }
  else
  {
    XSetWindowBackground(dpy, Scr.SizeWindow, Scr.StdBack);
  }

  return;
}

/***********************************************************************
 *
 *  Procedure:
 *      DisplayPosition - display the position in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current fvwm window
 *      x, y    - position of the window
 *
 ************************************************************************/

static void DisplayPosition(
  FvwmWindow *tmp_win, XEvent *eventp, int x, int y,int Init)
{
  char str[100];
  int offset;
  fscreen_scr_arg fscr;

  if (Scr.gs.do_hide_position_window)
    return;
  position_geometry_window(eventp);
  /* Translate x,y into local screen coordinates, in case Xinerama is used. */
  fscr.xypos.x = x;
  fscr.xypos.y = y;
  FScreenTranslateCoordinates(
    NULL, FSCREEN_GLOBAL, &fscr, FSCREEN_XYPOS, &x, &y);
  (void)sprintf(str, GEOMETRY_WINDOW_POS_STRING, x, y);
  if (Init)
  {
    XClearWindow(dpy, Scr.SizeWindow);
  }
  else
  {
    /* just clear indside the relief lines to reduce flicker */
    XClearArea(dpy,Scr.SizeWindow, GEOMETRY_WINDOW_BW, GEOMETRY_WINDOW_BW,
	       Scr.SizeStringWidth, Scr.DefaultFont.height, False);
  }

  if (Pdepth >= 2)
  {
    RelieveRectangle(
      dpy, Scr.SizeWindow, 0, 0,
      Scr.SizeStringWidth + GEOMETRY_WINDOW_BW * 2 - 1,
      Scr.DefaultFont.height + GEOMETRY_WINDOW_BW * 2 - 1,
      Scr.StdReliefGC, Scr.StdShadowGC, GEOMETRY_WINDOW_BW);
  }
  offset = (Scr.SizeStringWidth -
	    XTextWidth(Scr.DefaultFont.font, str, strlen(str))) / 2;
  offset += GEOMETRY_WINDOW_BW;
#ifdef I18N_MB
  XmbDrawString(dpy, Scr.SizeWindow, Scr.DefaultFont.fontset, Scr.StdGC,
		offset,
		Scr.DefaultFont.font->ascent + GEOMETRY_WINDOW_BW,
		str, strlen(str));
#else
  XDrawString(dpy, Scr.SizeWindow, Scr.StdGC,
	      offset,
	      Scr.DefaultFont.font->ascent + GEOMETRY_WINDOW_BW,
	      str, strlen(str));
#endif
}


/***********************************************************************
 *
 *  Procedure:
 *      DisplaySize - display the size in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current fvwm window
 *      width   - the width of the rubber band
 *      height  - the height of the rubber band
 *
 ***********************************************************************/
static void DisplaySize(
  FvwmWindow *tmp_win, XEvent *eventp, int width, int height, Bool Init,
  Bool resetLast)
{
  char str[100];
  int dwidth,dheight,offset;
  static int last_width = 0;
  static int last_height = 0;

  if (Scr.gs.do_hide_resize_window)
    return;
  position_geometry_window(eventp);
  if (resetLast)
  {
    last_width = 0;
    last_height = 0;
  }
  if (last_width == width && last_height == height)
    return;

  last_width = width;
  last_height = height;

  dheight = height - tmp_win->title_g.height - 2*tmp_win->boundary_width;
  dwidth = width - 2*tmp_win->boundary_width;

  dwidth -= tmp_win->hints.base_width;
  dheight -= tmp_win->hints.base_height;
  dwidth /= tmp_win->hints.width_inc;
  dheight /= tmp_win->hints.height_inc;

  (void) sprintf (str, GEOMETRY_WINDOW_SIZE_STRING, dwidth, dheight);
  if (Init)
  {
    XClearWindow(dpy,Scr.SizeWindow);
  }
  else
  {
    /* just clear indside the relief lines to reduce flicker */
    XClearArea(dpy, Scr.SizeWindow, GEOMETRY_WINDOW_BW, GEOMETRY_WINDOW_BW,
	       Scr.SizeStringWidth, Scr.DefaultFont.height, False);
  }

  if (Pdepth >= 2)
  {
    RelieveRectangle(
      dpy, Scr.SizeWindow, 0, 0,
      Scr.SizeStringWidth + GEOMETRY_WINDOW_BW * 2 - 1,
      Scr.DefaultFont.height + GEOMETRY_WINDOW_BW*2 - 1,
      Scr.StdReliefGC, Scr.StdShadowGC, GEOMETRY_WINDOW_BW);
  }
  offset = (Scr.SizeStringWidth -
	    XTextWidth(Scr.DefaultFont.font, str, strlen(str))) / 2;
  offset += GEOMETRY_WINDOW_BW;
#ifdef I18N_MB
  XmbDrawString(dpy, Scr.SizeWindow, Scr.DefaultFont.fontset, Scr.StdGC,
		offset, Scr.DefaultFont.font->ascent + GEOMETRY_WINDOW_BW,
		str, strlen(str));
#else
  XDrawString(dpy, Scr.SizeWindow, Scr.StdGC,
	      offset, Scr.DefaultFont.font->ascent + GEOMETRY_WINDOW_BW,
	      str, strlen(str));
#endif
}


void CMD_ResizeMove(F_CMD_ARGS)
{
  int FinalX = 0;
  int FinalY = 0;
  int FinalW = 0;
  int FinalH = 0;
  int n;
  int x,y;
  Bool fWarp = False;
  Bool fPointer = False;
  Bool has_focus;
  int dx;
  int dy;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_RESIZE, ButtonPress))
    return;
  if (tmp_win == NULL)
    return;
  if (IS_FIXED(tmp_win))
    return;
  if (check_if_function_allowed(F_RESIZE,tmp_win,True,NULL) == 0)
  {
    XBell(dpy, 0);
    return;
  }

  /* gotta have a window */
  w = tmp_win->frame;
  if (!XGetGeometry(dpy, w, &JunkRoot, &x, &y, (unsigned int *)&FinalW,
		    (unsigned int *)&FinalH, &JunkBW, &JunkDepth))
  {
    XBell(dpy, 0);
    return;
  }

  FinalX = x;
  FinalY = y;

  n = GetResizeMoveArguments(
    &action,
    tmp_win->hints.base_width, tmp_win->hints.base_height,
    tmp_win->hints.width_inc, tmp_win->hints.height_inc,
    tmp_win->boundary_width, tmp_win->title_g.height,
    &FinalX, &FinalY, &FinalW, &FinalH, &fWarp, &fPointer);
  if (n < 4)
    return;

  if (IS_MAXIMIZED(tmp_win))
  {
    /* must redraw the buttons now so that the 'maximize' button does not stay
     * depressed. */
    SET_MAXIMIZED(tmp_win, 0);
    DrawDecorations(
      tmp_win, DRAW_BUTTONS, (tmp_win == Scr.Hilite), True, None);
  }
  dx = FinalX - tmp_win->frame_g.x;
  dy = FinalY - tmp_win->frame_g.y;
  /* size will be less or equal to requested */
  constrain_size(tmp_win, (unsigned int *)&FinalW, (unsigned int *)&FinalH,
		 0, 0, 0);
  if (IS_SHADED(tmp_win))
  {
    SetupFrame(tmp_win, FinalX, FinalY, FinalW, tmp_win->frame_g.height, False);
  }
  else
  {
    SetupFrame(tmp_win, FinalX, FinalY, FinalW, FinalH, True);
  }
  if (fWarp)
    XWarpPointer(dpy, None, None, 0, 0, 0, 0, FinalX - x, FinalY - y);
  if (IS_MAXIMIZED(tmp_win))
  {
    tmp_win->max_g.x += dx;
    tmp_win->max_g.y += dy;
  }
  else
  {
    tmp_win->normal_g.x += dx;
    tmp_win->normal_g.y += dy;
  }
  has_focus = (tmp_win == get_focus_window())? True : False;
  DrawDecorations(tmp_win, DRAW_ALL, has_focus, True, None);
  update_absolute_geometry(tmp_win);
  maximize_adjust_offset(tmp_win);
  XSync(dpy, 0);
  GNOME_SetWinArea(tmp_win);

  return;
}


static void InteractiveMove(
  Window *win, FvwmWindow *tmp_win, int *FinalX, int *FinalY, XEvent *eventp,
  Bool do_start_at_pointer)
{
  int origDragX,origDragY,DragX, DragY, DragWidth, DragHeight;
  int XOffset, YOffset;
  Window w;
  Bool do_move_opaque = False;

  w = *win;

  if (Scr.bo.InstallRootCmap)
    InstallRootColormap();
  else
    InstallFvwmColormap();
  /* warp the pointer to the cursor position from before menu appeared */
  /* domivogt (17-May-1999): an XFlush should not hurt anyway, so do it
   * unconditionally to remove the external */
  XFlush(dpy);

  if (do_start_at_pointer)
  {
    if (XQueryPointer(
      dpy, Scr.Root, &JunkRoot, &JunkChild, &DragX, &DragY, &JunkX, &JunkY,
      &JunkMask) == False)
    {
      /* pointer is on a different screen */
      DragX = 0;
      DragY = 0;
    }
  }
  else
  {
    /* Although a move is usually done with a button depressed we have to check
     * for ButtonRelease too since the event may be faked. */
    GetLocationFromEventOrQuery(dpy, Scr.Root, &Event, &DragX, &DragY);
  }

  MyXGrabServer(dpy);
  if (!XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
		    (unsigned int *)&DragWidth, (unsigned int *)&DragHeight,
		    &JunkBW,  &JunkDepth))
  {
    MyXUngrabServer(dpy);
    return;
  }
  MyXGrabKeyboard(dpy);
  if (do_start_at_pointer)
  {
    origDragX = DragX;
    origDragY = DragY;
  }

  if (IS_ICONIFIED(tmp_win))
    do_move_opaque = True;
  else if (IS_MAPPED(tmp_win))
  {
    float areapct;

    areapct = 100.0;
    areapct *= ((float)DragWidth / (float)Scr.MyDisplayWidth);
    areapct *= ((float)DragHeight / (float)Scr.MyDisplayHeight);
    /* round up */
    areapct += 0.1;
    if (Scr.OpaqueSize < 0 || (float)areapct <= (float)Scr.OpaqueSize)
    {
      do_move_opaque = True;
    }
  }
  if (do_move_opaque)
  {
    MyXUngrabServer(dpy);
  }
  else
  {
    Scr.flags.is_wire_frame_displayed = True;
  }

  if (!do_move_opaque && IS_ICONIFIED(tmp_win))
    XUnmapWindow(dpy,w);

  XOffset = origDragX - DragX;
  YOffset = origDragY - DragY;
  if (!Scr.gs.do_hide_position_window)
  {
    position_geometry_window(NULL);
    XMapRaised(dpy,Scr.SizeWindow);
  }
  moveLoop(tmp_win, XOffset,YOffset,DragWidth,DragHeight, FinalX,FinalY,
	   do_move_opaque);
  if (!Scr.gs.do_hide_position_window)
  {
    XUnmapWindow(dpy,Scr.SizeWindow);
  }
  if (Scr.bo.InstallRootCmap)
    UninstallRootColormap();
  else
    UninstallFvwmColormap();

  if (!do_move_opaque)
  {
    /* Throw away some events that dont interest us right now. */
    discard_events(EnterWindowMask|LeaveWindowMask);
    Scr.flags.is_wire_frame_displayed = False;
    MyXUngrabServer(dpy);
  }
  MyXUngrabKeyboard(dpy);

  return;
}


/* Perform the movement of the window. ppctMovement *must* have a 1.0 entry
 * somewhere in ins list of floats, and movement will stop when it hits a 1.0
 * entry */
static void AnimatedMoveAnyWindow(
  FvwmWindow *tmp_win, Window w, int startX, int startY, int endX, int endY,
  Bool fWarpPointerToo, int cmsDelay, float *ppctMovement)
{
  int pointerX, pointerY;
  int currentX, currentY;
  int lastX, lastY;
  int deltaX, deltaY;

  if (tmp_win && IS_FIXED(tmp_win))
    return;

  /* set our defaults */
  if (ppctMovement == NULL)
    ppctMovement = rgpctMovementDefault;
  if (cmsDelay < 0)
    cmsDelay = cmsDelayDefault;

  if (startX < 0 || startY < 0)
  {
    if (!XGetGeometry(dpy, w, &JunkRoot, &currentX, &currentY,
		      &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth))
    {
      XBell(dpy, 0);
      return;
    }
    if (startX < 0)
      startX = currentX;
    if (startY < 0)
      startY = currentY;
  }

  deltaX = endX - startX;
  deltaY = endY - startY;
  lastX = startX;
  lastY = startY;

  if (deltaX == 0 && deltaY == 0)
    /* go nowhere fast */
    return;

  /* Needed for aborting */
  MyXGrabKeyboard(dpy);
  do
  {
    currentX = startX + deltaX * (*ppctMovement);
    currentY = startY + deltaY * (*ppctMovement);
    if (lastX == currentX && lastY == currentY)
      /* don't waste time in the same spot */
      continue;
    XMoveWindow(dpy,w,currentX,currentY);
    if (fWarpPointerToo == True)
    {
      if (XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
			&JunkX,&JunkY,&pointerX,&pointerY,&JunkMask) == False)
      {
	/* pointer is on a different screen */
	pointerX = currentX;
	pointerY = currentY;
      }
      else
      {
	pointerX += currentX - lastX;
	pointerY += currentY - lastY;
      }
      XWarpPointer(dpy,None,Scr.Root,0,0,0,0, pointerX,pointerY);
    }
    if (tmp_win && !IS_SHADED(tmp_win) && !Scr.bo.DisableConfigureNotify)
    {
      /* send configure notify event for windows that care about their
       * location */
      SendConfigureNotify(
	tmp_win, currentX, currentY, tmp_win->frame_g.width,
	tmp_win->frame_g.height, 0, False);
#ifdef FVWM_DEBUG_MSGS
      fvwm_msg(DBG,"AnimatedMoveAnyWindow",
	       "Sent ConfigureNotify (w == %d, h == %d)",
               tmp_win->frame_g.width, tmp_win->frame_g.height);
#endif
    }
    XFlush(dpy);
    if (tmp_win)
    {
      tmp_win->frame_g.x = currentX;
      tmp_win->frame_g.y = currentY;
      update_absolute_geometry(tmp_win);
      maximize_adjust_offset(tmp_win);
      BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
      FlushAllMessageQueues();
    }

    usleep(cmsDelay * 1000); /* usleep takes microseconds */
    /* this didn't work for me -- maybe no longer necessary since
     * we warn the user when they use > .5 seconds as a between-frame delay
     * time.
     *
     * domivogt (28-apr-1999): That is because the keyboard was not grabbed.
     * works nicely now.
     */
    if (XCheckMaskEvent(dpy, ButtonPressMask|ButtonReleaseMask|KeyPressMask,
			&Event))
    {
      StashEventTime(&Event);
      /* finish the move immediately */
      XMoveWindow(dpy,w,endX,endY);
      break;
    }
    lastX = currentX;
    lastY = currentY;
  }
  while
    (*ppctMovement != 1.0 && ppctMovement++);
  MyXUngrabKeyboard(dpy);
  XFlush(dpy);
  if (tmp_win)
    GNOME_SetWinArea(tmp_win);
  return;
}

/* used for moving menus, not a client window */
void AnimatedMoveOfWindow(Window w, int startX, int startY,
                             int endX, int endY, Bool fWarpPointerToo,
                             int cmsDelay, float *ppctMovement)
{
  AnimatedMoveAnyWindow(NULL, w, startX, startY, endX, endY, fWarpPointerToo,
                        cmsDelay, ppctMovement);
}

/* used for moving client windows */
void AnimatedMoveFvwmWindow(FvwmWindow *tmp_win, Window w, int startX,
			    int startY, int endX, int endY,
			    Bool fWarpPointerToo, int cmsDelay,
			    float *ppctMovement)
{
  AnimatedMoveAnyWindow(tmp_win, w, startX, startY, endX, endY,
			fWarpPointerToo, cmsDelay, ppctMovement);
}


/****************************************************************************
 *
 * Start a window move operation
 *
 ****************************************************************************/
static void move_window_doit(F_CMD_ARGS, Bool do_animate, int mode)
{
  int FinalX = 0;
  int FinalY = 0;
  int n;
  int x,y;
  unsigned int width, height;
  int page_x, page_y;
  Bool fWarp = False;
  Bool fPointer = False;
  int dx;
  int dy;

  if (DeferExecution(
	eventp,&w,&tmp_win,&context,
	(mode == MOVE_NORMAL) ? CRS_MOVE : CRS_SELECT, ButtonPress))
  {
    return;
  }

  if (tmp_win == NULL)
    return;

  if (IS_FIXED(tmp_win))
    return;
  if (check_if_function_allowed(F_MOVE, tmp_win, True, NULL) == 0)
  {
    return;
  }


  /* gotta have a window */
  w = tmp_win->frame;
  if(IS_ICONIFIED(tmp_win))
  {
    if(tmp_win->icon_pixmap_w != None)
    {
      w = tmp_win->icon_pixmap_w;
      XUnmapWindow(dpy,tmp_win->icon_w);
    }
    else
    {
      w = tmp_win->icon_w;
    }
  }
  if (!XGetGeometry(dpy, w, &JunkRoot, &x, &y, &width, &height,
		    &JunkBW, &JunkDepth))
  {
    return;
  }

  if (mode == MOVE_PAGE)
  {
    rectangle r;
    rectangle s;

    do_animate = False;
    SET_STICKY(tmp_win, 0);

    if (!get_page_arguments(action, &page_x, &page_y))
    {
      page_x = Scr.Vx;
      page_y = Scr.Vy;
    }
    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;
    s.x = page_x - Scr.Vx;
    s.y = page_y - Scr.Vy;
    s.width = Scr.MyDisplayWidth;
    s.height = Scr.MyDisplayHeight;
    move_into_rectangle(&r, &s);
    FinalX = r.x;
    FinalY = r.y;
  }
  else if (mode == MOVE_SCREEN)
  {
    rectangle r;
    rectangle s;
    rectangle p;
    int fscreen;

    do_animate = False;
    SET_STICKY(tmp_win, 0);

    fscreen = FScreenGetScreenArgument(action, FSCREEN_SPEC_CURRENT);
    FScreenGetScrRect(NULL, fscreen, &s.x, &s.y, &s.width, &s.height);
    page_x = Scr.Vx;
    page_y = Scr.Vy;
    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;
    p.x = page_x - Scr.Vx;
    p.y = page_y - Scr.Vy;
    p.width = Scr.MyDisplayWidth;
    p.height = Scr.MyDisplayHeight;
    /* move to page first */
    move_into_rectangle(&r, &p);
    /* then move to screen */
    move_into_rectangle(&r, &s);
    FinalX = r.x;
    FinalY = r.y;
  }
  else
  {
    FinalX = x;
    FinalY = y;
    n = GetMoveArguments(
      &action, width, height, &FinalX, &FinalY, &fWarp, &fPointer);

    if (n != 2 || fPointer)
      InteractiveMove(&w, tmp_win, &FinalX, &FinalY, eventp, fPointer);
    else if (IS_ICONIFIED(tmp_win))
    {
      SET_ICON_MOVED(tmp_win, 1);
    }
  }

  if (w == tmp_win->frame)
  {
    dx = FinalX - tmp_win->frame_g.x;
    dy = FinalY - tmp_win->frame_g.y;
    if (do_animate)
    {
      AnimatedMoveFvwmWindow(tmp_win,w,-1,-1,FinalX,FinalY,fWarp,-1,NULL);
    }
    SetupFrame(tmp_win, FinalX, FinalY,
	       tmp_win->frame_g.width, tmp_win->frame_g.height, True);
    if (fWarp & !do_animate)
      XWarpPointer(dpy, None, None, 0, 0, 0, 0, FinalX - x, FinalY - y);
    if (IS_MAXIMIZED(tmp_win))
    {
      tmp_win->max_g.x += dx;
      tmp_win->max_g.y += dy;
    }
    else
    {
      tmp_win->normal_g.x += dx;
      tmp_win->normal_g.y += dy;
    }
    update_absolute_geometry(tmp_win);
    maximize_adjust_offset(tmp_win);
    XSync(dpy, 0);
    GNOME_SetWinArea(tmp_win);
  }
  else /* icon window */
  {
    tmp_win->icon_g.x = FinalX;
    tmp_win->icon_xl_loc = FinalX -
      (tmp_win->icon_g.width - tmp_win->icon_p_width)/2;
    tmp_win->icon_g.y = FinalY;
    BroadcastPacket(M_ICON_LOCATION, 7,
		    tmp_win->w, tmp_win->frame,
		    (unsigned long)tmp_win,
		    tmp_win->icon_g.x, tmp_win->icon_g.y,
		    tmp_win->icon_p_width,
		    tmp_win->icon_g.height + tmp_win->icon_p_height);
    if (do_animate)
    {
      AnimatedMoveOfWindow(tmp_win->icon_w,-1,-1,tmp_win->icon_xl_loc,
			   FinalY+tmp_win->icon_p_height, fWarp,-1,NULL);
    }
    else
    {
      XMoveWindow(dpy,tmp_win->icon_w, tmp_win->icon_xl_loc,
		  FinalY+tmp_win->icon_p_height);
      if (fWarp)
	XWarpPointer(dpy, None, None, 0, 0, 0, 0, FinalX - x, FinalY - y);
    }
    if(tmp_win->icon_pixmap_w != None)
    {
      XMapWindow(dpy,tmp_win->icon_w);
      if (do_animate)
      {
	AnimatedMoveOfWindow(tmp_win->icon_pixmap_w, -1,-1,
			     tmp_win->icon_g.x,FinalY,fWarp,-1,NULL);
      }
      else
      {
	XMoveWindow(dpy, tmp_win->icon_pixmap_w, tmp_win->icon_g.x, FinalY);
	if (fWarp)
	  XWarpPointer(dpy, None, None, 0, 0, 0, 0, FinalX - x, FinalY - y);
      }
      XMapWindow(dpy,w);
    }
    XSync(dpy, 0);
  }
  return;
}

void CMD_Move(F_CMD_ARGS)
{
  move_window_doit(F_PASS_ARGS, False, MOVE_NORMAL);
}

void CMD_AnimatedMove(F_CMD_ARGS)
{
  move_window_doit(F_PASS_ARGS, True, MOVE_NORMAL);
}

void CMD_MoveToPage(F_CMD_ARGS)
{
  move_window_doit(F_PASS_ARGS, False, MOVE_PAGE);
}

void CMD_MoveToScreen(F_CMD_ARGS)
{
  move_window_doit(F_PASS_ARGS, False, MOVE_SCREEN);
}

/* This function does the SnapAttraction stuff. If takes x and y coordinates
 * (*px and *py) and returns the snapped values. */
static void DoSnapAttract(
  FvwmWindow *tmp_win, unsigned int Width, unsigned int Height,
  int *px, int *py)
{
  int nyt,nxl,dist,closestLeft,closestRight,closestBottom,closestTop;
  rectangle self, other;
  FvwmWindow *tmp;

  /* resist based on window edges */
  closestTop = Scr.SnapAttraction;
  closestBottom = Scr.SnapAttraction;
  closestRight = Scr.SnapAttraction;
  closestLeft = Scr.SnapAttraction;
  nxl = -99999;
  nyt = -99999;
  self.x = *px;
  self.y = *py;
  self.width = Width;
  self.height = Height;
  if (IS_ICONIFIED(tmp_win) && !HAS_NO_ICON_TITLE(tmp_win))
  {
    self.height += tmp_win->icon_g.height;
  }

  /*
   * snap attraction
   */
  /* snap to other windows */
  if ((Scr.SnapMode & (SNAP_ICONS | SNAP_WINDOWS | SNAP_SAME)) &&
      Scr.SnapAttraction > 0)
  {
    for (tmp = Scr.FvwmRoot.next; tmp; tmp = tmp->next)
    {
      if (tmp_win->Desk != tmp->Desk || tmp_win == tmp)
      {
	continue;
      }
      /* check snapping type */
      switch (Scr.SnapMode)
      {
      case 1:  /* SameType */
	if (IS_ICONIFIED(tmp) != IS_ICONIFIED(tmp_win))
	{
	  continue;
	}
	break;
      case 2:  /* Icons */
	if (!IS_ICONIFIED(tmp) || !IS_ICONIFIED(tmp_win))
	{
	  continue;
	}
	break;
      case 3:  /* Windows */
	if (IS_ICONIFIED(tmp) || IS_ICONIFIED(tmp_win))
	{
	  continue;
	}
	break;
      case 0:  /* All */
      default:
	/* NOOP */
	break;
      }
      /* get other window dimensions */
      if (IS_ICONIFIED(tmp))
      {
	if (tmp->icon_p_height > 0)
	{
	  other.width = tmp->icon_p_width;
	  other.height = tmp->icon_p_height;
	}
	else
	{
	  other.width = tmp->icon_g.width;
	  other.height = tmp->icon_g.height;
	}
	other.x = tmp->icon_g.x;
	other.y = tmp->icon_g.y;
	if (!HAS_NO_ICON_TITLE(tmp))
	{
	  other.height += tmp->icon_g.height;
	}
      }
      else
      {
	other.width = tmp->frame_g.width;
	other.height = tmp->frame_g.height;
	other.x = tmp->frame_g.x;
	other.y = tmp->frame_g.y;
      }
      /* prevent that window snaps off screen */
      if (other.x <= 0)
      {
	other.x -= Scr.SnapAttraction + 10000;
	other.width += Scr.SnapAttraction + 10000;
      }
      if (other.y <= 0)
      {
	other.y -= Scr.SnapAttraction + 10000;
	other.height += Scr.SnapAttraction + 10000;
      }
      if (other .x + other.width >= Scr.MyDisplayWidth)
      {
	other.width += Scr.SnapAttraction + 10000;
      }
      if (other .y + other.height >= Scr.MyDisplayHeight)
      {
	other.height += Scr.SnapAttraction + 10000;
      }

      /* snap horizontally */
      if (!((other.y + (int)other.height) < (*py) ||
	    (other.y) > (*py + (int)self.height) ))
      {
	dist = abs(other.x - (*px + (int)self.width));
	if (dist < closestRight)
	{
	  closestRight = dist;
	  if (((*px + (int)self.width) >= other.x) &&
	      ((*px + (int)self.width) < other.x + Scr.SnapAttraction))
	  {
	    nxl = other.x - (int)self.width;
	  }
	  if (((*px + (int)self.width) >= other.x - Scr.SnapAttraction) &&
	      ((*px + (int)self.width) < other.x))
	  {
	    nxl = other.x - (int)self.width;
	  }
	}
	dist = abs(other.x + (int)other.width - *px);
	if (dist < closestLeft)
	{
	  closestLeft = dist;
	  if ((*px <= other.x + (int)other.width) &&
	      (*px > other.x + (int)other.width - Scr.SnapAttraction))
	  {
	    nxl = other.x + (int)other.width;
	  }
	  if ((*px <= other.x + (int)other.width + Scr.SnapAttraction) &&
	      (*px > other.x + (int)other.width))
	  {
	    nxl = other.x + (int)other.width;
	  }
	}
      } /* horizontally */
      /* snap vertically */
      if (!((other.x + (int)other.width) < (*px) ||
	    (other.x) > (*px + (int)self.width)))
      {
	dist = abs(other.y - (*py + (int)self.height));
	if (dist < closestBottom)
	{
	  closestBottom = dist;
	  if (((*py + (int)self.height) >= other.y) &&
	      ((*py + (int)self.height) < other.y + Scr.SnapAttraction))
	  {
	    nyt = other.y - (int)self.height;
	  }
	  if (((*py + (int)self.height) >= other.y - Scr.SnapAttraction) &&
	      ((*py + (int)self.height) < other.y))
	  {
	    nyt = other.y - (int)self.height;
	  }
	}
	dist = abs(other.y + (int)other.height - *py);
	if (dist < closestTop)
	{
	  closestTop = dist;
	  if ((*py <= other.y + (int)other.height) &&
	      (*py > other.y + (int)other.height - Scr.SnapAttraction))
	  {
	    nyt = other.y + (int)other.height;
	  }
	  if ((*py <= other.y + (int)other.height + Scr.SnapAttraction) &&
	      (*py > other.y + (int)other.height))
	  {
	    nyt = other.y + (int)other.height;
	  }
	}
      } /* vertically */
    } /* for */
  } /* snap to other windows */

  /* snap to screen egdes */
  if ((Scr.SnapMode & SNAP_SCREEN) && Scr.SnapAttraction > 0)
  {
    /* horizontally */
    if (!(Scr.MyDisplayWidth < (*px) || (*px + (int)self.width) < 0))
    {
      dist = abs(Scr.MyDisplayHeight - (*py + (int)self.height));
      if (dist < closestBottom)
      {
	closestBottom = dist;
	if (((*py + (int)self.height) >= Scr.MyDisplayHeight) &&
	    ((*py + (int)self.height) < Scr.MyDisplayHeight +
	     Scr.SnapAttraction))
	{
	  nyt = Scr.MyDisplayHeight - (int)self.height;
	}
	if (((*py + (int)self.height) >= Scr.MyDisplayHeight -
	     Scr.SnapAttraction) &&
	    ((*py + (int)self.height) < Scr.MyDisplayHeight))
	{
	  nyt = Scr.MyDisplayHeight - (int)self.height;
	}
      }
      dist = abs(*py);
      if (dist < closestTop)
      {
	closestTop = dist;
	if ((*py <= 0)&&(*py > - Scr.SnapAttraction))
	{
	  nyt = 0;
	}
	if ((*py <=  Scr.SnapAttraction)&&(*py > 0))
	{
	  nyt = 0;
	}
      }
    } /* horizontally */
    /* vertically */
    if (!(Scr.MyDisplayHeight < (*py) || (*py + (int)self.height) < 0))
    {
      dist = abs(Scr.MyDisplayWidth - (*px + (int)self.width));
      if (dist < closestRight)
      {
	closestRight = dist;

	if (((*px + (int)self.width) >= Scr.MyDisplayWidth) &&
	    ((*px + (int)self.width) < Scr.MyDisplayWidth +
	     Scr.SnapAttraction))
	{
	  nxl = Scr.MyDisplayWidth - (int)self.width;
	}

	if (((*px + (int)self.width) >= Scr.MyDisplayWidth -
	     Scr.SnapAttraction) &&
	    ((*px + (int)self.width) < Scr.MyDisplayWidth))
	{
	  nxl = Scr.MyDisplayWidth - (int)self.width;
	}
      }
      dist = abs(*px);
      if (dist < closestLeft)
      {
	closestLeft = dist;

	if ((*px <= 0) &&
	    (*px > - Scr.SnapAttraction))
	{
	  nxl = 0;
	}
	if ((*px <= Scr.SnapAttraction) &&
	    (*px > 0))
	{
	  nxl = 0;
	}
      }
    } /* vertically */
  } /* snap to screen edges */

  if (nxl != -99999)
  {
    *px = nxl;
  }
  if (nyt != -99999)
  {
    *py = nyt;
  }

  /*
   * Snap grid handling
   */
  if (Scr.SnapGridX > 1 && nxl == -99999)
  {
    if (*px != *px / Scr.SnapGridX * Scr.SnapGridX)
    {
      *px = (*px + ((*px >= 0) ? Scr.SnapGridX : -Scr.SnapGridX) / 2) /
	Scr.SnapGridX * Scr.SnapGridX;
    }
  }
  if (Scr.SnapGridY > 1 && nyt == -99999)
  {
    if (*py != *py / Scr.SnapGridY * Scr.SnapGridY)
    {
      *py = (*py + ((*py >= 0) ? Scr.SnapGridY : -Scr.SnapGridY) / 2) /
	Scr.SnapGridY * Scr.SnapGridY;
    }
  }

  /*
   * Resist moving windows beyond the edge of the screen
   */
  if (Scr.MoveResistance > 0)
  {
    /* snap to right edge */
    if (((*px + Width) >= Scr.MyDisplayWidth)
	&& ((*px + Width) < Scr.MyDisplayWidth + Scr.MoveResistance))
    {
      *px = Scr.MyDisplayWidth - Width;
    }
    /* snap to left edge */
    else if ((*px <= 0) && (*px > -Scr.MoveResistance))
    {
      *px = 0;
    }
    /* snap to bottom edge */
    if (((*py + Height) >= Scr.MyDisplayHeight)
	&& ((*py + Height) < Scr.MyDisplayHeight + Scr.MoveResistance))
    {
      *py = Scr.MyDisplayHeight - Height;
    }
    /* snap to top edge */
    else if ((*py <= 0) && (*py > -Scr.MoveResistance))
    {
      *py = 0;
    }
  }
  /* Resist moving windows between xineramascreens */
  if (Scr.XiMoveResistance > 0 && FScreenIsEnabled())
  {
    int scr_x0, scr_y0, scr_x1, scr_y1;
    Bool do_recalc_rectangle = False;

    FScreenGetResistanceRect(
      *px, *py, Width, Height, &scr_x0, &scr_y0, &scr_x1, &scr_y1);

    /* snap to right edge */
    if (scr_x1 != Scr.MyDisplayWidth  &&
	*px + Width >= scr_x1 && *px + Width <  scr_x1 + Scr.XiMoveResistance)
    {
      *px = scr_x1 - Width;
      do_recalc_rectangle = True;
    }
    /* snap to left edge */
    else if (scr_x0 != 0 &&
	     *px <= scr_x0 && scr_x0 - *px < Scr.XiMoveResistance)
    {
      *px = scr_x0;
      do_recalc_rectangle = True;
    }
    if (do_recalc_rectangle)
    {
      /* Snapping in X direction can move the window off a screen.  Thus, it
       * may no longer be necessary to snap in Y direction. */
      FScreenGetResistanceRect(
	*px, *py, Width, Height, &scr_x0, &scr_y0, &scr_x1, &scr_y1);
    }
    /* snap to bottom edge */
    if (scr_y1 != Scr.MyDisplayHeight &&
	*py + Height >= scr_y1 && *py + Height <  scr_y1 + Scr.XiMoveResistance)
    {
      *py = scr_y1 - Height;
    }
    /* snap to top edge */
    else if (scr_y0 != 0 &&
	     *py <= scr_y0 && scr_y0 - *py < Scr.XiMoveResistance)
    {
      *py = scr_y0;
    }
  }

  return;
}

/****************************************************************************
 *
 * Move the rubberband around, return with the new window location
 *
 * Returns True if the window has to be resized after the move.
 *
 ****************************************************************************/
Bool moveLoop(FvwmWindow *tmp_win, int XOffset, int YOffset, int Width,
	      int Height, int *FinalX, int *FinalY,Bool do_move_opaque)
{
  extern Window bad_window;
  Bool finished = False;
  Bool aborted = False;
  int xl,xl2,yt,yt2,delta_x,delta_y,paged;
  unsigned int button_mask = 0;
  FvwmWindow tmp_win_copy;
  int dx = Scr.EdgeScrollX ? Scr.EdgeScrollX : Scr.MyDisplayWidth;
  int dy = Scr.EdgeScrollY ? Scr.EdgeScrollY : Scr.MyDisplayHeight;
  int vx = Scr.Vx;
  int vy = Scr.Vy;
  int xl_orig = 0;
  int yt_orig = 0;
  int cnx = 0;
  int cny = 0;
  int x_virtual_offset = 0;
  int y_virtual_offset = 0;
  Bool sent_cn = False;
  Bool do_resize_too = False;
  Bool do_exec_placement_func = False;
  int x_bak;
  int y_bak;
  Window move_w = None;
  int orig_icon_x = 0;
  int orig_icon_y = 0;

  if (!GrabEm(CRS_MOVE, GRAB_NORMAL))
  {
    XBell(dpy, 0);
    return False;
  }

  if (!IS_MAPPED(tmp_win) && !IS_ICONIFIED(tmp_win))
    do_move_opaque = False;

  bad_window = None;
  if (IS_ICONIFIED(tmp_win))
  {
    if (tmp_win->icon_pixmap_w != None)
      move_w = tmp_win->icon_pixmap_w;
    else if (tmp_win->icon_w != None)
      move_w = tmp_win->icon_w;
  }
  else
  {
    move_w = tmp_win->frame;
  }
  if (!XGetGeometry(dpy, move_w, &JunkRoot, &x_bak, &y_bak,
		    &JunkWidth, &JunkHeight, &JunkBW,&JunkDepth))
  {
    /* This is allright here since the window may not be mapped yet. */
  }

  if (IS_ICONIFIED(tmp_win))
  {
    orig_icon_x = tmp_win->icon_g.x;
    orig_icon_y = tmp_win->icon_g.y;
  }

  /* make a copy of the tmp_win structure for sending to the pager */
  memcpy(&tmp_win_copy, tmp_win, sizeof(FvwmWindow));
  /* prevent flicker when paging */
  SET_WINDOW_BEING_MOVED_OPAQUE(tmp_win, do_move_opaque);

  if (XQueryPointer(
	dpy, Scr.Root, &JunkRoot, &JunkChild, &xl, &yt, &JunkX, &JunkY,
	&button_mask) == False)
  {
    /* pointer is on a different screen */
    xl = 0;
    yt = 0;
  }
  else
  {
    xl += XOffset;
    yt += YOffset;
  }
  button_mask &= DEFAULT_ALL_BUTTONS_MASK;
  xl_orig = xl;
  yt_orig = yt;

  /* draw initial outline */
  if (!IS_ICONIFIED(tmp_win) &&
      ((!do_move_opaque && !Scr.gs.EmulateMWM) || !IS_MAPPED(tmp_win)))
    draw_move_resize_grid(xl, yt, Width - 1, Height - 1);

  DisplayPosition(tmp_win, &Event, xl, yt, True);

  while (!finished && bad_window != tmp_win->w)
  {
    int rc = 0;

    /* wait until there is an interesting event */
    while (rc != -1 && (!XPending(dpy) ||
	   !XCheckMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
			    KeyPressMask | PointerMotionMask |
			    ButtonMotionMask | ExposureMask, &Event)))
    {
      rc = HandlePaging(
	      dx, dy, &xl, &yt, &delta_x, &delta_y, False, False, True);
      if (rc == 1)
      {
	/* Fake an event to force window reposition */
	if (delta_x)
	{
	  x_virtual_offset = 0;
	}
	if (XOffset)
	{
	  xl += XOffset;
	}
	if (delta_y)
	{
	  y_virtual_offset = 0;
	}
	if (YOffset)
	{
	  yt += YOffset;
	}
	DoSnapAttract(tmp_win, Width, Height, &xl, &yt);
	Event.type = MotionNotify;
	Event.xmotion.time = lastTimestamp;
	Event.xmotion.x_root = xl - XOffset;
	Event.xmotion.y_root = yt - YOffset;
	break;
      }
    }
    if (rc == -1)
    {
      /* block until an event arrives */
      XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
		 KeyPressMask | PointerMotionMask |
		 ButtonMotionMask | ExposureMask, &Event);
    }
    StashEventTime(&Event);

    /* discard any extra motion events before a logical release */
    if (Event.type == MotionNotify)
    {
      XEvent new_event;

      /*** logic borrowed from icewm ***/
      while (XPending(dpy) > 0 &&
	     XCheckMaskEvent(dpy, ButtonMotionMask | PointerMotionMask |
			     ButtonPressMask | ButtonRelease | KeyPressMask,
			     &new_event))
      {
	if (Event.type != new_event.type)
	{
	  XPutBackEvent(dpy, &new_event);
	  break;
	}
	else
	{
	  Event = new_event;
	}
      }
      /*** end of code borrowed from icewm ***/
      StashEventTime(&Event);

    } /* if (Event.type == MotionNotify) */

    /* Handle a limited number of key press events to allow mouseless
     * operation */
    if (Event.type == KeyPress)
    {
      Keyboard_shortcuts(
	&Event, tmp_win, &x_virtual_offset, &y_virtual_offset, ButtonRelease);
    }
    switch(Event.type)
    {
    case KeyPress:
      /* simple code to bag out of move - CKH */
      if (XLookupKeysym(&(Event.xkey),0) == XK_Escape)
      {
	if (!do_move_opaque)
	  switch_move_resize_grid(False);
	if (!IS_ICONIFIED(tmp_win))
	{
	  if (do_move_opaque)
	  {
	    *FinalX = tmp_win->frame_g.x;
	    *FinalY = tmp_win->frame_g.y;
	  }
	}
	else
	{
	  *FinalX = orig_icon_x;
	  *FinalY = orig_icon_y;
	}
	aborted = True;
	finished = True;
      }
      break;
    case ButtonPress:
      if (Event.xbutton.button <= NUMBER_OF_MOUSE_BUTTONS &&
	  ((Button1Mask << (Event.xbutton.button - 1)) & button_mask))
      {
	/* No new button was pressed, just a delayed event */
	break;
      }
      if(((Event.xbutton.button == 2)&&(!Scr.gs.EmulateMWM))||
	 ((Event.xbutton.button == 1)&&(Scr.gs.EmulateMWM)&&
	  (Event.xbutton.state & ShiftMask)))
      {
	do_resize_too = True;
	do_exec_placement_func = False;
	SET_PLACED_WB3(tmp_win,False);
	/* Fallthrough to button-release */
      }
      else if(Event.xbutton.button == 3)
      {
	do_exec_placement_func = True;
	do_resize_too = False;
	SET_PLACED_WB3(tmp_win,True);
	/* Fallthrough to button-release */
      }
      else
      {
	/* Abort the move if
	 *  - the move started with a pressed button and another button
	 *    was pressed during the operation
	 *  - no button was pressed at the beginning and any button
	 *    except button 1 was pressed. */
	SET_PLACED_WB3(tmp_win,False);
	if (button_mask || (Event.xbutton.button != 1))
	{
	  if(!do_move_opaque)
	    switch_move_resize_grid(False);
	  if (!IS_ICONIFIED(tmp_win))
	  {
	    *FinalX = tmp_win->frame_g.x;
	    *FinalY = tmp_win->frame_g.y;
	  }
	  else
	  {
	    *FinalX = orig_icon_x;
	    *FinalY = orig_icon_y;
	  }
	  aborted = True;
	  finished = True;
	}
	break;
      }
    case ButtonRelease:
      if(!do_move_opaque)
	switch_move_resize_grid(False);
      xl2 = Event.xbutton.x_root + XOffset + x_virtual_offset;
      yt2 = Event.xbutton.y_root + YOffset + y_virtual_offset;
      /* ignore the position of the button release if it was on a
       * different page. */
      if (!(((xl <  0 && xl2 >= 0) || (xl >= 0 && xl2 <  0) ||
	     (yt <  0 && yt2 >= 0) || (yt >= 0 && yt2 <  0)) &&
	    (abs(xl - xl2) > Scr.MyDisplayWidth / 2 ||
	     abs(yt - yt2) > Scr.MyDisplayHeight / 2)))
      {
	xl = xl2;
	yt = yt2;
      }
      if (xl != xl_orig || yt != yt_orig || vx != Scr.Vx || vy != Scr.Vy)
      {
	/* only snap if the window actually moved! */
        DoSnapAttract(tmp_win, Width, Height, &xl, &yt);
      }

      *FinalX = xl;
      *FinalY = yt;

      finished = True;
      break;

    case MotionNotify:
      if (Event.xmotion.same_screen == False)
      {
	continue;
      }
      xl = Event.xmotion.x_root;
      yt = Event.xmotion.y_root;
      if (xl > 0 && xl < Scr.MyDisplayWidth - 1)
      {
	/* pointer was moved away from the left/right border with the mouse,
	 * reset the virtual x offset */
	x_virtual_offset = 0;
      }
      if (yt > 0 && yt < Scr.MyDisplayHeight - 1)
      {
	/* pointer was moved away from the top/bottom border with the mouse,
	 * reset the virtual y offset */
	y_virtual_offset = 0;
      }
      xl += XOffset + x_virtual_offset;
      yt += YOffset + y_virtual_offset;

      DoSnapAttract(tmp_win, Width, Height, &xl, &yt);

      /* check Paging request once and only once after outline redrawn */
      /* redraw after paging if needed - mab */
      paged = 0;
      while (paged <= 1)
      {
	if(!do_move_opaque)
	  draw_move_resize_grid(xl, yt, Width - 1, Height - 1);
	else
	{
	  if (IS_ICONIFIED(tmp_win))
	  {
	    tmp_win->icon_g.x = xl ;
	    tmp_win->icon_xl_loc = xl -
	      (tmp_win->icon_g.width - tmp_win->icon_p_width)/2;
	    tmp_win->icon_g.y = yt;
	    if(tmp_win->icon_pixmap_w != None)
	      XMoveWindow(dpy, tmp_win->icon_pixmap_w, tmp_win->icon_g.x,yt);
	    else if (tmp_win->icon_w != None)
	      XMoveWindow(dpy, tmp_win->icon_w,tmp_win->icon_xl_loc,
			  yt+tmp_win->icon_p_height);
	  }
	  else
	  {
	    XMoveWindow(dpy,tmp_win->frame,xl,yt);
	  }
	}
	DisplayPosition(tmp_win, &Event, xl, yt, False);

	/* prevent window from lagging behind mouse when paging - mab */
	if (paged == 0)
	{
	  xl = Event.xmotion.x_root;
	  yt = Event.xmotion.y_root;
	  HandlePaging(
	    dx, dy, &xl, &yt, &delta_x, &delta_y, False, False, False);
	  if (delta_x)
	  {
	    x_virtual_offset = 0;
	  }
	  if (XOffset)
	  {
	    xl += XOffset;
	  }
	  if (delta_y)
	  {
	    y_virtual_offset = 0;
	  }
	  if (YOffset)
	  {
	    yt += YOffset;
	  }
	  DoSnapAttract(tmp_win, Width, Height, &xl, &yt);
	  if (!delta_x && !delta_y)
	    /* break from while (paged <= 1) */
	    break;
	}
	paged++;
      }  /* end while (paged) */

      break;

    case Expose:
      if (!do_move_opaque)
      {
	/* must undraw the rubber band in case the event causes some drawing */
	switch_move_resize_grid(False);
      }
      DispatchEvent(False);
      if (!do_move_opaque)
	draw_move_resize_grid(xl, yt, Width - 1, Height - 1);
      break;

    default:
      /* cannot happen */
      break;
    } /* switch */
    xl += x_virtual_offset;
    yt += y_virtual_offset;
    if (do_move_opaque && !IS_ICONIFIED(tmp_win) && !IS_SHADED(tmp_win) &&
	!Scr.bo.DisableConfigureNotify)
    {
      /* send configure notify event for windows that care about their
       * location; don't send anything if position didn't change */
      if (!sent_cn || cnx != xl || cny != yt)
      {
        cnx = xl;
        cny = yt;
        sent_cn = True;
	SendConfigureNotify(tmp_win, xl, yt, Width, Height, 0, False);
#ifdef FVWM_DEBUG_MSGS
        fvwm_msg(
	  DBG, "SetupFrame","Sent ConfigureNotify (w %d, h %d)", Width, Height);
#endif
      }
    }
    if (do_move_opaque)
    {
      if (!IS_ICONIFIED(tmp_win))
      {
	tmp_win_copy.frame_g.x = xl;
	tmp_win_copy.frame_g.y = yt;
      }
      /* only do this with opaque moves, (i.e. the server is not grabbed) */
      BroadcastConfig(M_CONFIGURE_WINDOW, &tmp_win_copy);
      FlushAllMessageQueues();
    }
  } /* while (!finished) */

  if (!Scr.gs.do_hide_position_window)
    XUnmapWindow(dpy,Scr.SizeWindow);
  if (aborted || bad_window == tmp_win->w)
  {
    if (vx != Scr.Vx || vy != Scr.Vy)
    {
      MoveViewport(vx, vy, False);
    }
    if (aborted && do_move_opaque)
    {
      XMoveWindow(dpy, move_w, x_bak, y_bak);
    }
    if (bad_window == tmp_win->w)
    {
      XUnmapWindow(dpy, move_w);
      XBell(dpy, 0);
    }
  }
  if (!aborted && bad_window != tmp_win->w && IS_ICONIFIED(tmp_win))
  {
    SET_ICON_MOVED(tmp_win, 1);
  }
  UngrabEm(GRAB_NORMAL);
  if (!do_resize_too)
  {
    /* Don't wait for buttons to come up when user is placing a new window
     * and wants to resize it. */
    WaitForButtonsUp(True);
  }
  SET_WINDOW_BEING_MOVED_OPAQUE(tmp_win, 0);
  bad_window = None;

  return do_resize_too;
}

void CMD_MoveThreshold(F_CMD_ARGS)
{
  int val = 0;

  if (GetIntegerArguments(action, NULL, &val, 1) < 1 || val < 0)
    Scr.MoveThreshold = DEFAULT_MOVE_THRESHOLD;
  else
    Scr.MoveThreshold = val;
}


void CMD_OpaqueMoveSize(F_CMD_ARGS)
{
  int val;

  if (GetIntegerArguments(action, NULL, &val, 1) < 1)
  {
    if (strncasecmp(action, "unlimited", 9) == 0)
      Scr.OpaqueSize = -1;
    else
      Scr.OpaqueSize = DEFAULT_OPAQUE_MOVE_SIZE;
  }
  else
    Scr.OpaqueSize = val;
}


static char *hide_options[] =
{
  "never",
  "move",
  "resize",
  NULL
};

void CMD_HideGeometryWindow(F_CMD_ARGS)
{
  char *token = PeekToken(action, NULL);

  Scr.gs.do_hide_position_window = 0;
  Scr.gs.do_hide_resize_window = 0;
  switch(GetTokenIndex(token, hide_options, 0, NULL))
  {
  case 0:
    break;
  case 1:
    Scr.gs.do_hide_position_window = 1;
    break;
  case 2:
    Scr.gs.do_hide_resize_window = 1;
    break;
  default:
    Scr.gs.do_hide_position_window = 1;
    Scr.gs.do_hide_resize_window = 1;
    break;
  }
  return;
}


void CMD_SnapAttraction(F_CMD_ARGS)
{
  int val;
  char *token;

  if (GetIntegerArguments(action, &action, &val, 1) != 1)
  {
    Scr.SnapAttraction = DEFAULT_SNAP_ATTRACTION;
    Scr.SnapMode = DEFAULT_SNAP_ATTRACTION_MODE;
    return;
  }
  Scr.SnapAttraction = val;
  if (val < 0)
  {
    Scr.SnapAttraction = DEFAULT_SNAP_ATTRACTION;
  }
  if (val == 0)
  {
    return;
  }

  token = PeekToken(action, &action);
  if (token == NULL)
  {
    return;
  }

  if (StrEquals(token,"All"))
  {
    Scr.SnapMode = SNAP_ICONS | SNAP_WINDOWS;
  }
  else if (StrEquals(token,"SameType"))
  {
    Scr.SnapMode = SNAP_SAME;
  }
  else if (StrEquals(token,"Icons"))
  {
    Scr.SnapMode = SNAP_ICONS;
  }
  else if (StrEquals(token,"Windows"))
  {
    Scr.SnapMode = SNAP_WINDOWS;
  }
  if (Scr.SnapMode == 0)
  {
    Scr.SnapMode = DEFAULT_SNAP_ATTRACTION_MODE;
  }
  else
  {
    token = PeekToken(action, &action);
    if (token == NULL)
    {
      return;
    }
  }

  if (StrEquals(token, "Screen"))
  {
    Scr.SnapMode |= SNAP_SCREEN;
  }
  else
  {
    fvwm_msg(ERR,"SetSnapAttraction", "Invalid argument: %s", token);
  }

  return;
}

void CMD_SnapGrid(F_CMD_ARGS)
{
  int val[2];

  if(GetIntegerArguments(action, NULL, &val[0], 2) != 2)
  {
    Scr.SnapGridX = DEFAULT_SNAP_GRID_X;
    Scr.SnapGridY = DEFAULT_SNAP_GRID_Y;
    return;
  }

  Scr.SnapGridX = val[0];
  if(Scr.SnapGridX < 1)
  {
    Scr.SnapGridX = DEFAULT_SNAP_GRID_X;
  }
  Scr.SnapGridY = val[1];
  if(Scr.SnapGridY < 1)
  {
    Scr.SnapGridY = DEFAULT_SNAP_GRID_Y;
  }
}

static Pixmap XorPixmap = None;

void CMD_XorValue(F_CMD_ARGS)
{
  int val;
  XGCValues gcv;
  unsigned long gcm;

  if(GetIntegerArguments(action, NULL, &val, 1) != 1)
  {
    val = 0;
  }

  gcm = GCFunction|GCLineWidth|GCForeground|GCFillStyle|GCSubwindowMode;
  gcv.subwindow_mode = IncludeInferiors;
  gcv.function = GXxor;
  gcv.line_width = 0;
  /* use passed in value, or try to calculate appropriate value if 0 */
  /* ctwm method: */
  /*
    gcv.foreground = (val1)?(val1):((((unsigned long) 1) << Scr.d_depth) - 1);
  */
  /* Xlib programming manual suggestion: */
  gcv.foreground = (val)?
    (val):(BlackPixel(dpy,Scr.screen) ^ WhitePixel(dpy,Scr.screen));
  gcv.fill_style = FillSolid;
  gcv.subwindow_mode = IncludeInferiors;

  /* modify XorGC, only create once */
  if (Scr.XorGC)
    XChangeGC(dpy, Scr.XorGC, gcm, &gcv);
  else
    Scr.XorGC = fvwmlib_XCreateGC(dpy, Scr.Root, gcm, &gcv);

  /* free up XorPixmap if neccesary */
  if (XorPixmap != None) {
    XFreePixmap(dpy, XorPixmap);
    XorPixmap = None;
  }
}


void CMD_XorPixmap(F_CMD_ARGS)
{
  char *PixmapName;
  Picture *xp;
  XGCValues gcv;
  unsigned long gcm;

  action = GetNextToken(action, &PixmapName);
  if(PixmapName == NULL)
  {
    /* return to default value. */
    action = "0";
    CMD_XorValue(F_PASS_ARGS);
    return;
  }
  /* get the picture in the root visual, colorlimit is ignored because the
   * pixels will be freed */
  UseDefaultVisual();
  xp = GetPicture(dpy, Scr.Root, NULL, PixmapName, 0);
  if (xp == NULL) {
    fvwm_msg(ERR,"SetXORPixmap","Can't find pixmap %s", PixmapName);
    free(PixmapName);
    UseFvwmVisual();
    return;
  }
  free(PixmapName);
  /* free up old pixmap */
  if (XorPixmap != None)
    XFreePixmap(dpy, XorPixmap);

  /* make a copy of the picture pixmap */
  XorPixmap = XCreatePixmap(dpy, Scr.Root, xp->width, xp->height, Pdepth);
  XCopyArea(dpy, xp->picture, XorPixmap, DefaultGC(dpy, Scr.screen), 0, 0,
	    xp->width, xp->height, 0, 0);
  /* destroy picture and free colors */
  DestroyPicture(dpy, xp);
  UseFvwmVisual();

  /* create Graphics context */
  gcm = GCFunction|GCLineWidth|GCTile|GCFillStyle|GCSubwindowMode;
  gcv.subwindow_mode = IncludeInferiors;
  gcv.function = GXxor;
  /* line width of 1 is necessary for Exceed servers */
  gcv.line_width = 1;
  gcv.tile = XorPixmap;
  gcv.fill_style = FillTiled;
  gcv.subwindow_mode = IncludeInferiors;
  /* modify XorGC, only create once */
  if (Scr.XorGC)
    XChangeGC(dpy, Scr.XorGC, gcm, &gcv);
  else
    Scr.XorGC = fvwmlib_XCreateGC(dpy, Scr.Root, gcm, &gcv);
}


/* ----------------------------- resizing code ----------------------------- */

/***********************************************************************
 *
 * window resizing borrowed from the "wm" window manager
 *
 ***********************************************************************/

/****************************************************************************
 *
 * Starts a window resize operation
 *
 ****************************************************************************/
void CMD_Resize(F_CMD_ARGS)
{
  extern Window bad_window;
  Bool finished = False, done = False, abort = False;
  Bool do_resize_opaque;
  int x,y,delta_x,delta_y,stashed_x,stashed_y;
  Window ResizeWindow;
  Bool fButtonAbort = False;
  Bool fForceRedraw = False;
  int n;
  unsigned int button_mask = 0;
  rectangle sdrag;
  rectangle sorig;
  rectangle *drag = &sdrag;
  rectangle *orig = &sorig;
  rectangle start_g;
  int ymotion = 0;
  int xmotion = 0;
  int was_maximized;
  unsigned edge_wrap_x;
  unsigned edge_wrap_y;
  int px;
  int py;
  int i;
  Bool called_from_title = False;

  bad_window = False;
  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_RESIZE, ButtonPress))
    return;
  if (tmp_win == NULL)
    return;

  ResizeWindow = tmp_win->frame;
  XQueryPointer( dpy, ResizeWindow, &JunkRoot, &JunkChild,
		 &JunkX, &JunkY, &px, &py, &button_mask);
  button_mask &= DEFAULT_ALL_BUTTONS_MASK;

  if (!check_if_function_allowed(F_RESIZE, tmp_win, True, NULL))
  {
    XBell(dpy, 0);
    return;
  }

  was_maximized = IS_MAXIMIZED(tmp_win);
  SET_MAXIMIZED(tmp_win, 0);
  if (was_maximized)
  {
    /* must redraw the buttons now so that the 'maximize' button does not stay
     * depressed. */
    DrawDecorations(
      tmp_win, DRAW_BUTTONS, (tmp_win == Scr.Hilite), True, None);
  }

  if (IS_SHADED(tmp_win) || !IS_MAPPED(tmp_win))
    do_resize_opaque = False;
  else
    do_resize_opaque = DO_RESIZE_OPAQUE(tmp_win);

  /* no suffix = % of screen, 'p' = pixels, 'c' = increment units */
  drag->width = tmp_win->frame_g.width;
  drag->height = tmp_win->frame_g.height;
  n = GetResizeArguments(
    &action, tmp_win->frame_g.x, tmp_win->frame_g.y,
    tmp_win->hints.base_width, tmp_win->hints.base_height,
    tmp_win->hints.width_inc, tmp_win->hints.height_inc,
    tmp_win->boundary_width, tmp_win->title_g.height,
    &(drag->width), &(drag->height));

  if (n == 2)
  {
    rectangle new_g;

    /* size will be less or equal to requested */
    new_g = tmp_win->frame_g;
    constrain_size(
      tmp_win, (unsigned int *)&drag->width, (unsigned int *)&drag->height,
      xmotion, ymotion, 0);
    gravity_resize(
      tmp_win->hints.win_gravity, &new_g,
      drag->width - new_g.width, drag->height - new_g.height);
    tmp_win->frame_g = new_g;
    if (IS_SHADED(tmp_win))
    {
      ForceSetupFrame(tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
		      drag->width, tmp_win->frame_g.height, False);
    }
    else
    {
      ForceSetupFrame(tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
		      drag->width, drag->height, False);
    }
    DrawDecorations(tmp_win, DRAW_ALL, True, True, None);
    update_absolute_geometry(tmp_win);
    maximize_adjust_offset(tmp_win);
    GNOME_SetWinArea(tmp_win);
    ResizeWindow = None;
    return;
  }

  if (Scr.bo.InstallRootCmap)
    InstallRootColormap();
  else
    InstallFvwmColormap();

  if(!GrabEm(CRS_RESIZE, GRAB_NORMAL))
  {
    XBell(dpy, 0);
    return;
  }

  /* handle problems with edge-wrapping while resizing */
  edge_wrap_x = Scr.flags.edge_wrap_x;
  edge_wrap_y = Scr.flags.edge_wrap_y;
  Scr.flags.edge_wrap_x = 0;
  Scr.flags.edge_wrap_y = 0;

  if (!do_resize_opaque)
  {
    MyXGrabServer(dpy);
  }
  if (!XGetGeometry(
	dpy, (Drawable) ResizeWindow, &JunkRoot, &drag->x, &drag->y,
	(unsigned int *)&drag->width, (unsigned int *)&drag->height,
	&JunkBW, &JunkDepth))
  {
    UngrabEm(GRAB_NORMAL);
    if (!do_resize_opaque)
    {
      MyXUngrabServer(dpy);
    }
    return;
  }
  if (IS_SHADED(tmp_win))
  {
    SET_MAXIMIZED(tmp_win, was_maximized);
    get_unshaded_geometry(tmp_win, drag);
    SET_MAXIMIZED(tmp_win, 0);
  }
  if (!do_resize_opaque)
  {
    Scr.flags.is_wire_frame_displayed = True;
  }
  MyXGrabKeyboard(dpy);

  *orig = *drag;
  start_g = *drag;
  ymotion = 0;
  xmotion = 0;

  /* pop up a resize dimensions window */
  if (!Scr.gs.do_hide_resize_window)
  {
    position_geometry_window(NULL);
    XMapRaised(dpy, Scr.SizeWindow);
  }
  DisplaySize(tmp_win, &Event, orig->width, orig->height,True,True);

  if((PressedW != Scr.Root)&&(PressedW != None))
  {
    /* Get the current position to determine which border to resize */
    if(PressedW == tmp_win->sides[0])   /* top */
      ymotion = 1;
    else if(PressedW == tmp_win->sides[1])  /* right */
      xmotion = -1;
    else if(PressedW == tmp_win->sides[2])  /* bottom */
      ymotion = -1;
    else if(PressedW == tmp_win->sides[3])  /* left */
      xmotion = 1;
    else if(PressedW == tmp_win->corners[0])  /* upper-left */
    {
      ymotion = 1;
      xmotion = 1;
    }
    else if(PressedW == tmp_win->corners[1])  /* upper-right */
    {
      xmotion = -1;
      ymotion = 1;
    }
    else if(PressedW == tmp_win->corners[2]) /* lower left */
    {
      ymotion = -1;
      xmotion = 1;
    }
    else if(PressedW == tmp_win->corners[3])  /* lower right */
    {
      ymotion = -1;
      xmotion = -1;
    }
  }

  /* begin of code responsible for warping the pointer to the border when
   * starting a resize. */
  if (tmp_win->title_w != None && PressedW == tmp_win->title_w)
  {
    /* title was pressed to thart the resize */
    called_from_title = True;
  }
  else
  {
    for (i = NUMBER_OF_BUTTONS; i--; )
    {
      /* see if the title button was pressed to that the resize */
      if (tmp_win->button_w[i] != None && PressedW == tmp_win->button_w[i])
      {
	/* yes */
	called_from_title = True;
      }
    }
  }
  /* don't warp if the resize was triggered by a press somwhere on the title
   * bar */
  if(PressedW != Scr.Root && xmotion == 0 && ymotion == 0 &&
     !called_from_title)
  {
    int dx = orig->width - px;
    int dy = orig->height - py;
    int wx = -1;
    int wy = -1;
    int tx;
    int ty;

    /* Now find the place to warp to. We simply use the sectors drawn when we
     * start resizing the window. */
#if 0
    tx = orig->width / 10 - 1;
    ty = orig->height / 10 - 1;
#else
    tx = 0;
    ty = 0;
#endif
    tx = max(tmp_win->boundary_width, tx);
    ty = max(tmp_win->boundary_width, ty);
    if (px >= 0 && dx >= 0 && py >= 0 && dy >= 0)
    {
      if (px < tx)
      {
	if (py < ty)
	{
	  xmotion = 1;
	  ymotion = 1;
	  wx = 0;
	  wy = 0;
	}
	else if (dy < ty)
	{
	  xmotion = 1;
	  ymotion = -1;
	  wx = 0;
	  wy = orig->height -1;
	}
	else
	{
	  xmotion = 1;
	  wx = 0;
	  wy = orig->height/2;
	  wy = py;
	}
      }
      else if (dx < tx)
      {
	if (py < ty)
	{
	  xmotion = -1;
	  ymotion = 1;
	  wx = orig->width - 1;
	  wy = 0;
	}
	else if (dy < ty)
	{
	  xmotion = -1;
	  ymotion = -1;
	  wx = orig->width - 1;
	  wy = orig->height -1;
	}
	else
	{
	  xmotion = -1;
	  wx = orig->width - 1;
	  wy = orig->height/2;
	  wy = py;
	}
      }
      else
      {
	if (py < ty)
	{
	  ymotion = 1;
	  wx = orig->width/2;
	  wy = 0;
	  wx = px;
	}
	else if (dy < ty)
	{
	  ymotion = -1;
	  wx = orig->width/2;
	  wy = orig->height -1;
	  wx = px;
	}
      }
    }

    if (wx != -1)
    {
      /* now warp the pointer to the border */
      XWarpPointer(dpy, None, ResizeWindow, 0, 0, 1, 1, wx, wy);
      XFlush(dpy);
    }
  }
  /* end of code responsible for warping the pointer to the border when
   * starting a resize. */

  /* draw the rubber-band window */
  if (!do_resize_opaque)
    draw_move_resize_grid(drag->x, drag->y, drag->width - 1, drag->height - 1);
  /* kick off resizing without requiring any motion if invoked with a key
   * press */
  if (eventp->type == KeyPress)
  {
    XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		  &stashed_x,&stashed_y,&JunkX, &JunkY, &JunkMask);
    DoResize(stashed_x, stashed_y, tmp_win, drag, orig, &xmotion, &ymotion,
	     do_resize_opaque);
  }
  else
    stashed_x = stashed_y = -1;

  /* loop to resize */
  while(!finished && bad_window != tmp_win->w)
  {
    int rc = 0;

    /* block until there is an interesting event */
    while (rc != -1 &&
	   (!XCheckMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
			     KeyPressMask | PointerMotionMask |
			     ButtonMotionMask | ExposureMask, &Event)))
    {
      rc = HandlePaging(
	      Scr.EdgeScrollX, Scr.EdgeScrollY, &x, &y, &delta_x, &delta_y,
	      False, False, True);
      if (rc == 1)
      {
	/* Fake an event to force window reposition */
	Event.type = MotionNotify;
	Event.xmotion.time = lastTimestamp;
	fForceRedraw = True;
	break;
      }
    }
    if (rc == -1)
    {
      XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
		 KeyPressMask | PointerMotionMask |
		 ButtonMotionMask | ExposureMask, &Event);
    }
    StashEventTime(&Event);

    if (Event.type == MotionNotify)
    {
      /* discard any extra motion events before a release */
      while(XCheckMaskEvent(dpy, ButtonMotionMask | PointerMotionMask |
			    ButtonReleaseMask | ButtonPressMask, &Event))
      {
	StashEventTime(&Event);
	if (Event.type == ButtonRelease || Event.type == ButtonPress)
	  break;
      }
    }

    done = False;
    /* Handle a limited number of key press events to allow mouseless
     * operation */
    if(Event.type == KeyPress)
      Keyboard_shortcuts(&Event, tmp_win, NULL, NULL, ButtonRelease);
    switch(Event.type)
    {
    case ButtonPress:
      done = True;
      if (Event.xbutton.button <= NUMBER_OF_MOUSE_BUTTONS &&
	  ((Button1Mask << (Event.xbutton.button - 1)) & button_mask))
      {
	/* No new button was pressed, just a delayed event */
	break;
      }
      /* Abort the resize if
       *  - the move started with a pressed button and another button
       *    was pressed during the operation
       *  - no button was started at the beginning and any button
       *    except button 1 was pressed. */
      if (button_mask || (Event.xbutton.button != 1))
	fButtonAbort = True;
    case KeyPress:
      /* simple code to bag out of move - CKH */
      if (XLookupKeysym(&(Event.xkey),0) == XK_Escape || fButtonAbort)
      {
	abort = True;
	finished = True;
	/* return pointer if aborted resize was invoked with key */
	if (stashed_x >= 0)
	{
	  XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, stashed_x,
		       stashed_y);
	}
	if (do_resize_opaque)
	{
	  DoResize(
	    start_g.x, start_g.y, tmp_win, &start_g, orig, &xmotion, &ymotion,
	    do_resize_opaque);
	  DrawDecorations(tmp_win, DRAW_ALL, True, True, None);
	}
      }
      done = True;
      break;

    case ButtonRelease:
      finished = True;
      done = True;
      break;

    case MotionNotify:
      if (Event.xmotion.same_screen == False)
      {
	continue;
      }
      if (!fForceRedraw)
      {
	x = Event.xmotion.x_root;
	y = Event.xmotion.y_root;
	/* resize before paging request to prevent resize from lagging
	 * mouse - mab */
	DoResize(
	  x, y, tmp_win, drag, orig, &xmotion, &ymotion, do_resize_opaque);
	/* need to move the viewport */
	HandlePaging(Scr.EdgeScrollX, Scr.EdgeScrollY, &x, &y,
		     &delta_x, &delta_y, False, False, False);
      }
      /* redraw outline if we paged - mab */
      if (delta_x != 0 || delta_y != 0)
      {
	orig->x -= delta_x;
	orig->y -= delta_y;
	drag->x -= delta_x;
	drag->y -= delta_y;

	DoResize(
	  x, y, tmp_win, drag, orig, &xmotion, &ymotion, do_resize_opaque);
      }
      fForceRedraw = False;
      done = True;
      break;

    default:
      break;
    }
    if(!done)
    {
      if (!do_resize_opaque)
	/* must undraw the rubber band in case the event causes some drawing */
	switch_move_resize_grid(False);
      DispatchEvent(False);
      if (!do_resize_opaque)
	draw_move_resize_grid(
	  drag->x, drag->y, drag->width - 1, drag->height - 1);
    }
    else
    {
      if (do_resize_opaque)
      {
	/* only do this with opaque resizes, (i.e. the server is not grabbed)
	 */
	BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
	FlushAllMessageQueues();
      }
    }
  }

  /* erase the rubber-band */
  if (!do_resize_opaque)
    switch_move_resize_grid(False);

  /* pop down the size window */
  if (!Scr.gs.do_hide_resize_window)
    XUnmapWindow(dpy, Scr.SizeWindow);

  if(!abort && bad_window != tmp_win->w)
  {
    /* size will be >= to requested */
    constrain_size(
      tmp_win, (unsigned int *)&drag->width, (unsigned int *)&drag->height,
      xmotion, ymotion, CS_ROUND_UP);
    if (IS_SHADED(tmp_win))
    {
      if (HAS_BOTTOM_TITLE(tmp_win))
      {
	SetupFrame(tmp_win, drag->x, tmp_win->frame_g.y,
		   drag->width, tmp_win->frame_g.height, False);
      }
      else
      {
	SetupFrame(tmp_win, drag->x, drag->y,
		   drag->width, tmp_win->frame_g.height, False);
      }
      tmp_win->normal_g.height = drag->height;
    }
    else
    {
      SetupFrame(
	tmp_win, drag->x, drag->y, drag->width, drag->height, False);
    }
  }
  if (abort && was_maximized)
  {
    /* since we aborted the resize, the window is still maximized */
    SET_MAXIMIZED(tmp_win, 1);
    /* force redraw */
    DrawDecorations(
      tmp_win, DRAW_BUTTONS, (tmp_win == Scr.Hilite), True, None);
  }

  if (bad_window == tmp_win->w)
  {
    XUnmapWindow(dpy, tmp_win->frame);
    XBell(dpy, 0);
  }

  if (Scr.bo.InstallRootCmap)
    UninstallRootColormap();
  else
    UninstallFvwmColormap();
  ResizeWindow = None;
  if (!do_resize_opaque)
  {
    /* Throw away some events that dont interest us right now. */
    discard_events(EnterWindowMask|LeaveWindowMask);
    Scr.flags.is_wire_frame_displayed = False;
    MyXUngrabServer(dpy);
  }
  MyXUngrabKeyboard(dpy);
  xmotion = 0;
  ymotion = 0;
  WaitForButtonsUp(True);
  UngrabEm(GRAB_NORMAL);
  Scr.flags.edge_wrap_x = edge_wrap_x;
  Scr.flags.edge_wrap_y = edge_wrap_y;
  update_absolute_geometry(tmp_win);
  maximize_adjust_offset(tmp_win);
  GNOME_SetWinArea(tmp_win);

  return;
}



/***********************************************************************
 *
 *  Procedure:
 *      DoResize - move the rubberband around.  This is called for
 *                 each motion event when we are resizing
 *
 *  Inputs:
 *      x_root   - the X corrdinate in the root window
 *      y_root   - the Y corrdinate in the root window
 *      tmp_win  - the current fvwm window
 *      drag     - resize internal structure
 *      orig     - resize internal structure
 *      xmotionp - pointer to xmotion in resize_window
 *      ymotionp - pointer to ymotion in resize_window
 *
 ************************************************************************/
static void DoResize(
  int x_root, int y_root, FvwmWindow *tmp_win, rectangle *drag,
  rectangle *orig, int *xmotionp, int *ymotionp, Bool do_resize_opaque)
{
  int action = 0;
  XEvent e;

  if ((y_root <= orig->y) ||
      ((*ymotionp == 1)&&(y_root < orig->y+orig->height-1)))
  {
    drag->y = y_root;
    drag->height = orig->y + orig->height - y_root;
    action = 1;
    *ymotionp = 1;
  }
  else if ((y_root >= orig->y + orig->height - 1)||
	   ((*ymotionp == -1)&&(y_root > orig->y)))
  {
    drag->y = orig->y;
    drag->height = 1 + y_root - drag->y;
    action = 1;
    *ymotionp = -1;
  }

  if ((x_root <= orig->x)||
      ((*xmotionp == 1)&&(x_root < orig->x + orig->width - 1)))
  {
    drag->x = x_root;
    drag->width = orig->x + orig->width - x_root;
    action = 1;
    *xmotionp = 1;
  }
  if ((x_root >= orig->x + orig->width - 1)||
    ((*xmotionp == -1)&&(x_root > orig->x)))
  {
    drag->x = orig->x;
    drag->width = 1 + x_root - orig->x;
    action = 1;
    *xmotionp = -1;
  }

  if (action)
  {
    /* round up to nearest OK size to keep pointer inside rubberband */
    constrain_size(
      tmp_win, (unsigned int *)&drag->width, (unsigned int *)&drag->height,
      *xmotionp, *ymotionp, CS_ROUND_UP);
    if (*xmotionp == 1)
      drag->x = orig->x + orig->width - drag->width;
    if (*ymotionp == 1)
      drag->y = orig->y + orig->height - drag->height;

    if(!do_resize_opaque)
    {
      draw_move_resize_grid(
	drag->x, drag->y, drag->width - 1, drag->height - 1);
    }
    else
    {
      SetupFrame(
	tmp_win, drag->x, drag->y, drag->width, drag->height, False);
    }
  }
  e.type = MotionNotify;
  e.xbutton.x_root = x_root;
  e.xbutton.y_root = y_root;
  DisplaySize(tmp_win, &e, drag->width, drag->height,False,False);
}



/***********************************************************************
 *
 *  Procedure:
 *	draw_move_resize_grid - move a window outline
 *
 *  Inputs:
 *	root	    - the window we are outlining
 *	x	    - upper left x coordinate
 *	y	    - upper left y coordinate
 *	width	    - the width of the rectangle
 *	height	    - the height of the rectangle
 *
 ***********************************************************************/
static int get_outline_rects(
  XRectangle *rects, int x, int y, int width, int height)
{
  int i;
  int n;
  int m;

  n = 3;
  m = (width - 5) / 2;
  if (m < n)
  {
    n = m;
  }
  m = (height - 5) / 2;
  if (m < n)
  {
    n = m;
  }
  if (n < 1)
  {
    n = 1;
  }

  for (i = 0; i < n; i++)
  {
    rects[i].x = x + i;
    rects[i].y = y + i;
    rects[i].width = width - (i << 1);
    rects[i].height = height - (i << 1);
  }
  if (width - (n << 1) >= 5 && height - (n << 1) >= 5)
  {
    if (width - (n << 1) >= 10)
    {
      int off = (width - (n << 1)) / 3 + n;
      rects[i].x = x + off;
      rects[i].y = y + n;
      rects[i].width = width - (off << 1);
      rects[i].height = height - (n << 1);
      i++;
    }
    if (height - (n << 1) >= 10)
    {
      int off = (height - (n << 1)) / 3 + n;
      rects[i].x = x + n;
      rects[i].y = y + off;
      rects[i].width = width - (n << 1);
      rects[i].height = height - (off << 1);
      i++;
    }
  }

  return i;
}

struct
{
  rectangle geom;
  struct
  {
    unsigned is_enabled : 1;
  } flags;
} move_resize_grid =
{
  { 0, 0, 0, 0 },
  { 0 }
};

static void draw_move_resize_grid(int x, int  y, int  width, int height)
{
  int nrects = 0;
  XRectangle rects[10];

  if (move_resize_grid.flags.is_enabled &&
      x == move_resize_grid.geom.x &&
      y == move_resize_grid.geom.y &&
      width == move_resize_grid.geom.width &&
      height == move_resize_grid.geom.height)
  {
    return;
  }

  memset(rects, 0, 10 * sizeof(XRectangle));
  /* place the resize rectangle into the array of rectangles */
  /* interleave them for best visual look */
  /* draw the new one, if any */
  if (move_resize_grid.flags.is_enabled
      /*move_resize_grid.geom.width && move_resize_grid.geom.height*/)
  {
    move_resize_grid.flags.is_enabled = 0;
    nrects +=
      get_outline_rects(
	&(rects[0]), move_resize_grid.geom.x, move_resize_grid.geom.y,
	move_resize_grid.geom.width, move_resize_grid.geom.height);
  }
  if (width && height)
  {
    move_resize_grid.flags.is_enabled = 1;
    move_resize_grid.geom.x = x;
    move_resize_grid.geom.y = y;
    move_resize_grid.geom.width = width;
    move_resize_grid.geom.height = height;
    nrects += get_outline_rects(&(rects[nrects]), x, y, width, height);
  }
  if (nrects > 0)
  {
    XDrawRectangles(dpy, Scr.Root, Scr.XorGC, rects, nrects);
    XSync(dpy, 0);
  }

  return;
}

void switch_move_resize_grid(Bool state)
{
  if (state == False)
  {
    if (move_resize_grid.flags.is_enabled)
    {
      draw_move_resize_grid(0, 0, 0, 0);
    }
    else
    {
      move_resize_grid.geom.x = 0;
      move_resize_grid.geom.y = 0;
      move_resize_grid.geom.width = 0;
      move_resize_grid.geom.height = 0;
    }
  }
  else if (!move_resize_grid.flags.is_enabled)
  {
    if (move_resize_grid.geom.width && move_resize_grid.geom.height)
    {
      draw_move_resize_grid(
	move_resize_grid.geom.x, move_resize_grid.geom.y,
	move_resize_grid.geom.width, move_resize_grid.geom.height);
    }
  }

  return;
}

/* ----------------------------- maximizing code --------------------------- */

static void move_sticky_window_to_same_page(
  int *x11, int *x12, int *y11, int *y12, int x21, int x22, int y21, int y22)
{
  /* make sure the x coordinate is on the same page as the reference window */
  if (*x11 >= x22)
  {
    while (*x11 >= x22)
    {
      *x11 -= Scr.MyDisplayWidth;
      *x12 -= Scr.MyDisplayWidth;
    }
  }
  else if (*x12 <= x21)
  {
    while (*x12 <= x21)
    {
      *x11 += Scr.MyDisplayWidth;
      *x12 += Scr.MyDisplayWidth;
    }
  }
  /* make sure the y coordinate is on the same page as the reference window */
  if (*y11 >= y22)
  {
    while (*y11 >= y22)
    {
      *y11 -= Scr.MyDisplayHeight;
      *y12 -= Scr.MyDisplayHeight;
    }
  }
  else if (*y12 <= y21)
  {
    while (*y12 <= y21)
    {
      *y11 += Scr.MyDisplayHeight;
      *y12 += Scr.MyDisplayHeight;
    }
  }

}
static void MaximizeHeight(
  FvwmWindow *win, unsigned int win_width, int win_x, unsigned int *win_height,
  int *win_y, Bool grow_up, Bool grow_down,
  int top_border, int bottom_border)
{
  FvwmWindow *cwin;
  int x11, x12, x21, x22;
  int y11, y12, y21, y22;
  int new_y1, new_y2;
  Bool is_sticky;

  x11 = win_x;             /* Start x */
  y11 = *win_y;            /* Start y */
  x12 = x11 + win_width;   /* End x   */
  y12 = y11 + *win_height; /* End y   */
  new_y1 = top_border;
  new_y2 = bottom_border;

  for (cwin = Scr.FvwmRoot.next; cwin; cwin = cwin->next)
  {
    if (IS_STICKY(cwin) || (IS_ICONIFIED(cwin) && IS_ICON_STICKY(cwin)))
      is_sticky = True;
    else
      is_sticky = False;
    if (cwin == win || (cwin->Desk != win->Desk && !is_sticky))
      continue;
    if (IS_ICONIFIED(cwin))
    {
      if(cwin->icon_w == None || IS_ICON_UNMAPPED(cwin))
	continue;
      x21 = cwin->icon_g.x;
      y21 = cwin->icon_g.y;
      x22 = x21 + cwin->icon_p_width;
      y22 = y21 + cwin->icon_p_height + cwin->icon_g.height;
    }
    else
    {
      x21 = cwin->frame_g.x;
      y21 = cwin->frame_g.y;
      x22 = x21 + cwin->frame_g.width;
      y22 = y21 + cwin->frame_g.height;
    }
    if (is_sticky)
    {
      move_sticky_window_to_same_page(
	&x21, &x22, &new_y1, &new_y2, x11, x12, y11, y12);
    }

    /* Are they in the same X space? */
    if (!((x22 <= x11) || (x21 >= x12)))
    {
      if ((y22 <= y11) && (y22 >= new_y1))
      {
	new_y1 = y22;
      }
      else if ((y12 <= y21) && (new_y2 >= y21))
      {
	new_y2 = y21;
      }
    }
  }
  if (!grow_up)
    new_y1 = y11;
  if (!grow_down)
    new_y2 = y12;
  *win_height = new_y2 - new_y1;
  *win_y = new_y1;
}

static void MaximizeWidth(
  FvwmWindow *win, unsigned int *win_width, int *win_x, unsigned int win_height,
  int win_y, Bool grow_left, Bool grow_right,
  int left_border, int right_border)
{
  FvwmWindow *cwin;
  int x11, x12, x21, x22;
  int y11, y12, y21, y22;
  int new_x1, new_x2;
  Bool is_sticky;

  x11 = *win_x;            /* Start x */
  y11 = win_y;             /* Start y */
  x12 = x11 + *win_width;  /* End x   */
  y12 = y11 + win_height;  /* End y   */
  new_x1 = left_border;
  new_x2 = right_border;

  for (cwin = Scr.FvwmRoot.next; cwin; cwin = cwin->next)
  {
    if (IS_STICKY(cwin) || (IS_ICONIFIED(cwin) && IS_ICON_STICKY(cwin)))
      is_sticky = True;
    else
      is_sticky = False;
    if (cwin == win || (cwin->Desk != win->Desk && !is_sticky))
      continue;
    if (IS_ICONIFIED(cwin))
    {
      if(cwin->icon_w == None || IS_ICON_UNMAPPED(cwin))
	continue;
      x21 = cwin->icon_g.x;
      y21 = cwin->icon_g.y;
      x22 = x21 + cwin->icon_p_width;
      y22 = y21 + cwin->icon_p_height + cwin->icon_g.height;
    }
    else
    {
      x21 = cwin->frame_g.x;
      y21 = cwin->frame_g.y;
      x22 = x21 + cwin->frame_g.width;
      y22 = y21 + cwin->frame_g.height;
    }
    if (is_sticky)
    {
      move_sticky_window_to_same_page(
	&new_x1, &new_x2, &y21, &y22, x11, x12, y11, y12);
    }

    /* Are they in the same Y space? */
    if (!((y22 <= y11) || (y21 >= y12)))
    {
      if ((x22 <= x11) && (x22 >= new_x1))
      {
	new_x1 = x22;
      }
      else if ((x12 <= x21) && (new_x2 >= x21))
      {
	new_x2 = x21;
      }
    }
  }
  if (!grow_left)
    new_x1 = x11;
  if (!grow_right)
    new_x2 = x12;
  *win_width  = new_x2 - new_x1;
  *win_x = new_x1;
}

/***********************************************************************
 *
 *  Procedure:
 *	(Un)Maximize a window.
 *
 ***********************************************************************/
void CMD_Maximize(F_CMD_ARGS)
{
  int page_x, page_y;
  int val1, val2, val1_unit, val2_unit;
  int toggle;
  char *token;
  char *taction;
  Bool grow_up = False;
  Bool grow_down = False;
  Bool grow_left = False;
  Bool grow_right = False;
  Bool do_force_maximize = False;
  Bool is_screen_given = False;
  int  scr_x, scr_y, scr_w, scr_h;
  rectangle new_g;
  FvwmWindow *sf;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;
  if (tmp_win == NULL)
    return;

  if (check_if_function_allowed(F_MAXIMIZE,tmp_win,True,NULL) == 0)
  {
    XBell(dpy, 0);
    return;
  }
  /* Check for "global" flag ("absolute" is for compatibility with E) */
  token = PeekToken(action, &taction);
  if (token)
  {
    if (StrEquals(token, "screen"))
    {
      int scr;

      is_screen_given = True;
      token = PeekToken(taction, &action);
      scr = FScreenGetScreenArgument(token, FSCREEN_SPEC_PRIMARY);
      FScreenGetScrRect(NULL, scr, &scr_x, &scr_y, &scr_w, &scr_h);
    }
  }
  toggle = ParseToggleArgument(action, &action, -1, 0);
  if (toggle == 0 && !IS_MAXIMIZED(tmp_win))
    return;

  if (toggle == 1 && IS_MAXIMIZED(tmp_win))
  {
    /* Fake that the window is not maximized. */
    do_force_maximize = True;
  }

  /* find the new page and geometry */
  new_g.x = tmp_win->frame_g.x;
  new_g.y = tmp_win->frame_g.y;
  new_g.width = tmp_win->frame_g.width;
  new_g.height = tmp_win->frame_g.height;
  if (IsRectangleOnThisPage(&tmp_win->frame_g, tmp_win->Desk))
  {
    /* maximize on visible page if any part of the window is visible */
    page_x = 0;
    page_y = 0;
  }
  else
  {
    int xoff = Scr.Vx % Scr.MyDisplayWidth;
    int yoff = Scr.Vy % Scr.MyDisplayHeight;

    /* maximize on the page where the center of the window is */
    page_x = truncate_to_multiple(
      tmp_win->frame_g.x + tmp_win->frame_g.width / 2 + xoff,
      Scr.MyDisplayWidth) - xoff;
    page_y = truncate_to_multiple(
      tmp_win->frame_g.y + tmp_win->frame_g.height / 2 + yoff,
      Scr.MyDisplayHeight) - yoff;
  }

  /* Check if we should constrain rectangle to some Xinerama screen */
  if (!is_screen_given)
  {
    fscreen_scr_arg fscr;

    fscr.xypos.x = tmp_win->frame_g.x + tmp_win->frame_g.width  / 2 - page_x;
    fscr.xypos.y = tmp_win->frame_g.y + tmp_win->frame_g.height / 2 - page_y;
    FScreenGetScrRect(&fscr, FSCREEN_XYPOS, &scr_x, &scr_y, &scr_w, &scr_h);
  }

#if 0
  fprintf(stderr, "%s: page=(%d,%d), scr=(%d,%d, %dx%d)\n", __FUNCTION__,
          page_x, page_y, scr_x, scr_y, scr_w, scr_h);
#endif

  /* parse first parameter */
  val1_unit = scr_w;
  token = PeekToken(action, &taction);
  if (token && StrEquals(token, "grow"))
  {
    grow_left = True;
    grow_right = True;
    val1 = 100;
    val1_unit = scr_w;
  }
  else if (token && StrEquals(token, "growleft"))
  {
    grow_left = True;
    val1 = 100;
    val1_unit = scr_w;
  }
  else if (token && StrEquals(token, "growright"))
  {
    grow_right = True;
    val1 = 100;
    val1_unit = scr_w;
  }
  else
  {
    if (GetOnePercentArgument(token, &val1, &val1_unit) == 0)
    {
      val1 = 100;
      val1_unit = scr_w;
    }
    else if (val1 < 0)
    {
      /* handle negative offsets */
      if (val1_unit == scr_w)
      {
	val1 = 100 + val1;
      }
      else
      {
	val1 = scr_w + val1;
      }
    }
  }

  /* parse second parameter */
  val2_unit = scr_h;
  token = PeekToken(taction, NULL);
  if (token && StrEquals(token, "grow"))
  {
    grow_up = True;
    grow_down = True;
    val2 = 100;
    val2_unit = scr_h;
  }
  else if (token && StrEquals(token, "growup"))
  {
    grow_up = True;
    val2 = 100;
    val2_unit = scr_h;
  }
  else if (token && StrEquals(token, "growdown"))
  {
    grow_down = True;
    val2 = 100;
    val2_unit = scr_h;
  }
  else
  {
    if (GetOnePercentArgument(token, &val2, &val2_unit) == 0)
    {
      val2 = 100;
      val2_unit = scr_h;
    }
    else if (val2 < 0)
    {
      /* handle negative offsets */
      if (val2_unit == scr_h)
      {
	val2 = 100 + val2;
      }
      else
      {
	val2 = scr_h + val2;
      }
    }
  }

  if (IS_MAXIMIZED(tmp_win) && !do_force_maximize)
  {
    SET_MAXIMIZED(tmp_win, 0);
    get_relative_geometry(&tmp_win->frame_g, &tmp_win->normal_g);
    if (IS_SHADED(tmp_win))
      get_shaded_geometry(tmp_win, &tmp_win->frame_g, &tmp_win->frame_g);
    ForceSetupFrame(
      tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y, tmp_win->frame_g.width,
      tmp_win->frame_g.height, True);
    DrawDecorations(tmp_win, DRAW_ALL, True, True, None);
  }
  else /* maximize */
  {
    /* handle command line arguments */
    if (grow_up || grow_down)
    {
      MaximizeHeight(
	tmp_win, new_g.width, new_g.x, (unsigned int *)&new_g.height,
	&new_g.y, grow_up, grow_down,
	page_y + scr_y, page_y + scr_y + scr_h);
    }
    else if(val2 > 0)
    {
      new_g.height = val2 * val2_unit / 100;
      new_g.y = page_y + scr_y;
    }
    if (grow_left || grow_right)
    {
      MaximizeWidth(
	tmp_win, (unsigned int *)&new_g.width, &new_g.x, new_g.height,
	new_g.y, grow_left, grow_right,
	page_x + scr_x, page_x + scr_x + scr_w);
    }
    else if(val1 >0)
    {
      new_g.width = val1 * val1_unit / 100;
      new_g.x = page_x + scr_x;
    }
    if(val1 == 0 && val2 == 0)
    {
      new_g.x = page_x + scr_x;
      new_g.y = page_y + scr_y;
      new_g.height = scr_h;
      new_g.width = scr_w;
    }
    /* now maximize it */
    SET_MAXIMIZED(tmp_win, 1);
    tmp_win->max_g_defect.width = 0;
    tmp_win->max_g_defect.height = 0;
    constrain_size(tmp_win, (unsigned int *)&new_g.width,
		   (unsigned int *)&new_g.height, 0, 0, CS_UPDATE_MAX_DEFECT);
    tmp_win->max_g = new_g;
    if (IS_SHADED(tmp_win))
      get_shaded_geometry(tmp_win, &new_g, &tmp_win->max_g);
    SetupFrame(
      tmp_win, new_g.x, new_g.y, new_g.width, new_g.height, True);
    DrawDecorations(tmp_win, DRAW_ALL, (Scr.Hilite == tmp_win), True, None);
    update_absolute_geometry(tmp_win);
    /* remember the offset between old and new position in case the maximized
     * window is moved more than the screen width/height. */
    tmp_win->max_offset.x = tmp_win->normal_g.x - tmp_win->max_g.x;
    tmp_win->max_offset.y = tmp_win->normal_g.y - tmp_win->max_g.y;
#if 0
fprintf(stderr,"%d %d %d %d, max_offset.x = %d, max_offset.y = %d\n", tmp_win->max_g.x, tmp_win->max_g.y, tmp_win->max_g.width, tmp_win->max_g.height, tmp_win->max_offset.x, tmp_win->max_offset.y);
#endif
  }
  if ((sf = get_focus_window()))
  {
    focus_grab_buttons(sf, True);
  }
  GNOME_SetWinArea(tmp_win);
}

/* ----------------------------- stick code -------------------------------- */

void handle_stick(F_CMD_ARGS, int toggle)
{
  FvwmWindow *sf;

  if ((toggle == 1 && IS_STICKY(tmp_win)) ||
      (toggle == 0 && !IS_STICKY(tmp_win)))
    return;

  if(IS_STICKY(tmp_win))
  {
    SET_STICKY(tmp_win, 0);
    tmp_win->Desk = Scr.CurrentDesk;
    GNOME_SetDeskCount();
    GNOME_SetDesk(tmp_win);
  }
  else
  {
    if (tmp_win->Desk != Scr.CurrentDesk)
      do_move_window_to_desk(tmp_win, Scr.CurrentDesk);
    SET_STICKY(tmp_win, 1);
    if (!IsRectangleOnThisPage(&tmp_win->frame_g, Scr.CurrentDesk))
    {
      action = "";
      move_window_doit(F_PASS_ARGS, FALSE, MOVE_PAGE);
      /* move_window_doit resets the STICKY flag, so we must set it after the
       * call! */
      SET_STICKY(tmp_win, 1);
    }
  }
  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
  DrawDecorations(
    tmp_win, DRAW_TITLE | DRAW_BUTTONS, (Scr.Hilite==tmp_win), True, None);
  if ((sf = get_focus_window()))
  {
    focus_grab_buttons(sf, True);
  }
  GNOME_SetHints(tmp_win);
}

void CMD_Stick(F_CMD_ARGS)
{
  int toggle;

  if (DeferExecution(eventp,&w,&tmp_win,&context,CRS_SELECT,ButtonRelease))
    return;

  toggle = ParseToggleArgument(action, &action, -1, 0);

  handle_stick(F_PASS_ARGS, toggle);
}
