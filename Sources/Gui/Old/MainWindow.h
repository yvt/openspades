//
//  MainWindow.h
//  OpenSpades
//
//  Created by Tomoaki Kawada on 7/11/13.
//  Copyright (c) 2013 OpenSpades Developers
//

#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

class Fl_Button;
class Fl_Input;
class Fl_Label;
class Fl_Tabs;

namespace spades {
	namespace gui {
		class MainWindow: public Fl_Window {
			Fl_Input *hostEdit;
			
			
			Fl_Tabs *tabs;
			
			Fl_Group *recentTab;
			
			Fl_Group *browserTab;
			
			Fl_Group *configTab;
			Fl_Input *playerNameInput;
			
			static void OnStartButton(Fl_Widget *, void *mw);
			static void OnQuitButton(Fl_Widget *, void *mw);
			
			void StartGame();
		public:
			MainWindow();
			~MainWindow();
		};
	}
}
