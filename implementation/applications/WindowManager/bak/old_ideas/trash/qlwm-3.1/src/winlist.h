/* winlist.h */

#ifndef WINLIST_H
#define WINLIST_H

#include "xwindow.h"

class winlist : public QPushButton
{
	Q_OBJECT
	
	QPopupMenu *wmenu;
	QPixmap winlistpix,tiledpix;
	void popup_list(void);    // popup window list
	bool isobscured(xwindow *, Window *, uint);
	bool isbottom(xwindow *, Window *, uint);

private slots:
	void highlight_pager(int);

protected:
	virtual void mousePressEvent(QMouseEvent *);
	virtual void mouseReleaseEvent(QMouseEvent *);

public:
	void start_popup(void);
	void hidden_win(void);
	void set_pixmap(void);
	winlist(QWidget *parent, const char *name);
};
#endif
