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

#include <functional>
#include <map>
#include <vector>

#include <Core/Math.h>
#include <Core/RefCountedObject.h>
#include <ScriptBindings/ScriptManager.h>

// scriptarray.h must be included after ScriptManager.h or it won't be able to
// find angelscript.h
#include <AngelScript/addons/scriptarray.h>

namespace spades {
	class Serveritem;
	class PackageUpdateManager;
	namespace gui {
		class StartupScreen;
		class StartupScreenHelper : public RefCountedObject {
			friend class StartupScreen;
			StartupScreen *scr;
			std::vector<IntVector3> modes;
			struct ReportLine {
				std::string text;
				Vector4 color;
			};
			std::vector<ReportLine> reportLines;
			std::string report;
			void AddReport(const std::string &text = std::string(),
			               Vector4 color = Vector4::Make(1.f, 1.f, 1.f, 1.f));

			std::vector<std::string> openalDevices;
			struct LocaleInfo {
				std::string name;
				std::string descriptionNative;
				std::string descriptionEnglish;
			};
			std::vector<LocaleInfo> locales;

			std::multimap<std::string, std::function<std::string(std::string)>> incapableConfigs;

			bool shaderHighCapable;
			bool postFilterHighCapable;
			bool particleHighCapable;
			bool openGLCapable;

		protected:
			~StartupScreenHelper();

		public:
			StartupScreenHelper();
			void BindStartupScreen(StartupScreen *scr) { this->scr = scr; }
			void StartupScreenDestroyed();

			void ExamineSystem();

			int GetNumVideoModes();
			int GetVideoModeWidth(int index);
			int GetVideoModeHeight(int index);

			int GetNumAudioOpenALDevices();
			std::string GetAudioOpenALDevice(int index);

			int GetNumReportLines();
			std::string GetReport() { return report; }
			std::string GetReportLineText(int line);
			Vector4 GetReportLineColor(int line);

			int GetNumLocales();
			std::string GetLocale(int index);
			std::string GetLocaleDescriptionNative(int index);
			std::string GetLocaleDescriptionEnglish(int index);

			std::string CheckConfigCapability(const std::string &cfg, const std::string &value);

			PackageUpdateManager& GetPackageUpdateManager();
			bool OpenUpdateInfoURL();

			/** Checks each config value and modifies it if its value isn't feasible */
			void FixConfigs();

			std::string GetOperatingSystemType();
			bool BrowseUserDirectory();

			void Start();
		};
	}
}
