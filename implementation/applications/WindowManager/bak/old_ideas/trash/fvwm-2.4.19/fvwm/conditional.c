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

/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "update.h"
#include "style.h"
#include "focus.h"


/**********************************************************************
 * Parses the flag string and returns the text between [ ] or ( )
 * characters.  The start of the rest of the line is put in restptr.
 * Note that the returned string is allocated here and it must be
 * freed when it is not needed anymore.
 * NOTE - exported via .h
 **********************************************************************/
char *CreateFlagString(char *string, char **restptr)
{
  char *retval;
  char *c;
  char *start;
  char closeopt;
  int length;

  c = string;
  while (isspace((unsigned char)*c) && (*c != 0))
    c++;

  if (*c == '[' || *c == '(')
  {
    /* Get the text between [ ] or ( ) */
    if (*c == '[')
      closeopt = ']';
    else
      closeopt = ')';
    c++;
    start = c;
    length = 0;
    while (*c != closeopt) {
      char *d;
      if (*c == 0) {
	fvwm_msg(ERR, "CreateFlagString",
		 "Conditionals require closing parenthesis");
	*restptr = NULL;
	return NULL;
      }
      /* skip quoted string */
      d = SkipQuote(c, NULL, NULL, NULL);
      length += d - c;
      c = d;
    }

    /* We must allocate a new string because we null terminate the string
     * between the [ ] or ( ) characters.
     */
    retval = safemalloc(length + 1);
    strncpy(retval, start, length);
    retval[length] = 0;

    *restptr = c + 1;
  }
  else
  {
    retval = NULL;
    *restptr = c;
  }

  return retval;
}

/**********************************************************************
 * The name field of the mask is allocated in CreateConditionMask.
 * It must be freed.
 * NOTE - exported via .h
 **********************************************************************/
void FreeConditionMask(WindowConditionMask *mask)
{
  if (mask->my_flags.needs_name)
    free(mask->name);
  else if (mask->my_flags.needs_not_name)
    free(mask->name - 1);
}

/* Assign the default values for the window mask
 * NOTE - exported via .h
 */
void DefaultConditionMask(WindowConditionMask *mask)
{
  memset(mask, 0, sizeof(WindowConditionMask));
  mask->layer = -2; /* -2  means no layer condition, -1 means current */
}

/**********************************************************************
 * Note that this function allocates the name field of the mask struct.
 * FreeConditionMask must be called for the mask when the mask is discarded.
 * NOTE - exported via .h
 **********************************************************************/
