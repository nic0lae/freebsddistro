/* 
 * Oroborus Window Manager
 *
 * Copyright (C) 2005 Stefan Pfetzing
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

#ifndef __FRAME_H__
#define __FRAME_H__

int frameLeft (client_t *);
int frameRight (client_t *);
int frameTop (client_t *);
int frameBottom (client_t *);
int frameX (client_t *);
int frameY (client_t *);
int frameWidth (client_t *);
int frameHeight (client_t *);
void frameDraw (client_t *);


#endif /* __FRAME_H_ */

/**This must remain at the end of the file.**********
 * vim600:set sw=2 ts=8 fdm=marker fmr=���,���:     *
 * vim600:set cindent cinoptions={1s,>2s,^-1s,n-1s: *
 ****************************************************/

