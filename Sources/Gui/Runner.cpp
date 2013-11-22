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

#include "Runner.h"
#include "View.h"
#include "SDLRunner.h"
#include "SDLAsyncRunner.h"
#include "ErrorDialog.h"
#include <Core/Settings.h>

SPADES_SETTING(cg_smp, "0");

namespace spades {
	namespace gui {
		Runner::Runner() {
		}
		Runner::~Runner() {
			
		}
		void Runner::RunProtected() {
			SPADES_MARK_FUNCTION();
			std::string err;
			try{
				Run();
			}catch(const spades::Exception& ex){
				err = ex.GetShortMessage();
				SPLog("Unhandled exception in SDLRunner:\n%s", ex.what());
			}catch(const std::exception& ex){
				err = ex.what();
				SPLog("Unhandled exception in SDLRunner:\n%s", ex.what());
			}
			if(!err.empty()){
				ErrorDialog dlg;
				dlg.set_modal();
				dlg.result = 0;
				
				// TODO: free this buffer (just leaking)
				Fl_Text_Buffer *buf = new Fl_Text_Buffer;
				buf->append(err.c_str());
				dlg.infoView->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
				dlg.infoView->buffer(buf);
				dlg.helpView->value("See SystemMessages.log for more details.");
				dlg.show();
				while(dlg.visible()){
					Fl::wait();
				}
			}
		}
		void Runner::Run() {
			SPADES_MARK_FUNCTION();
			class ConcreteRunner: public SDLRunner {
				Runner *r;
			protected:
				virtual View *CreateView(client::IRenderer *renderer, client::IAudioDevice *dev) {
					return r->CreateView(renderer, dev);
				}
			public:
				ConcreteRunner(Runner *r): r(r){ }
			};
			class ConcreteAsnycRunner: public SDLAsyncRunner {
				Runner *r;
			protected:
				virtual View *CreateView(client::IRenderer *renderer, client::IAudioDevice *dev) {
					return r->CreateView(renderer, dev);
				}
			public:
				ConcreteAsnycRunner(Runner *r): r(r){ }
			};
		
			if(cg_smp){
				ConcreteAsnycRunner r(this);
				r.Run();
			}else{
				ConcreteRunner r(this);
				r.Run();
			}
		
		}
	}
}
