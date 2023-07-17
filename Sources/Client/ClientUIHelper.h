/*
 Copyright (c) 2013 yvt
 Portion of the code is based on Serverbrowser.cpp.

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

namespace spades {
	namespace client {
		class ClientUI;
		class ClientUIHelper : public RefCountedObject {

			friend class ClientUI;

			ClientUI *ui;

		public:
			ClientUIHelper(ClientUI *);
			void ClientUIDestroyed();

			void SayGlobal(const std::string &);
			void SayTeam(const std::string &);

			void AlertNotice(const std::string &);
			void AlertWarning(const std::string &);
			void AlertError(const std::string &);
		};
	} // namespace client
} // namespace spades