void CreateConditionMask(char *flags, WindowConditionMask *mask)
{
  char *condition;
  char *prev_condition = NULL;
  char *tmp;

  if (flags == NULL)
    return;

  /* Next parse the flags in the string. */
  tmp = flags;
  tmp = GetNextSimpleOption(tmp, &condition);

  while (condition)
  {
    if (StrEquals(condition,"AcceptsFocus"))
    {
      mask->my_flags.do_accept_focus = 1;
      mask->my_flags.use_do_accept_focus = 1;
    }
    else if (StrEquals(condition,"!AcceptsFocus"))
    {
      mask->my_flags.do_accept_focus = 0;
      mask->my_flags.use_do_accept_focus = 1;
    }
    else if (StrEquals(condition,"Iconic"))
    {
      SET_ICONIFIED(mask, 1);
      SETM_ICONIFIED(mask, 1);
    }
    else if (StrEquals(condition,"!Iconic"))
    {
      SET_ICONIFIED(mask, 0);
      SETM_ICONIFIED(mask, 1);
    }
    else if (StrEquals(condition,"Visible"))
    {
      SET_PARTIALLY_VISIBLE(mask, 1);
      SETM_PARTIALLY_VISIBLE(mask, 1);
    }
    else if (StrEquals(condition,"!Visible"))
    {
      SET_PARTIALLY_VISIBLE(mask, 0);
      SETM_PARTIALLY_VISIBLE(mask, 1);
    }
    else if (StrEquals(condition,"PlacedByButton3"))
    {
      SET_PLACED_WB3(mask, 1);
      SETM_PLACED_WB3(mask, 1);
    }
    else if (StrEquals(condition,"!PlacedByButton3"))
    {
      SET_PLACED_WB3(mask, 0);
      SETM_PLACED_WB3(mask, 1);
    }
    else if (StrEquals(condition,"Raised"))
    {
      SET_FULLY_VISIBLE(mask, 1);
      SETM_FULLY_VISIBLE(mask, 1);
    }
    else if (StrEquals(condition,"!Raised"))
    {
      SET_FULLY_VISIBLE(mask, 0);
      SETM_FULLY_VISIBLE(mask, 1);
    }
    else if (StrEquals(condition,"Sticky"))
    {
      SET_STICKY(mask, 1);
      SETM_STICKY(mask, 1);
    }
    else if (StrEquals(condition,"!Sticky"))
    {
      SET_STICKY(mask, 0);
      SETM_STICKY(mask, 1);
    }
    else if (StrEquals(condition,"Maximized"))
    {
      SET_MAXIMIZED(mask, 1);
      SETM_MAXIMIZED(mask, 1);
    }
    else if (StrEquals(condition,"!Maximized"))
    {
      SET_MAXIMIZED(mask, 0);
      SETM_MAXIMIZED(mask, 1);
    }
    else if (StrEquals(condition,"Shaded"))
    {
      SET_SHADED(mask, 1);
      SETM_SHADED(mask, 1);
    }
    else if (StrEquals(condition,"!Shaded"))
    {
      SET_SHADED(mask, 0);
      SETM_SHADED(mask, 1);
    }
    else if (StrEquals(condition,"Transient"))
    {
      SET_TRANSIENT(mask, 1);
      SETM_TRANSIENT(mask, 1);
    }
    else if (StrEquals(condition,"!Transient"))
    {
      SET_TRANSIENT(mask, 0);
      SETM_TRANSIENT(mask, 1);
    }
    else if (StrEquals(condition,"PlacedByFvwm"))
    {
      SET_PLACED_BY_FVWM(mask, 1);
      SETM_PLACED_BY_FVWM(mask, 1);
    }
    else if (StrEquals(condition,"!PlacedByFvwm"))
    {
      SET_PLACED_BY_FVWM(mask, 0);
      SETM_PLACED_BY_FVWM(mask, 1);
    }
    else if (StrEquals(condition,"CurrentDesk"))
      mask->my_flags.needs_current_desk = 1;
    else if (StrEquals(condition,"CurrentPage"))
    {
      mask->my_flags.needs_current_desk = 1;
      mask->my_flags.needs_current_page = 1;
    }
    else if (StrEquals(condition,"CurrentGlobalPage"))
    {
      mask->my_flags.needs_current_desk = 1;
      mask->my_flags.needs_current_global_page = 1;
    }
    else if (StrEquals(condition,"CurrentPageAnyDesk") ||
	    StrEquals(condition,"CurrentScreen"))
      mask->my_flags.needs_current_page = 1;
    else if (StrEquals(condition,"CurrentGlobalPageAnyDesk"))
      mask->my_flags.needs_current_global_page = 1;
    else if (StrEquals(condition,"CirculateHit"))
      mask->my_flags.use_circulate_hit = 1;
    else if (StrEquals(condition,"CirculateHitIcon"))
      mask->my_flags.use_circulate_hit_icon = 1;
    else if (StrEquals(condition,"CirculateHitShaded"))
      mask->my_flags.use_circulate_hit_shaded = 1;
    else if (StrEquals(condition, "Layer"))
    {
       if (sscanf(tmp,"%d",&mask->layer))
       {
	  tmp = GetNextToken (tmp, &condition);
          if (mask->layer < 0)
            mask->layer = -2; /* silently ignore invalid layers */
       }
       else
       {
          mask->layer = -1; /* needs current layer */
       }
    }
    else if (!mask->my_flags.needs_name && !mask->my_flags.needs_not_name)
    {
      /* only 1st name to avoid mem leak */
      mask->name = condition;
      condition = NULL;
      if (mask->name[0] == '!')
      {
	mask->my_flags.needs_not_name = 1;
	mask->name++;
      }
      else
	mask->my_flags.needs_name = 1;
    }

    if (prev_condition)
      free(prev_condition);

    prev_condition = condition;
    tmp = GetNextSimpleOption(tmp, &condition);
  }

  if (prev_condition)
    free(prev_condition);
}

