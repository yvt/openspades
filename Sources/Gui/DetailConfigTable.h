//
//  DetailConfigTable.h
//  OpenSpades
//
//  Created by yvt on 8/5/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <FL/Fl_Table.H>
#include <FL/Fl_Input.H>
#include <vector>
#include <string>

class DetailConfigTable: public Fl_Table {
	std::vector<std::string> items;
	
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
	
	
	virtual void draw_cell(TableContext context, int R,int C, int X,int Y,int W,int H);
public:
	DetailConfigTable(int X,int Y,int W,int H,const char* L=0);
	~DetailConfigTable() { }
	
	void EndEditing() { done_editing(); }
};
