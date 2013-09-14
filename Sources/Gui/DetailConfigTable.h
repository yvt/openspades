/*
 Copyright (c) 2013 yvt
 
 This file is part of OpenSpades.
 
 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.
 
 */

#pragma once

#include <FL/Fl_Table.H>
#include <FL/Fl_Input.H>
#include <vector>
#include <string>

class DetailConfigTable: public Fl_Table {
	std::vector<std::string> mAllItems;
	std::vector<std::string> mFilteredItems;
	std::string mFilter;

	Fl_Input *input;
	
	int row_edit, col_edit;				// row/col being modified
	int s_left, s_top, s_right, s_bottom;			// kb nav + mouse selection

	void done_editing();
	void set_value_hide();
	void start_editing(int R, int C);
	void event_callback2();				// table's event callback (instance)
	static void event_callback(Fl_Widget*, void *v) {	// table's event callback (static)
		((DetailConfigTable*)v)->event_callback2();
	}
	static void input_cb(Fl_Widget*, void* v) {		// input widget's callback
		((DetailConfigTable*)v)->set_value_hide();
	}
protected:
	
	void filterUpdated();
	virtual void draw_cell(TableContext context, int R,int C, int X,int Y,int W,int H);
public:
	DetailConfigTable(int X,int Y,int W,int H,const char* L=0);
	~DetailConfigTable() { }
	void setFilter( const char* newFilter );

	void EndEditing() { done_editing(); }
};
