/*
 * PluginConsole.cpp
 *
 *  Created on: 23.08.2018
 *      Author: michi
 */

#include "PluginConsole.h"
#include "../../Session.h"
#include "../../Plugins/TsunamiPlugin.h"
#include "../../Plugins/PluginManager.h"
#include "../../Module/ConfigPanel.h"

class PluginPanel : public hui::Panel
{
public:
	PluginPanel(TsunamiPlugin *p, PluginConsole *_console)
	{
		addGrid("!width=380,noexpandx", 0, 0, "grid");
		setTarget("grid");
		addGrid("", 0, 0, "header-grid");
		setTarget("header-grid");
		addButton("", 0, 0, "close");
		setImage("close", "hui:close");
		setTooltip("close", _("stop plugin"));
		addLabel("!expandx,center,bold,big\\" + p->module_subtype, 1, 0, "label");
		plugin = p;
		console = _console;
		auto *c = p->create_panel();
		if (c)
			embed(c, "grid", 0, 1);

		event("close", [&]{ plugin->stop_request(); });
	}

	TsunamiPlugin *plugin;
	PluginConsole *console;

};

PluginConsole::PluginConsole(Session *s) :
	BottomBar::Console(_("Plugins"), s)
{
	addGrid("", 0, 0, "main-grid");
	setTarget("main-grid");
	addScroller("", 0, 0, "scroller");
	addButton("!expandy,noexpandx", 1, 0, "add");
	setImage("add", "hui:add");
	setTooltip("add", _("add plugin"));
	setTarget("scroller");
	addGrid("", 0, 0, "panel-grid");
	next_x = 0;

	event("add", [&]{ on_add_button(); });

	session->subscribe(this, [&]{ on_add_plugin(); }, session->MESSAGE_ADD_PLUGIN);
	session->subscribe(this, [&]{ on_remove_plugin(); }, session->MESSAGE_REMOVE_PLUGIN);
}

PluginConsole::~PluginConsole()
{
	session->unsubscribe(this);
	for (auto *p: panels)
		delete p;
}

void PluginConsole::on_add_button()
{
	string name = session->plugin_manager->ChooseModule(this, session, ModuleType::TSUNAMI_PLUGIN, "");
	if (name != "")
		session->executeTsunamiPlugin(name);
}

void PluginConsole::on_add_plugin()
{
	auto *p = new PluginPanel(session->last_plugin, this);
	embed(p, "panel-grid", next_x ++, 0);
	panels.add(p);
	blink();
}

void PluginConsole::on_remove_plugin()
{
	foreachi (auto *p, panels, i)
		if (p->plugin == session->last_plugin){
			delete p;
			panels.erase(i);
			break;
		}
}

