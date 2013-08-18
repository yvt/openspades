//
//  DetailConfigWindowHandler.cpp
//  OpenSpades
//
//  Created by yvt on 8/5/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "DetailConfigWindow.h"
#include "../Core/Settings.h"
#include <vector>
#include <string>

void DetailConfigWindow::Init() {
	std::vector<std::string> items;
	items = spades::Settings::GetInstance()->GetAllItemNames();
	
}

