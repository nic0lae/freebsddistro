/* 
 * Oroborus Window Manager
 *
 * Copyright (C) 2001 Ken Lynch
 * Copyright (C) 2002-2005 Stefan Pfetzing
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/extensions/shape.h>
#include <X11/xpm.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#include "client.h"
#include "keyboard.h"
#include "frame.h"
#include "i18n.h"
#include "events.h"
#include "hints.h"
#include "misc.h"
#include "pixmap.h"

#include "globals.h"

int
frameLeft (client_t * c)
{
#ifdef DEBUG
  printf ("entering frameLeft\n");
#endif
  if (c->has_border)
    return sides[SIDE_LEFT][ACTIVE].width;
  else
    return 0;
}

int
frameRight (client_t * c)
{
#ifdef DEBUG
  printf ("entering frameRight\n");
#endif

  if (c->has_border)
    return sides[SIDE_RIGHT][ACTIVE].width;
  else
    return 0;
}

int
frameTop (client_t * c)
{
#ifdef DEBUG
  printf ("entering frameTop\n");
#endif

  if (c->has_border)
    return title[TITLE_3][ACTIVE].height;
  else
    return 0;
}

int
frameBottom (client_t * c)
{
#ifdef DEBUG
  printf ("entering frameBottom\n");
#endif

  if (c->has_border)
    return sides[SIDE_BOTTOM][ACTIVE].height;
  else
    return 0;
}

int
frameX (client_t * c)
{
#ifdef DEBUG
  printf ("entering frameX\n");
#endif

  if (c->has_border)
    return c->x - frameLeft (c);
  else
    return c->x;
}

int
frameY (client_t * c)
{
#ifdef DEBUG
  printf ("entering frameY\n");
#endif

  if (c->has_border)
    return c->y - frameTop (c);
  else
    return c->y;
}

int
frameWidth (client_t * c)
{
#ifdef DEBUG
  printf ("entering frameWidth\n");
#endif

  if (c->has_border)
    return c->width + frameLeft (c) + frameRight (c);
  else
    return c->width;
}

int
frameHeight (client_t * c)
{
#ifdef DEBUG
  printf ("entering frameHeight\n");
#endif

  if (c->has_border && c->win_state & WIN_STATE_SHADED)
    return frameTop (c) + frameBottom (c) - 1;
  else if (c->has_border)
    return c->height + frameTop (c) + frameBottom (c);
  else
    return c->height;
}

void
fillRectangle (Display * dpy, Drawable d, Pixmap pm, int x, int y,
	       int width, int height)
{
  XGCValues gv;
  GC gc;
  unsigned long mask;

#ifdef DEBUG
  printf ("entering fillRectangle\n");
#endif

  gv.fill_style = FillTiled;
  gv.tile = pm;
  gv.ts_x_origin = x;
  gv.ts_y_origin = y;
  gv.foreground = WhitePixel (dpy, DefaultScreen (dpy));
  if (gv.tile != None)
    mask = GCTile | GCFillStyle | GCTileStipXOrigin;
  else
    mask = GCForeground;
  gc = XCreateGC (dpy, d, mask, &gv);

  XFillRectangle (dpy, d, gc, x, y, width, height);
  XFreeGC (dpy, gc);
}

void
frameCreateTitlePixmap (client_t * c, int state, int left, int right,
			MyPixmap * pm)
{
  int width, x = 0, tp = 0, w1 = 0, w2, w3, w4, w5, temp;
  MyPixmap title1, title3, title5;

#ifdef DEBUG
  printf ("entering frameCreateTitlePixmap\n");
#endif

  if (left > right)
    {
      temp = left;
      left = right;
      right = temp;
    }

  width =
    frameWidth (c) - corners[CORNER_TOP_LEFT][ACTIVE].width -
    corners[CORNER_TOP_RIGHT][ACTIVE].width;
  if (left < corners[CORNER_TOP_LEFT][ACTIVE].width)
    left = corners[CORNER_TOP_LEFT][ACTIVE].width;
  if (right > frameWidth (c) - corners[CORNER_TOP_RIGHT][ACTIVE].width)
    right = frameWidth (c) - corners[CORNER_TOP_RIGHT][ACTIVE].width;
  if (right < corners[CORNER_TOP_LEFT][ACTIVE].width)
    right = corners[CORNER_TOP_LEFT][ACTIVE].width;

  left = left - corners[CORNER_TOP_LEFT][ACTIVE].width;
  right = right - corners[CORNER_TOP_LEFT][ACTIVE].width;

  w2 = title[TITLE_2][ACTIVE].width;
  w4 = title[TITLE_4][ACTIVE].width;
  if (full_width_title)
    {
      w1 = left;
      w5 = width - right;
      w3 = width - w1 - w2 - w4 - w5;
      if (w3 < 0)
	w3 = 0;
      switch (title_alignment)
	{
	case ALIGN_RIGHT:
	  tp = w3 - getTextWidth (c->name, &title_font);
	  break;
	case ALIGN_CENTER:
	  tp = (w3 / 2) - (getTextWidth (c->name, &title_font) / 2);
	  break;
	}
      if (tp < 0)
	tp = 0;
    }
  else
    {
      w3 = getTextWidth (c->name, &title_font);
      w5 = width;
      if (w3 > width - w2 - w4)
	w3 = width - w2 - w4;
      if (w3 < 0)
	w3 = 0;
      switch (title_alignment)
	{
	case ALIGN_LEFT:
	  w1 = left;
	  break;
	case ALIGN_RIGHT:
	  w1 = right - w2 - w3 - w4;
	  break;
	case ALIGN_CENTER:
	  w1 = left + ((right - left) / 2) - (w3 / 2) - w2;
	  break;
	}
      if (w1 < left)
	w1 = left;
    }

  pm->pixmap = XCreatePixmap (dpy, root, width, frameTop (c), depth);
  pm->mask = XCreatePixmap (dpy, pm->pixmap, width, frameTop (c), 1);

  if (w1 > 0)
    {
      scalePixmap (dpy, &title[TITLE_1][state], &title1, w1, frameTop (c));
      fillRectangle (dpy, pm->pixmap, title1.pixmap, 0, 0, w1, frameTop (c));
      fillRectangle (dpy, pm->mask, title1.mask, 0, 0, w1, frameTop (c));
      x = x + w1;
    }

  fillRectangle (dpy, pm->pixmap, title[TITLE_2][state].pixmap, x, 0, w2,
		 frameTop (c));
  fillRectangle (dpy, pm->mask, title[TITLE_2][state].mask, x, 0, w2,
		 frameTop (c));
  x = x + w2;

  if (w3 > 0)
    {
      scalePixmap (dpy, &title[TITLE_3][state], &title3, w3, frameTop (c));
      fillRectangle (dpy, pm->pixmap, title3.pixmap, x, 0, w3, frameTop (c));
      fillRectangle (dpy, pm->mask, title3.mask, x, 0, w3, frameTop (c));
      drawString (dpy, pm->pixmap, &title_font, title_gc[state], x + tp,
		  title_font.ascent + (frameTop (c) - title_font.height) / 2,
		  c->name);
      x = x + w3;
    }

  if (x > right - w4)
    x = right - w4;
  fillRectangle (dpy, pm->pixmap, title[TITLE_4][state].pixmap, x, 0, w4,
		 frameTop (c));
  fillRectangle (dpy, pm->mask, title[TITLE_4][state].mask, x, 0, w4,
		 frameTop (c));
  x = x + w4;

  if (w5 > 0)
    {
      scalePixmap (dpy, &title[TITLE_5][state], &title5, w5, frameTop (c));
      fillRectangle (dpy, pm->pixmap, title5.pixmap, x, 0, w5, frameTop (c));
      fillRectangle (dpy, pm->mask, title5.mask, x, 0, w5, frameTop (c));
    }
  freePixmap (dpy, &title1);
  freePixmap (dpy, &title3);
  freePixmap (dpy, &title5);
}

int
getButtonFromLetter (char c)
{
  int b;

#ifdef DEBUG
  printf ("entering getButtonFromLetter\n");
#endif

  switch (c)
    {
    case 'H':
      b = HIDE_BUTTON;
      break;
    case 'C':
      b = CLOSE_BUTTON;
      break;
    case 'M':
      b = MAXIMIZE_BUTTON;
      break;
    case 'S':
      b = SHADE_BUTTON;
      break;
    default:
      b = -1;
    }
  return b;
}

char
getLetterFromButton (int b)
{
  char c;

#ifdef DEBUG
  printf ("entering getLetterFromButton\n");
#endif

  switch (b)
    {
    case HIDE_BUTTON:
      c = 'H';
      break;
    case CLOSE_BUTTON:
      c = 'C';
      break;
    case MAXIMIZE_BUTTON:
      c = 'M';
      break;
    case SHADE_BUTTON:
      c = 'S';
      break;
    default:
      c = 0;
    }
  return c;
}

void
frameSetShape (client_t * c, int state, MyPixmap * title,
	       MyPixmap pm_sides[3], int button_x[BUTTON_ENUM_MAX])
{
  Window temp;
  int i;
  XRectangle rect;

#ifdef DEBUG
  printf ("entering frameSetShape\n");
  printf ("setting shape for client (%#lx)\n", c->window);
#endif

  if (!shape)
    return;

  temp =
    XCreateSimpleWindow (dpy, root, 0, 0, frameWidth (c), frameHeight (c),
			 0, 0, 0);

  if (c->win_state & WIN_STATE_SHADED)
    {
      rect.x = 0;
      rect.y = 0;
      rect.width = frameWidth (c);
      rect.height = frameHeight (c);
      XShapeCombineRectangles (dpy, temp, ShapeBounding, 0, 0, &rect, 1,
			       ShapeSubtract, 0);
    }
  else
    XShapeCombineShape (dpy, temp, ShapeBounding, frameLeft (c), frameTop (c),
			c->window, ShapeBounding, ShapeSet);
  if (c->has_border)
    {
      XShapeCombineMask (dpy, c->title, ShapeBounding, 0, 0,
			 title->mask, ShapeSet);
      for (i = 0; i < 3; i++)
	XShapeCombineMask (dpy, c->sides[i], ShapeBounding, 0, 0,
			   pm_sides[i].mask, ShapeSet);
      for (i = 0; i < 4; i++)
	XShapeCombineMask (dpy, c->corners[i], ShapeBounding, 0, 0,
			   corners[i][state].mask, ShapeSet);
      for (i = 0; i < BUTTON_ENUM_MAX; i++)
	{
	  if (c->button_pressed[i])
	    XShapeCombineMask (dpy, c->buttons[i], ShapeBounding,
			       0, 0, buttons[i][PRESSED].mask, ShapeSet);
	  else
	    XShapeCombineMask (dpy, c->buttons[i], ShapeBounding,
			       0, 0, buttons[i][state].mask, ShapeSet);
	}

      if (corners[CORNER_TOP_LEFT][ACTIVE].height >
	  frameHeight (c) - frameBottom (c) + 1)
	{
	  rect.x = 0;
	  rect.y = frameHeight (c) - frameBottom (c) + 1;
	  rect.width = corners[CORNER_TOP_LEFT][ACTIVE].width;
	  rect.height =
	    corners[CORNER_TOP_LEFT][ACTIVE].height - (frameHeight (c) -
						       frameBottom (c) + 1);
	  XShapeCombineRectangles (dpy, c->corners[CORNER_TOP_LEFT],
				   ShapeBounding, 0, 0, &rect, 1,
				   ShapeSubtract, 0);
	}
      if (corners[CORNER_TOP_RIGHT][ACTIVE].height >
	  frameHeight (c) - frameBottom (c) + 1)
	{
	  rect.x = 0;
	  rect.y = frameHeight (c) - frameBottom (c) + 1;
	  rect.width = corners[CORNER_TOP_RIGHT][ACTIVE].width;
	  rect.height =
	    corners[CORNER_TOP_RIGHT][ACTIVE].height - (frameHeight (c) -
							frameBottom (c) + 1);
	  XShapeCombineRectangles (dpy, c->corners[CORNER_TOP_RIGHT],
				   ShapeBounding, 0, 0, &rect, 1,
				   ShapeSubtract, 0);
	}
      if (corners[CORNER_BOTTOM_LEFT][ACTIVE].height >
	  frameHeight (c) - frameTop (c) + 1)
	{
	  rect.x = 0;
	  rect.y = 0;
	  rect.width = corners[CORNER_BOTTOM_LEFT][ACTIVE].width;
	  rect.height =
	    corners[CORNER_BOTTOM_LEFT][ACTIVE].height - (frameHeight (c) -
							  frameTop (c) + 1);
	  XShapeCombineRectangles (dpy, c->corners[CORNER_BOTTOM_LEFT],
				   ShapeBounding, 0, 0, &rect, 1,
				   ShapeSubtract, 0);
	}
      if (corners[CORNER_BOTTOM_RIGHT][ACTIVE].height >
	  frameHeight (c) - frameTop (c) + 1)
	{
	  rect.x = 0;
	  rect.y = 0;
	  rect.width = corners[CORNER_BOTTOM_RIGHT][ACTIVE].width;
	  rect.height =
	    corners[CORNER_BOTTOM_RIGHT][ACTIVE].height - (frameHeight (c) -
							   frameTop (c) + 1);
	  XShapeCombineRectangles (dpy, c->corners[CORNER_BOTTOM_RIGHT],
				   ShapeBounding, 0, 0, &rect, 1,
				   ShapeSubtract, 0);
	}

      XShapeCombineShape (dpy, temp, ShapeBounding, 0, frameTop (c),
			  c->sides[SIDE_LEFT], ShapeBounding, ShapeUnion);
      XShapeCombineShape (dpy, temp, ShapeBounding,
			  frameWidth (c) - frameRight (c), frameTop (c),
			  c->sides[SIDE_RIGHT], ShapeBounding, ShapeUnion);
      XShapeCombineShape (dpy, temp, ShapeBounding,
			  corners[CORNER_TOP_LEFT][ACTIVE].width, 0, c->title,
			  ShapeBounding, ShapeUnion);
      XShapeCombineShape (dpy, temp, ShapeBounding,
			  corners[CORNER_BOTTOM_LEFT][ACTIVE].width,
			  frameHeight (c) - frameBottom (c),
			  c->sides[SIDE_BOTTOM], ShapeBounding, ShapeUnion);
      XShapeCombineShape (dpy, temp, ShapeBounding, 0,
			  frameHeight (c) -
			  corners[CORNER_BOTTOM_LEFT][ACTIVE].height,
			  c->corners[CORNER_BOTTOM_LEFT], ShapeBounding,
			  ShapeUnion);
      XShapeCombineShape (dpy, temp, ShapeBounding,
			  frameWidth (c) -
			  corners[CORNER_BOTTOM_RIGHT][ACTIVE].width,
			  frameHeight (c) -
			  corners[CORNER_BOTTOM_RIGHT][ACTIVE].height,
			  c->corners[CORNER_BOTTOM_RIGHT], ShapeBounding,
			  ShapeUnion);
      XShapeCombineShape (dpy, temp, ShapeBounding, 0, 0,
			  c->corners[CORNER_TOP_LEFT], ShapeBounding,
			  ShapeUnion);
      XShapeCombineShape (dpy, temp, ShapeBounding,
			  frameWidth (c) -
			  corners[CORNER_TOP_RIGHT][ACTIVE].width, 0,
			  c->corners[CORNER_TOP_RIGHT], ShapeBounding,
			  ShapeUnion);
      for (i = 0; i < BUTTON_ENUM_MAX; i++)
	if (strchr (button_layout, getLetterFromButton (i)))
	  XShapeCombineShape (dpy, temp, ShapeBounding, button_x[i],
			      (frameTop (c) -
			       buttons[i][ACTIVE].height) / 2,
			      c->buttons[i], ShapeBounding, ShapeUnion);
    }
  XShapeCombineShape (dpy, c->frame, ShapeBounding, 0, 0, temp, ShapeBounding,
		      ShapeSet);
  XDestroyWindow (dpy, temp);
}

void
frameDraw (client_t * c)
{
  int state =
    ACTIVE, i, x, button, left, right, top_width,
    bottom_width, left_height, right_height, button_x[4];
  MyPixmap pm_title, pm_sides[3];

#ifdef DEBUG
  printf ("entering frameDraw\n");
#endif

  if (c != clientGetFocus ())
    state = INACTIVE;

  if (c->has_border)
    {
      XMapWindow (dpy, c->title);
      for (i = 0; i < 3; i++)
	XMapWindow (dpy, c->sides[i]);
      for (i = 0; i < 4; i++)
	XMapWindow (dpy, c->corners[i]);
      for (i = 0; i < 4; i++)
	{
	  if (strchr (button_layout, getLetterFromButton (i)))
	    XMapWindow (dpy, c->buttons[i]);
	  else
	    XUnmapWindow (dpy, c->buttons[i]);
	}

      x = frameLeft (c) - 1 + button_offset;
      for (i = 0; i < strlen (button_layout); i++)
	{
	  button = getButtonFromLetter (button_layout[i]);
	  if (button >= 0)
	    {
	      XMoveResizeWindow (dpy, c->buttons[button], x,
				 (frameTop (c) -
				  buttons[button][ACTIVE].height) / 2,
				 buttons[button][ACTIVE].width,
				 buttons[button][ACTIVE].height);
	      button_x[button] = x;
	      x = x + buttons[button][ACTIVE].width + button_spacing;
	    }
	  else
	    break;
	}
      left = x - button_spacing;

      x =
	frameWidth (c) - frameRight (c) + 1 + button_spacing - button_offset;
      for (i = strlen (button_layout) - 1; i >= 0; i--)
	{
	  button = getButtonFromLetter (button_layout[i]);
	  if (button >= 0)
	    {
	      x = x - buttons[button][ACTIVE].width - button_spacing;
	      XMoveResizeWindow (dpy, c->buttons[button], x,
				 (frameTop (c) -
				  buttons[button][ACTIVE].height) / 2,
				 buttons[button][ACTIVE].width,
				 buttons[button][ACTIVE].height);
	      button_x[button] = x;
	    }
	  else
	    break;
	}
      right = x;

      top_width =
	frameWidth (c) - corners[CORNER_TOP_LEFT][ACTIVE].width -
	corners[CORNER_TOP_RIGHT][ACTIVE].width;
      bottom_width =
	frameWidth (c) - corners[CORNER_BOTTOM_LEFT][ACTIVE].width -
	corners[CORNER_BOTTOM_RIGHT][ACTIVE].width;
      left_height =
	frameHeight (c) - frameTop (c) -
	corners[CORNER_BOTTOM_LEFT][ACTIVE].height;
      right_height =
	frameHeight (c) - frameTop (c) -
	corners[CORNER_BOTTOM_RIGHT][ACTIVE].height;

      frameCreateTitlePixmap (c, state, left, right, &pm_title);
      scalePixmap (dpy, &sides[SIDE_LEFT][state], &pm_sides[SIDE_LEFT],
		   frameLeft (c), left_height);
      scalePixmap (dpy, &sides[SIDE_RIGHT][state], &pm_sides[SIDE_RIGHT],
		   frameRight (c), right_height);
      scalePixmap (dpy, &sides[SIDE_BOTTOM][state], &pm_sides[SIDE_BOTTOM],
		   bottom_width, frameBottom (c));

      XSetWindowBackgroundPixmap (dpy, c->title, pm_title.pixmap);
      for (i = 0; i < 3; i++)
	XSetWindowBackgroundPixmap (dpy, c->sides[i], pm_sides[i].pixmap);
      for (i = 0; i < 4; i++)
	XSetWindowBackgroundPixmap (dpy, c->corners[i],
				    corners[i][state].pixmap);
      for (i = 0; i < BUTTON_ENUM_MAX; i++)
	{
	  if (c->button_pressed[i])
	    XSetWindowBackgroundPixmap (dpy, c->buttons[i],
					buttons[i][PRESSED].pixmap);
	  else
	    XSetWindowBackgroundPixmap (dpy, c->buttons[i],
					buttons[i][state].pixmap);
	}

      XMoveResizeWindow (dpy, c->title,
			 corners[CORNER_TOP_LEFT][ACTIVE].width, 0,
			 top_width < 1 ? 1 : top_width, frameTop (c));
      XMoveResizeWindow (dpy, c->sides[SIDE_BOTTOM],
			 corners[CORNER_BOTTOM_LEFT][ACTIVE].width,
			 frameHeight (c) - frameBottom (c),
			 bottom_width < 1 ? 1 : bottom_width,
			 frameBottom (c));
      XMoveResizeWindow (dpy, c->sides[SIDE_LEFT], 0, frameTop (c),
			 frameLeft (c), left_height < 1 ? 1 : left_height);
      XMoveResizeWindow (dpy, c->sides[SIDE_RIGHT],
			 frameWidth (c) - frameRight (c), frameTop (c),
			 frameRight (c), right_height < 1 ? 1 : right_height);

      XMoveResizeWindow (dpy, c->corners[CORNER_TOP_LEFT], 0, 0,
			 corners[CORNER_TOP_LEFT][ACTIVE].width,
			 corners[CORNER_TOP_LEFT][ACTIVE].height);
      XMoveResizeWindow (dpy, c->corners[CORNER_TOP_RIGHT],
			 frameWidth (c) -
			 corners[CORNER_TOP_RIGHT][ACTIVE].width, 0,
			 corners[CORNER_TOP_RIGHT][ACTIVE].width,
			 corners[CORNER_TOP_RIGHT][ACTIVE].height);
      XMoveResizeWindow (dpy, c->corners[CORNER_BOTTOM_LEFT], 0,
			 frameHeight (c) -
			 corners[CORNER_BOTTOM_LEFT][ACTIVE].height,
			 corners[CORNER_BOTTOM_LEFT][ACTIVE].width,
			 corners[CORNER_BOTTOM_LEFT][ACTIVE].height);
      XMoveResizeWindow (dpy, c->corners[CORNER_BOTTOM_RIGHT],
			 frameWidth (c) -
			 corners[CORNER_BOTTOM_RIGHT][ACTIVE].width,
			 frameHeight (c) -
			 corners[CORNER_BOTTOM_RIGHT][ACTIVE].height,
			 corners[CORNER_BOTTOM_RIGHT][ACTIVE].width,
			 corners[CORNER_BOTTOM_RIGHT][ACTIVE].height);

      XClearWindow (dpy, c->title);
      for (i = 0; i < 3; i++)
	XClearWindow (dpy, c->sides[i]);
      for (i = 0; i < 4; i++)
	XClearWindow (dpy, c->corners[i]);
      for (i = 0; i < BUTTON_ENUM_MAX; i++)
	XClearWindow (dpy, c->buttons[i]);
      frameSetShape (c, state, &pm_title, pm_sides, button_x);
      freePixmap (dpy, &pm_title);
      for (i = 0; i < 3; i++)
	freePixmap (dpy, &pm_sides[i]);
    }
  else
    {
      for (i = 0; i < 3; i++)
	XUnmapWindow (dpy, c->sides[i]);
      for (i = 0; i < 4; i++)
	XUnmapWindow (dpy, c->corners[i]);
      for (i = 0; i < BUTTON_ENUM_MAX; i++)
	XUnmapWindow (dpy, c->buttons[i]);
      frameSetShape (c, 0, NULL, NULL, NULL);
    }
}

/***This must remain at the end of the file.***********************************************
 * vi:set sw=2 cindent cinoptions={1s,>2s,^-1s,n-1s foldmethod=marker foldmarker=���,���: *
 * arch-tag: frame handling for oroborus-wm                                               *
 ******************************************************************************************/
