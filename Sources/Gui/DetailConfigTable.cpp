//
//  DetailConfigTable.cpp
//  OpenSpades
//
//  Created by yvt on 8/5/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "DetailConfigTable.h"
#include <FL/Fl_Window.H>
#include "../Core/Settings.h"
#include <FL/fl_draw.H>

DetailConfigTable::DetailConfigTable(int X,int Y,int W,int H,const char* L) : Fl_Table(X,Y,W,H,L) {
	callback(&event_callback, (void*)this);
	when(FL_WHEN_NOT_CHANGED|when());
	// Create input widget that we'll use whenever user clicks on a cell
	input = new Fl_Input(W/2,H/2,0,0);
	input->hide();
	input->callback(input_cb, (void*)this);
	input->when(FL_WHEN_ENTER_KEY_ALWAYS);		// callback triggered when user hits Enter
	input->maximum_size(256);
	input->textsize(12);
	input->box(FL_THIN_UP_BOX);
	row_edit = col_edit = 0;
	s_left = s_top = s_right = s_bottom = 0;
	/*for (int c = 0; c < MAX_COLS; c++)
		for (int r = 0; r < MAX_ROWS; r++)
			values[r][c] = (r + 2) * (c + 3);		// initialize cells
	*/
	
	items = spades::Settings::GetInstance()->GetAllItemNames();
	
	row_header(0);
	rows(items.size());
	cols(2);
	
	col_width(0, 250);
	col_width(1, 300);
	row_height_all(20);
	
	end();
}

void DetailConfigTable::set_value_hide() {
	spades::Settings::ItemHandle item(items[row_edit]);
	
	std::string old = item;
	std::string newv = input->value();
	
	if(old != newv){
		item = newv;
	}
	
    input->hide();
    window()->cursor(FL_CURSOR_DEFAULT);		// XXX: if we don't do this, cursor can disappear!
}

void DetailConfigTable::done_editing() {
    if (input->visible()) {				// input widget visible, ie. edit in progress?
		set_value_hide();					// Transfer its current contents to cell and hide
    }
}

void DetailConfigTable::start_editing(int R, int C) {
	spades::Settings::ItemHandle item(items[R]);
    row_edit = R;					// Now editing this row/col
    col_edit = C;
    int X,Y,W,H;
    find_cell(CONTEXT_CELL, R,C, X,Y,W,H);		// Find X/Y/W/H of cell
    input->resize(X,Y,W,H);				// Move Fl_Input widget there
   
	std::string s = item;
	input->value(s.c_str());
    input->position(0,s.size());			// Select entire input field
    input->show();					// Show the input widget, now that we've positioned it
    input->take_focus();
}

void DetailConfigTable::event_callback2() {
	int R = callback_row();
	int C = callback_col();
	TableContext context = callback_context();
	
	switch ( context ) {
		case CONTEXT_CELL: {				// A table event occurred on a cell
			switch (Fl::event()) { 				// see what FLTK event caused it
				case FL_PUSH:					// mouse click?
					done_editing();				// finish editing previous
					if (C == 1)		// only edit value
						start_editing(R,C);				// start new edit
					return;
					/*
				case FL_KEYBOARD:				// key press in table?
					if ( Fl::event_key() == FL_Escape ) exit(0);	// ESC closes app
					if (C == 0) return;	// no editing column
					done_editing();				// finish any previous editing
					set_selection(R, C, R, C);			// select the current cell
					start_editing(R,C);				// start new edit
					if (Fl::event() == FL_KEYBOARD && Fl::e_text[0] != '\r') {
						input->handle(Fl::event());			// pass keypress to input widget
					}
					return;*/
			}
			return;
		}
			
		case CONTEXT_TABLE:					// A table event occurred on dead zone in table
		case CONTEXT_ROW_HEADER:				// A table event occurred on row/column header
		case CONTEXT_COL_HEADER:
			done_editing();					// done editing, hide
			return;
			
		default:
			return;
	}
}

void DetailConfigTable::draw_cell(TableContext context, int R,int C, int X,int Y,int W,int H) {
	//static char s[30];
	switch ( context ) {
		case CONTEXT_STARTPAGE:			// table about to redraw
			// Get kb nav + mouse 'selection region' for use below
			get_selection(s_top, s_left, s_bottom, s_right);
			break;
			
		case CONTEXT_COL_HEADER:			// table wants us to draw a column heading (C is column)
			fl_font(FL_HELVETICA | FL_BOLD, 12);	// set font for heading to bold
			fl_push_clip(X,Y,W,H);			// clip region for text
		{
			fl_draw_box(FL_THIN_UP_BOX, X,Y,W,H, col_header_color());
			fl_color(FL_FOREGROUND_COLOR);
			
			const char *str;
			if(C == 0){
				str = "Name";
			}else{
				str = "Value";
			}
			fl_draw(str, X,Y,W,H, FL_ALIGN_CENTER);
		}
			fl_pop_clip();
			return;
			
		case CONTEXT_ROW_HEADER:			// table wants us to draw a row heading (R is row)
		/*	fl_font(FL_HELVETICA | FL_BOLD, 14);	// set font for row heading to bold
			fl_push_clip(X,Y,W,H);
		{
			fl_draw_box(FL_THIN_UP_BOX, X,Y,W,H, row_header_color());
			fl_color(FL_BLACK);
			if (R == rows()-1) {			// Last row? Show 'Total'
				fl_draw("TOTAL", X,Y,W,H, FL_ALIGN_CENTER);
			} else {				// Not last row? show row#
				sprintf(s, "%d", R+1);
				fl_draw(s, X,Y,W,H, FL_ALIGN_CENTER);
			}
		}
			fl_pop_clip();*/
			return;
			
		case CONTEXT_CELL: {			// table wants us to draw a cell
			if (R == row_edit && C == col_edit && input->visible()) {
				return;					// dont draw for cell with input widget over it
			}
			// Background
			// Keyboard nav and mouse selection highlighting
			if ( C == 1 ) {
				fl_draw_box(FL_THIN_UP_BOX, X,Y,W,H, FL_BACKGROUND2_COLOR);
			} else { // header
				fl_draw_box(FL_THIN_UP_BOX, X,Y,W,H, FL_BACKGROUND_COLOR);
			}
			// Text
			fl_push_clip(X+3, Y+3, W-6, H-6);
			{
				fl_color(FL_FOREGROUND_COLOR);
				fl_font(FL_HELVETICA, 12);		// ..in regular font
				
				spades::Settings::ItemHandle item(items[R]);
				const char *str;
				if(C == 0){
					str = items[R].c_str();
				}else{
					str = item.CString();
				}
				fl_draw(str, X+3,Y+3,W-6,H-6, FL_ALIGN_LEFT);
			
			}
			fl_pop_clip();
			return;
		}
			
		case CONTEXT_RC_RESIZE: {			// table resizing rows or columns
			if (!input->visible()) return;
			find_cell(CONTEXT_TABLE, row_edit, col_edit, X, Y, W, H);
			if (X==input->x() && Y==input->y() && W==input->w() && H==input->h()) {
				return;					// no change? ignore
			}
			input->resize(X,Y,W,H);
			return;
		}
			
		default:
			return;
	}
}

