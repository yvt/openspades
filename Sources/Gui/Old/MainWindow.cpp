//
//  MainWindow.cpp
//  OpenSpades
//
//  Created by Tomoaki Kawada on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "MainWindow.h"
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include "SDLRunner.h"
#include <exception>
#include <FL/fl_ask.H>
#include "../Core/Debug.h"
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Tabs.H>

namespace spades {
	namespace gui {
		MainWindow::MainWindow():
		Fl_Window(640, 350, "OpenSpades") {
			SPADES_MARK_FUNCTION();
			
			begin();
			
			hostEdit = new Fl_Input(120, 3, w() - 120 - 106, 24, "Quick Launch:");
			hostEdit->value("192.168.24.24");
			
			Fl_Button *startButton;
			startButton = new Fl_Return_Button(w() - 103, 3, 100, 24, "Start");
			startButton->user_data(this);
			startButton->callback(OnStartButton);
			
			
			tabs = new Fl_Tabs(0, 30, w(), h() - 30);
			
			tabs->begin();
			
			int cliX, cliY, cliW, cliH;
			tabs->client_area(cliX, cliY, cliW, cliH);
			
			// recent servers
			recentTab = new Fl_Group(cliX, cliY, cliW, cliH, "Recent");
			///tabs->push(recentTab);
			
			recentTab->begin();
			
			new Fl_Input(80, 80, 100, 20, "test");
			
			recentTab->end();
			
			
			// server browser
			browserTab = new Fl_Group(cliX, cliY, cliW, cliH, "Servers");
			//tabs->push(browserTab);
			
			browserTab->begin();
			
			browserTab->end();
			
			// config page
			configTab = new Fl_Group(cliX, cliY, cliW, cliH, "Config");
			configTab->begin();
			playerNameInput = new Fl_Input(cliX + 120, cliY + 10, 200, 24, "Player Name:");
			playerNameInput->value("tcpip (testing)");
			configTab->end();
			
			tabs->end();
			
			end();
		}
		
		MainWindow::~MainWindow(){
			SPADES_MARK_FUNCTION();
			
		}
		
		void MainWindow::StartGame() {
			SPADES_MARK_FUNCTION();
			
			hide();
			
			std::string host = hostEdit->value();
#if 0
			SDLRunner r(host);
			r.Run();
#else
			
			try{
				SDLRunner r(host, playerNameInput->value());
				r.Run();
			}catch(const std::exception& ex){
				puts("-------- UNHANDLED EXCEPTION --------");
				puts(ex.what());
				fl_message("Error occured:\n\n%s", ex.what());
			}
#endif
		}
		
		void MainWindow::OnStartButton(Fl_Widget *, void *mw){
			SPADES_MARK_FUNCTION();
			
			MainWindow *self = (MainWindow *)mw;
			self->StartGame();
			
		}
		
		void MainWindow::OnQuitButton(Fl_Widget *, void *mw) {
			SPADES_MARK_FUNCTION();
			
			MainWindow *self = (MainWindow *)mw;
			self->hide();
		}
	}
}