/**********************************************************************
 * Checks whether the given window matches the mask created with
 * CreateConditionMask.
 * NOTE - exported via .h
 **********************************************************************/
Bool MatchesConditionMask(FvwmWindow *fw, WindowConditionMask *mask)
{
  Bool fMatchesName;
  Bool fMatchesIconName;
  Bool fMatchesClass;
  Bool fMatchesResource;
  Bool fMatches;
  FvwmWindow *sf = get_focus_window();

  if (!blockcmpmask((char *)&(fw->flags), (char *)&(mask->flags),
                    (char *)&(mask->flag_mask), sizeof(fw->flags)))
    return 0;

  /*
  if ((mask->onFlags & fw->flags) != mask->onFlags)
    return 0;

  if ((mask->offFlags & fw->flags) != 0)
    return 0;
  */

  if (!mask->my_flags.use_circulate_hit && DO_SKIP_CIRCULATE(fw))
    return 0;

  if (!mask->my_flags.use_circulate_hit_icon && IS_ICONIFIED(fw) &&
      DO_SKIP_ICON_CIRCULATE(fw))
    return 0;

  if (!mask->my_flags.use_circulate_hit_shaded && IS_SHADED(fw) &&
      DO_SKIP_SHADED_CIRCULATE(fw))
    return 0;

  if (IS_ICONIFIED(fw) && IS_TRANSIENT(fw) && IS_ICONIFIED_BY_PARENT(fw))
    return 0;

  if (mask->my_flags.needs_current_desk && fw->Desk != Scr.CurrentDesk)
    return 0;
  if (mask->my_flags.needs_current_page)
  {
    if (FScreenIsEnabled())
    {
      if (!FScreenIsRectangleOnScreen(NULL, FSCREEN_CURRENT, &(fw->frame_g)))
	return 0;
    }
    else
    {
      if (!IsRectangleOnThisPage(&(fw->frame_g), Scr.CurrentDesk))
	return 0;
    }
  }
  else if (mask->my_flags.needs_current_global_page)
  {
    if (!IsRectangleOnThisPage(&(fw->frame_g), Scr.CurrentDesk))
      return 0;
  }

  /* Yes, I know this could be shorter, but it's hard to understand then */
  fMatchesName = matchWildcards(mask->name, fw->name);
  fMatchesIconName = matchWildcards(mask->name, fw->icon_name);
  fMatchesClass = (fw->class.res_class &&
		   matchWildcards(mask->name,fw->class.res_class));
  fMatchesResource = (fw->class.res_name &&
		      matchWildcards(mask->name, fw->class.res_name));
  fMatches = (fMatchesName || fMatchesIconName || fMatchesClass ||
	      fMatchesResource);

  if (mask->my_flags.needs_name && !fMatches)
    return 0;

  if (mask->my_flags.needs_not_name && fMatches)
    return 0;

  if ((mask->layer == -1) && (sf) && (fw->layer != sf->layer))
    return 0;

  if ((mask->layer >= 0) && (fw->layer != mask->layer))
    return 0;

  if (mask->my_flags.use_do_accept_focus)
  {
    Bool f;

    f = do_accept_input_focus(fw);
    if (fw && HAS_NEVER_FOCUS(fw))
    {
      f = 0;
    }
    else if (fw && IS_LENIENT(fw))
    {
      f = 1;
    }
    if (!f != !mask->my_flags.do_accept_focus)
    {
      return 0;
    }
  }

  return 1;
}

/**************************************************************************
 *
 * Direction = 1 ==> "Next" operation
 * Direction = -1 ==> "Previous" operation
 * Direction = 0 ==> operation on current window (returns pass or fail)
 *
 **************************************************************************/
