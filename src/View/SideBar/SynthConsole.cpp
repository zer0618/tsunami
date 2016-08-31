/*
 * SynthConsole.cpp
 *
 *  Created on: 13.04.2014
 *      Author: michi
 */

#include "SynthConsole.h"
#include "../AudioView.h"
#include "../Dialog/ConfigurableSelectorDialog.h"
#include "../Dialog/DetuneSynthesizerDialog.h"
#include "../../Data/Track.h"
#include "../../Audio/Synth/Synthesizer.h"
#include "../../Plugins/ConfigPanel.h"
#include "../../Plugins/PluginManager.h"
#include "../../Tsunami.h"
#include "../../TsunamiWindow.h"



class SynthPanel : public HuiPanel, public Observer
{
public:
	SynthPanel(Track *t) :
		Observer("SynthPanel")
	{
		track = t;
		synth = t->synth;
		fromResource("synth_panel");
		setString("name", synth->name);
		p = synth->createPanel();
		if (p){
			embed(p, "grid", 0, 1);
			p->update();
		}else{
			setTarget("grid", 0);
			addLabel(_("not configurable"), 0, 1, 0, 0, "");
			hideControl("load_favorite", true);
			hideControl("save_favorite", true);
		}

		event("load_favorite", this, &SynthPanel::onLoad);
		event("save_favorite", this, &SynthPanel::onSave);

		old_param = synth->configToString();
		subscribe(synth, synth->MESSAGE_CHANGE);
		subscribe(synth, synth->MESSAGE_CHANGE_BY_ACTION);
	}
	virtual ~SynthPanel()
	{
		unsubscribe(synth);
	}
	void onLoad()
	{
		string name = tsunami->plugin_manager->SelectFavoriteName(win, (Configurable*)synth, false);
		if (name.num == 0)
			return;
		tsunami->plugin_manager->ApplyFavorite(synth, name);
		track->editSynthesizer(old_param);
		old_param = synth->configToString();
	}
	void onSave()
	{
		string name = tsunami->plugin_manager->SelectFavoriteName(win, synth, true);
		if (name.num == 0)
			return;
		tsunami->plugin_manager->SaveFavorite(synth, name);
	}
	virtual void onUpdate(Observable *o, const string &message)
	{
		if (message == o->MESSAGE_CHANGE){
			track->editSynthesizer(old_param);
		}
		if (p)
			p->update();
		old_param = synth->configToString();
	}
	Track *track;
	Synthesizer *synth;
	ConfigPanel *p;
	string old_param;
};

SynthConsole::SynthConsole(AudioView *_view) :
	SideBarConsole(_("Synthesizer")),
	Observer("SynthConsole")
{
	view = _view;
	id_inner = "grid";

	fromResource("synth_console");

	event("select", this, &SynthConsole::onSelect);
	event("detune", this, &SynthConsole::onDetune);

	event("edit_song", this, &SynthConsole::onEditSong);
	event("edit_track", this, &SynthConsole::onEditTrack);

	track = NULL;
	panel = NULL;

	subscribe(view, view->MESSAGE_CUR_TRACK_CHANGE);
}

SynthConsole::~SynthConsole()
{
	unsubscribe(view);
	if (track){
		unsubscribe(track);
		if (track->synth)
			unsubscribe(track->synth);
	}
}

void SynthConsole::onSelect()
{
	if (!track)
		return;
	Synthesizer *s = ChooseSynthesizer(win, track->song, track->synth->name);
	if (s)
		track->setSynthesizer(s);
}

void SynthConsole::onDetune()
{
	HuiDialog *dlg = new DetuneSynthesizerDialog(track->synth, track, view, win);
	dlg->show();
}

void SynthConsole::onEditSong()
{
	tsunami->win->side_bar->open(SideBar::SONG_CONSOLE);
}

void SynthConsole::onEditTrack()
{
	tsunami->win->side_bar->open(SideBar::TRACK_CONSOLE);
}

void SynthConsole::clear()
{
	if (track){
		unsubscribe(track);
		if (track->synth){
			unsubscribe(track->synth);
			delete(panel);
			panel = NULL;
			removeControl("separator_0");
		}
	}
	track = NULL;
}

void SynthConsole::setTrack(Track *t)
{
	clear();
	track = t;
	if (!track)
		return;

	subscribe(track, track->MESSAGE_DELETE);
	subscribe(track, track->MESSAGE_CHANGE);

	if (track->synth){
		subscribe(track->synth, track->synth->MESSAGE_DELETE);
		panel = new SynthPanel(track);
		embed(panel, id_inner, 0, 0);
		addSeparator("!horizontal", 0, 1, 0, 0, "separator_0");
	}
}

void SynthConsole::onUpdate(Observable* o, const string &message)
{
	if ((o->getName() == "Synthesizer") and (message == o->MESSAGE_DELETE)){
		clear();
	}else if ((o == track) and (message == track->MESSAGE_DELETE)){
		setTrack(NULL);
	}else if ((o == view) and (message == view->MESSAGE_CUR_TRACK_CHANGE))
		setTrack(view->cur_track);
	else
		setTrack(track);
}

