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
#include "SDLRunner.h"
#include "View.h"
#include <Core/Exception.h>
#include <Core/Settings.h>
#include <Core/Strings.h>

DEFINE_SPADES_SETTING(r_videoWidth, "1024");
DEFINE_SPADES_SETTING(r_videoHeight, "640");

namespace spades {
	namespace gui {
		Runner::Runner() : m_videoWidth(r_videoWidth), m_videoHeight(r_videoHeight) {
			if (m_videoWidth < 1 || m_videoWidth > 16384)
				SPRaise("Value of r_videoWidth is invalid.");
			if (m_videoHeight < 1 || m_videoHeight > 16384)
				SPRaise("Value of r_videoHeight is invalid.");
		}
		Runner::~Runner() {}

		void Runner::RunProtected() {
			SPADES_MARK_FUNCTION();
			std::string err;
			try {
				Run();
			} catch (const spades::Exception &ex) {
				err = ex.GetShortMessage();
				SPLog("[!] Unhandled exception in SDLRunner:\n%s", ex.what());
			} catch (const std::exception &ex) {
				err = ex.what();
				SPLog("[!] Unhandled exception in SDLRunner:\n%s", ex.what());
			}
			if (!err.empty()) {

				std::string msg = err;
				msg = _Tr("Main", "A serious error caused OpenSpades to stop "
				                  "working:\n\n{0}\n\nSee SystemMessages.log for more details.",
				          msg);

				SDL_InitSubSystem(SDL_INIT_VIDEO);
				if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
				                             _Tr("Main", "OpenSpades Fatal Error").c_str(),
				                             msg.c_str(), nullptr)) {
					// showing dialog failed.
					// TODO: do appropriate action
				}
			}
		}
		void Runner::Run() {
			SPADES_MARK_FUNCTION();
			class ConcreteRunner : public SDLRunner {
				Runner *r;

			protected:
				View *CreateView(client::IRenderer *renderer, client::IAudioDevice *dev) override {
					return r->CreateView(renderer, dev);
				}

			public:
				ConcreteRunner(Runner *r) : r(r) {}
			};

			ConcreteRunner r(this);
			r.Run(m_videoWidth, m_videoHeight);
		}

		void Runner::OverrideVideoSize(int width, int height) {
			if (width < 1 || width > 16384)
				SPInvalidArgument("width");
			if (height < 1 || height > 16384)
				SPInvalidArgument("height");
			m_videoWidth = width;
			m_videoHeight = height;
		}
	}
}