static FvwmWindow *Circulate(char *action, int Direction, char **restofline)
{
  int pass = 0;
  FvwmWindow *fw, *found = NULL;
  FvwmWindow *sf;
  WindowConditionMask mask;
  char *flags;

  /* Create window mask */
  flags = CreateFlagString(action, restofline);
  DefaultConditionMask(&mask);
  if (Direction == 0)
  { /* override for Current [] */
    mask.my_flags.use_circulate_hit = 1;
    mask.my_flags.use_circulate_hit_icon = 1;
    mask.my_flags.use_circulate_hit_shaded = 1;
  }
  CreateConditionMask(flags, &mask);
  if (flags)
    free(flags);
  sf = get_focus_window();
  if (sf)
  {
    if (Direction > 0)
      fw = sf->prev;
    else if (Direction < 0)
      fw = sf->next;
    else
      fw = sf;
  }
  else
  {
    if (Direction == 0)
    {
      return NULL;
    }
    fw = NULL;
  }

  for (pass = 0; pass < 3 && !found; pass++)
  {
    while (fw && !found && fw != &Scr.FvwmRoot)
    {
#ifdef FVWM_DEBUG_MSGS
      fvwm_msg(DBG,"Circulate","Trying %s",fw->name);
#endif /* FVWM_DEBUG_MSGS */
      /* Make CirculateUp and CirculateDown take args. by Y.NOMURA */
      if (MatchesConditionMask(fw, &mask))
      {
	found = fw;
      }
      else
      {
	if (Direction > 0)
	  fw = fw->prev;
	else
	  fw = fw->next;
      }
      if (Direction == 0)
      {
	FreeConditionMask(&mask);
        return found;
      }
    }
    if (!fw || fw == &Scr.FvwmRoot)
    {
      if (Direction > 0)
      {
        /* Go to end of list */
	for (fw = &Scr.FvwmRoot; fw && fw->next; fw = fw->next)
	{
	  /* nop */
        }
      }
      else
      {
        /* Go to top of list */
        fw = Scr.FvwmRoot.next;
      }
    }
  }
  FreeConditionMask(&mask);

  return found;
}

void CMD_Prev(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, -1, &restofline);
  if(found && restofline)
  {
    old_execute_function(
      restofline, found, eventp, C_WINDOW, *Module, 0, NULL);
  }

}

void CMD_Next(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 1, &restofline);
  if(found && restofline)
  {
    old_execute_function(
      restofline, found, eventp, C_WINDOW, *Module, 0, NULL);
  }

}

void CMD_None(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 1, &restofline);
  if(!found && restofline)
  {
    old_execute_function(
      restofline, NULL, eventp, C_ROOT, *Module, 0, NULL);
  }
}

void CMD_Current(F_CMD_ARGS)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 0, &restofline);
  if(found && restofline)
  {
    old_execute_function(
      restofline, found, eventp, C_WINDOW, *Module, 0, NULL);
  }
}

void CMD_All(F_CMD_ARGS)
{
  FvwmWindow *t, **g;
  char *restofline;
  WindowConditionMask mask;
  char *flags;
  int num, i;

  flags = CreateFlagString(action, &restofline);
  DefaultConditionMask(&mask);
  CreateConditionMask(flags, &mask);
  if (flags)
    free(flags);
  mask.my_flags.use_circulate_hit = 1;
  mask.my_flags.use_circulate_hit_icon = 1;
  mask.my_flags.use_circulate_hit_shaded = 1;

  num = 0;
  for (t = Scr.FvwmRoot.next; t; t = t->next)
  {
    num++;
  }

  g = (FvwmWindow **) safemalloc (num * sizeof(FvwmWindow *));

  num = 0;
  for (t = Scr.FvwmRoot.next; t; t = t->next)
  {
    if (MatchesConditionMask(t, &mask))
    {
      g[num++] = t;
    }
  }

  for (i = 0; i < num; i++)
  {
    old_execute_function(
      restofline, g[i], eventp, C_WINDOW, *Module, 0, NULL);
  }

  free(g);
  FreeConditionMask(&mask);

  return;
}

static void GetDirectionReference(FvwmWindow *w, rectangle *r)
{
  if (IS_ICONIFIED(w))
    *r = w->icon_g;
  else
    *r = w->frame_g;
}

/**********************************************************************
 * Execute a function to the closest window in the given
 * direction.
 **********************************************************************/
