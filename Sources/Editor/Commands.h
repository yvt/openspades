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

#include <Core/RefCountedObject.h>
#include <vector>
#include <list>

namespace spades { namespace editor {

	class Command: public RefCountedObject {
	protected:
		~Command() { }
	public:
		Command() { }
		virtual void Perform() = 0;
		virtual void Revert() = 0;
		virtual std::string GetName() = 0;
	};
	
	class CompoundCommand: public Command {
		std::string name;
		std::vector<Handle<Command>> commands;
	protected:
		~CompoundCommand() { }
	public:
		CompoundCommand(const std::string& name,
						const std::vector<Handle<Command>>& cmd):
		commands(cmd), name(name) { }
		CompoundCommand(const std::string& name,
						std::vector<Handle<Command>>&& cmd):
		commands(cmd), name(name) { }
		void Perform() override;
		void Revert() override;
		std::string GetName() override {
			return name;
		}
	};
	
	class CommandManager: public RefCountedObject {
		std::list<Handle<Command>> history;
		decltype(history)::iterator nextIter;
	protected:
		~CommandManager();
	public:
		CommandManager();
		void Perform(Command *);
		
		bool CanUndo();
		bool CanRedo();
		
		std::string GetPreviousCommandName();
		std::string GetNextCommandName();
		
		void Undo();
		void Redo();
	};
	
} }
