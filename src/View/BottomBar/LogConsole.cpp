/*
 * LogConsole.cpp
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#include "BottomBar.h"
#include "../../Stuff/Log.h"
#include "LogConsole.h"

LogConsole::LogConsole(Log *_log) :
	BottomBarConsole(_("Messages")),
	Observer("LogConsole")
{
	log = _log;

	fromResource("log_console");

	event("clear", std::bind(&LogConsole::onClear, this));

	HuiRunLater(0.5f, std::bind(&LogConsole::reload, this));

	subscribe(log);
}

LogConsole::~LogConsole()
{
	unsubscribe(log);
}

void LogConsole::onClear()
{
	log->clear();
}

void LogConsole::reload()
{
	reset("log_list");
	for (auto &m : log->messages){
		if (m.type == Log::TYPE_ERROR){
			addString("log_list", "hui:error\\" + m.text);
			((BottomBar*)parent)->open(BottomBar::LOG_CONSOLE);
		}else if (m.type == Log::TYPE_WARNING){
			addString("log_list", "hui:warning\\" + m.text);
			((BottomBar*)parent)->open(BottomBar::LOG_CONSOLE);
		}else{
			addString("log_list", "hui:info\\" + m.text);
		}
	}
}

void LogConsole::onUpdate(Observable *o, const string &message)
{
	reload();
}