void CMD_Direction(F_CMD_ARGS)
{
  static char *directions[] =
  {
    "North",
    "East",
    "South",
    "West",
    "NorthEast",
    "SouthEast",
    "SouthWest",
    "NorthWest",
    NULL
  };
  /* The rectangles are inteded for a future enhancement and are not used yet.
   */
  rectangle my_g;
  rectangle his_g;
  int my_cx;
  int my_cy;
  int his_cx;
  int his_cy;
  int offset = 0;
  int distance = 0;
  int score;
  int best_score;
  FvwmWindow *window;
  FvwmWindow *best_window;
  int dir;
  char *flags;
  char *restofline;
  char *tmp;
  WindowConditionMask mask;

  /* Parse the direction. */
  action = GetNextToken(action, &tmp);
  dir = GetTokenIndex(tmp, directions, 0, NULL);
  if (dir == -1)
  {
    fvwm_msg(ERR, "Direction", "Invalid direction %s", (tmp)? tmp : "");
    if (tmp)
      free(tmp);
    return;
  }
  if (tmp)
    free(tmp);

  /* Create the mask for flags */
  flags = CreateFlagString(action, &restofline);
  if (!restofline)
  {
    if (flags)
      free(flags);
    return;
  }
  DefaultConditionMask(&mask);
  CreateConditionMask(flags, &mask);
  if (flags)
    free(flags);

  /* If there is a focused window, use that as a starting point.
   * Otherwise we use the pointer as a starting point. */
  if (tmp_win)
  {
    GetDirectionReference(tmp_win, &my_g);
    my_cx = my_g.x + my_g.width / 2;
    my_cy = my_g.y + my_g.height / 2;
  }
  else
  {
    XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		  &my_g.x, &my_g.y, &JunkX, &JunkY, &JunkMask);
    my_g.width = 1;
    my_g.height = 1;
    my_cx = my_g.x;
    my_cy = my_g.y;
  }

  /* Next we iterate through all windows and choose the closest one in
   * the wanted direction.
   */
  best_window = NULL;
  best_score = -1;
  for (window = Scr.FvwmRoot.next; window; window = window->next)
  {
    /* Skip every window that does not match conditionals.
     * Skip also currently focused window.  That would be too close. :)
     */
    if (window == tmp_win || !MatchesConditionMask(window, &mask))
      continue;

    /* Calculate relative location of the window. */
    GetDirectionReference(window, &his_g);
    his_g.x -= my_cx;
    his_g.y -= my_cy;
    his_cx = his_g.x + his_g.width / 2;
    his_cy = his_g.y + his_g.height / 2;

    if (dir > 3)
    {
      int tx;
      /* Rotate the diagonals 45 degrees counterclockwise. To do this,
       * multiply the matrix /+h +h\ with the vector (x y).
       *                     \-h +h/
       * h = sqrt(0.5). We can set h := 1 since absolute distance doesn't
       * matter here. */
      tx = his_cx + his_cy;
      his_cy = -his_cx + his_cy;
      his_cx = tx;
    }
    /* Arrange so that distance and offset are positive in desired direction.
     */
    switch (dir)
    {
    case 0: /* N */
    case 2: /* S */
    case 4: /* NE */
    case 6: /* SW */
      offset = (his_cx < 0) ? -his_cx : his_cx;
      distance = (dir == 0 || dir == 4) ? -his_cy : his_cy;
      break;
    case 1: /* E */
    case 3: /* W */
    case 5: /* SE */
    case 7: /* NW */
      offset = (his_cy < 0) ? -his_cy : his_cy;
      distance = (dir == 3 || dir == 7) ? -his_cx : his_cx;
      break;
    }

    /* Target must be in given direction. */
    if (distance <= 0)
      continue;

    /* Calculate score for this window.  The smaller the better. */
    score = distance + offset;
    /* windows more than 45 degrees off the direction are heavily penalized
     * and will only be chosen if nothing else within a million pixels */
    if (offset > distance)
      score += 1000000;
    if (best_score == -1 || score < best_score)
    {
      best_window = window;
      best_score = score;
    }
  } /* for */

  if (best_window)
  {
    old_execute_function(
      restofline, best_window, eventp, C_WINDOW, *Module, 0,
      NULL);
  }

  FreeConditionMask(&mask);
}

