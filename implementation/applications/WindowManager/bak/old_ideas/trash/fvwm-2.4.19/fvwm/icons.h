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

#ifndef _ICONS_
#define _ICONS_

#ifdef NO_ICONS
#define ICON_HEIGHT(t) 1
#else
#define ICON_HEIGHT(t) ((t)->icon_font.height + 6)
#endif

int get_visible_icon_window_count(FvwmWindow *tmp_win);
void AutoPlaceIcon(FvwmWindow *);
void RedoIconName(FvwmWindow *);
void DrawIconWindow(FvwmWindow *);
void CreateIconWindow(FvwmWindow *tmp_win, int def_x, int def_y);
void GetIconWindow(FvwmWindow *tmp_win);
void GetIconBitmap(FvwmWindow *tmp_win);
void Iconify(FvwmWindow *, int, int);
void DeIconify(FvwmWindow *);
void SetMapStateProp(FvwmWindow *, int);


#endif /* _ICONS_ */
