/*
 Copyright (c) 2013 OpenSpades Developers
 
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

#include <Core/RefCountedObject.h>
#include <vector>
#include <Core/Mutex.h>
#include <ScriptBindings/ScriptManager.h>
#include <AngelScript/addons/scriptarray.h>
#include <Core/Math.h>
#include <functional>
#include <map>

namespace spades {
	class Serveritem;
	namespace gui {
		class StartupScreen;
		class StartupScreenHelper: public RefCountedObject {
			friend class StartupScreen;
			StartupScreen *scr;
			std::vector<IntVector3> modes;
			struct ReportLine {
				std::string text;
				Vector4 color;
			};
			std::vector<ReportLine> reportLines;
			std::string report;
			void AddReport(const std::string& text = std::string(), Vector4 color = Vector4::Make(1.f, 1.f, 1.f, 1.f));
			
			std::multimap<std::string, std::function<std::string(std::string)> > incapableConfigs;
			
			bool shaderHighCapable;
			bool postFilterHighCapable;
			bool particleHighCapable;
			bool openGLCapable;
		protected:
			virtual ~StartupScreenHelper();
		public:
			StartupScreenHelper();
			void BindStartupScreen(StartupScreen *scr) {
				this->scr = scr;
			}
			void StartupScreenDestroyed();
			
			void ExamineSystem();
			
			int GetNumVideoModes();
			int GetVideoModeWidth(int index);
			int GetVideoModeHeight(int index);
			
			int GetNumReportLines();
			std::string GetReport() { return report; }
			std::string GetReportLineText(int line);
			Vector4 GetReportLineColor(int line);
			
			std::string CheckConfigCapability(const std::string& cfg, const std::string& value);
			
			void Start();
		};
	}
}