/* A very simple function, but handy if you want to call
 * complex functions from root context without selecting a window
 * for every single function in it. */
void CMD_Pick(F_CMD_ARGS)
{
  char *restofline;
  char *flags;
  WindowConditionMask mask;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;

  flags = CreateFlagString(action, &restofline);
  DefaultConditionMask(&mask);
  mask.my_flags.use_circulate_hit = 1;
  mask.my_flags.use_circulate_hit_icon = 1;
  mask.my_flags.use_circulate_hit_shaded = 1;
  CreateConditionMask(flags, &mask);
  if (flags)
    free(flags);
  if (MatchesConditionMask(tmp_win, &mask) && restofline)
  {
    old_execute_function(
      restofline, tmp_win, eventp, C_WINDOW, *Module, 0, NULL);
  }
  FreeConditionMask(&mask);
}

void CMD_ThisWindow(F_CMD_ARGS)
{
	char *restofline;
	char *flags;
	WindowConditionMask mask;

	if (!tmp_win)
	{
		return;
	}

	flags = CreateFlagString(action, &restofline);
	DefaultConditionMask(&mask);
	mask.my_flags.use_circulate_hit = 1;
	mask.my_flags.use_circulate_hit_icon = 1;
	CreateConditionMask(flags, &mask);
	if (flags)
	{
		free(flags);
	}

	if (MatchesConditionMask(tmp_win, &mask) && restofline)
	{
		old_execute_function(
			restofline, tmp_win, eventp, C_WINDOW, *Module, 0,
			NULL);
	}

	FreeConditionMask(&mask);
	return;
}

void CMD_WindowId(F_CMD_ARGS)
{
  FvwmWindow *t;
  char *token;
  char *naction;
  unsigned long win;
  Bool use_condition = False;
  Bool use_screenroot = False;
  WindowConditionMask mask;
  char *flags, *restofline;

  /* Get window ID */
  action = GetNextToken(action, &token);

  if (token && StrEquals(token, "root"))
  {
    int screen = Scr.screen;

    free(token);
    token = PeekToken(action, &naction);
    if (!token || GetIntegerArguments(token, NULL, &screen, 1) != 1)
    {
      screen = Scr.screen;
    }
    else
    {
      action = naction;
    }
    use_screenroot = True;
    if (screen < 0 || screen >= Scr.NumberOfScreens)
      screen = 0;
    win = XRootWindow(dpy, screen);
    if (win == None)
      return;
  }
  else if (token)
  {
    /* SunOS doesn't have strtoul */
    win = (unsigned long)strtol(token, NULL, 0);
    free(token);
  }
  else
  {
    win = 0;
  }

  /* Look for condition - CreateFlagString returns NULL if no '(' or '[' */
  if (!use_screenroot)
  {
    flags = CreateFlagString(action, &restofline);
    if (flags)
    {
      /* Create window mask */
      use_condition = True;
      DefaultConditionMask(&mask);

      /* override for Current [] */
      mask.my_flags.use_circulate_hit = 1;
      mask.my_flags.use_circulate_hit_icon = 1;

      CreateConditionMask(flags, &mask);
      free(flags);

      /* Relocate action */
      action = restofline;
    }
  }

  /* Search windows */
  for (t = Scr.FvwmRoot.next; t; t = t->next)
  {
    if (t->w == win)
    {
      /* do it if no conditions or the conditions match */
      if (action && (!use_condition || MatchesConditionMask(t, &mask)))
        old_execute_function(
	  action, t, eventp, C_WINDOW, *Module, 0, NULL);
      break;
    }
  }
  if (!t && !use_condition && action)
  {
    exec_func_args_type efa;

    /* The window is not managed by fvwm. Still some functions may work on it.
     */
    memset(&efa, 0, sizeof(efa));
    efa.eventp = eventp;
    efa.win = win;
    efa.action = action;
    efa.context = C_UNMANAGED;
    efa.module = *Module;
    efa.flags.is_window_unmanaged = 1;
    execute_function(&efa);
  }

  /* Tidy up */
  if (use_condition)
  {
    FreeConditionMask(&mask);
  }
}
