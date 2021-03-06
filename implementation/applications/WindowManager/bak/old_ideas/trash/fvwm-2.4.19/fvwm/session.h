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

/* File: session.h
 *
 * Description:
 *       exports from session.c shall go into this file
 *
 * Created:
 *       4 April 1999 - Steve Robbins <steve@nyongwa.montreal.qc.ca>
 */

#ifndef SESSION_H
#define SESSION_H


/*
**  Load and save the 'global', ie not window-related, state of FVWM
**  into a file.
*/
void LoadGlobalState(char *filename);

/*
**  Turn off SM for new windows
*/
void DisableRestoringState(void);

/*
**  Load and save window states.
*/
void LoadWindowStates (char *filename);

/*
** Save state to the named file, and if running under SM,
** make the SM properly restart fvwm.
*/
void RestartInSession (char *filename, Bool isNative, Bool doPreserveState);

/*
**  Fill in the FvwmWindow struct with information saved from
**  the last session. This expects the fields
**    t->w
**    t->name
**    t->class
**    t->tmpflags.NameChanged
**  to have meaningful values. The shade and maximize flags are set
**  if the window should start out as shaded or maximized, respecively.
**  The dimensions returned in x, y, w, h should be used when the
**  window is to be maximized.
 */
Bool MatchWinToSM(FvwmWindow *ewin, int *do_shade, int *do_max);

#ifdef SESSION
void SetClientID(char *client_id);

/*
**  Try to open a connection to the session manager. If non-NULL,
**  reuse the client_id.
 */
void SessionInit(void);

/*
**  The file number of the session manager connection or -1
**  if no session manager was found.
 */
extern int sm_fd;

/*
**  Process messages received from the session manager. Call this
**  from the main event loop when there is input waiting sm_fd.
 */
void ProcessICEMsgs(void);

#else

#define sm_fd -1
#define ProcessICEMsgs()
#define SessionInit()
#define SetClientID(client_id)

#endif

/*
 * Fvwm Function implementation
 */
Bool quitSession(void);
Bool saveSession(void);
Bool saveQuitSession(void);

#endif
