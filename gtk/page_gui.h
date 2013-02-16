/*
 *  gcml2 -- an implementation of Eric Raymond's CML2 in C
 *  Copyright (C) 2000-2001 Greg Banks
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#ifndef _page_gui_h_
#define _page_gui_h_

void page_gui_init(GtkWidget *notebook);
cml_node *page_gui_get_nth(int n);
cml_node *page_gui_get_current(void);
cml_node *page_gui_get_rootmost(void);
cml_node *page_gui_get_leafmost(void);
void page_gui_move_rootmost(void);
void page_gui_move_up(void);
void page_gui_move_down(void);
void page_gui_move_leafmost(void);
void page_gui_push(cml_node *mn);
void page_gui_update(cml_node *, gboolean make_current);
void page_gui_update_current(void);


#endif /* _page_gui_h_ */
