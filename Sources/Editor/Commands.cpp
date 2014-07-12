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

#include "Commands.h"

namespace spades { namespace editor {
	
	void CompoundCommand::Perform() {
		for (auto it = commands.begin();
			 it != commands.end(); ++it)
			(*it)->Perform();
	}
	
	void CompoundCommand::Revert() {
		for (auto it = commands.rbegin();
			 it != commands.rend(); ++it)
			(*it)->Revert();
	}
	
	CommandManager::CommandManager():
	history(),
	nextIter(history.end()) {
		
	}
	
	CommandManager::~CommandManager() {
		
	}
	
	void CommandManager::Perform(Command *cmd) {
		SPAssert(cmd);
		
		cmd->Perform();
		
		history.erase(nextIter, history.end());
		history.push_back(cmd);
		nextIter = history.end();
	}
	
	bool CommandManager::CanUndo() {
		return nextIter != history.begin();
	}
	bool CommandManager::CanRedo() {
		return nextIter != history.end();
	}
	
	std::string CommandManager::GetPreviousCommandName() {
		if (!CanUndo()) return std::string();
		auto it = nextIter; --it;
		return (*it)->GetName();
	}
	
	std::string CommandManager::GetNextCommandName() {
		if (!CanRedo()) return std::string();
		return (*nextIter)->GetName();
	}
	
	void CommandManager::Undo() {
		if (CanUndo()) {
			auto it = nextIter; --it;
			(*it)->Revert();
			nextIter = it;
		}
	}
	
	void CommandManager::Redo() {
		if (CanRedo()) {
			(*nextIter)->Perform();
			++nextIter;
		}
	}
	
} }

