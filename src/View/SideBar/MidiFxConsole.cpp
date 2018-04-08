/*
 * MidiFxConsole.cpp
 *
 *  Created on: 09.09.2014
 *      Author: michi
 */

#include "MidiFxConsole.h"
#include "../../Data/Track.h"
#include "../../Data/Midi/MidiData.h"
#include "../../Module/Synth/Synthesizer.h"
#include "../../Module/Midi/MidiEffect.h"
#include "../../Module/ConfigPanel.h"
#include "../AudioView.h"
#include "../../Plugins/PluginManager.h"
#include "../../Session.h"


class SingleMidiFxPanel : public hui::Panel
{
public:
	SingleMidiFxPanel(Session *_session, Track *t, MidiEffect *_fx, int _index)
	{
		session = _session;
		song = session->song;
		track = t;
		fx = _fx;
		index = _index;

		fromResource("fx_panel");

		setString("name", fx->module_subtype);
		p = fx->create_panel();
		if (p){
			embed(p, "grid", 0, 1);
			p->update();
		}else{
			setTarget("grid");
			addLabel(_("not configurable"), 0, 1, "");
			hideControl("load_favorite", true);
			hideControl("save_favorite", true);
		}

		event("enabled", std::bind(&SingleMidiFxPanel::onEnabled, this));
		event("delete", std::bind(&SingleMidiFxPanel::onDelete, this));
		event("load_favorite", std::bind(&SingleMidiFxPanel::onLoad, this));
		event("save_favorite", std::bind(&SingleMidiFxPanel::onSave, this));

		check("enabled", fx->enabled);

		old_param = fx->config_to_string();
		fx->subscribe(this, std::bind(&SingleMidiFxPanel::onFxChange, this), fx->MESSAGE_CHANGE);
		fx->subscribe(this, std::bind(&SingleMidiFxPanel::onFxChangeByAction, this), fx->MESSAGE_CHANGE_BY_ACTION);
	}
	virtual ~SingleMidiFxPanel()
	{
		fx->unsubscribe(this);
	}
	void onLoad()
	{
		string name = session->plugin_manager->SelectFavoriteName(win, fx, false);
		if (name.num == 0)
			return;
		session->plugin_manager->ApplyFavorite(fx, name);
		if (track)
			track->editMidiEffect(index, old_param);
		old_param = fx->config_to_string();
	}
	void onSave()
	{
		string name = session->plugin_manager->SelectFavoriteName(win, fx, true);
		if (name.num == 0)
			return;
		session->plugin_manager->SaveFavorite(fx, name);
	}
	void onEnabled()
	{
		if (track)
			track->enableMidiEffect(index, isChecked(""));
	}
	void onDelete()
	{
		if (track)
			track->deleteMidiEffect(index);
	}
	void onFxChange()
	{
		if (track)
			track->editMidiEffect(index, old_param);
		check("enabled", fx->enabled);
		p->update();
		old_param = fx->config_to_string();
	}
	void onFxChangeByAction()
	{
		check("enabled", fx->enabled);
		p->update();
		old_param = fx->config_to_string();
	}
	Session *session;
	Song *song;
	Track *track;
	MidiEffect *fx;
	ConfigPanel *p;
	string old_param;
	int index;
};

MidiFxConsole::MidiFxConsole(Session *session) :
	SideBarConsole(_("Midi Fx"), session)
{
	fromResource("midi_fx_editor");

	id_inner = "midi_fx_inner_table";

	track = NULL;
	//Enable("add", false);
	enable("track_name", false);

	event("add", std::bind(&MidiFxConsole::onAdd, this));

	event("edit_song", std::bind(&MidiFxConsole::onEditSong, this));
	event("edit_track", std::bind(&MidiFxConsole::onEditTrack, this));
	event("edit_midi", std::bind(&MidiFxConsole::onEditMidi, this));

	view->subscribe(this, std::bind(&MidiFxConsole::onViewCurTrackChange, this), view->MESSAGE_CUR_TRACK_CHANGE);
	update();
}

MidiFxConsole::~MidiFxConsole()
{
	clear();
	view->unsubscribe(this);
	song->unsubscribe(this);
}

void MidiFxConsole::update()
{
	bool allow = false;
	if (view->cur_track)
		allow = (view->cur_track->type == Track::Type::MIDI);
	hideControl("me_grid_yes", !allow);
	hideControl("me_grid_no", allow);
	hideControl(id_inner, !allow);
}

void MidiFxConsole::onViewCurTrackChange()
{
	update();
	setTrack(view->cur_track);
}

void MidiFxConsole::onTrackDelete()
{
	update();
	setTrack(NULL);
}

void MidiFxConsole::onUpdate()
{
	update();
	setTrack(track);
}

void MidiFxConsole::onAdd()
{
	string name = session->plugin_manager->ChooseModule(win, session, Module::Type::AUDIO_EFFECT);
	MidiEffect *effect = CreateMidiEffect(session, name);
	if (track)
		track->addMidiEffect(effect);
}

void MidiFxConsole::clear()
{
	if (track)
		track->unsubscribe(this);
	foreachi(hui::Panel *p, panels, i){
		delete(p);
		removeControl("separator_" + i2s(i));
	}
	panels.clear();
	track = NULL;
	//Enable("add", false);
}

void MidiFxConsole::onEditSong()
{
	bar()->open(SideBar::SONG_CONSOLE);
}

void MidiFxConsole::onEditTrack()
{
	bar()->open(SideBar::TRACK_CONSOLE);
}

void MidiFxConsole::onEditMidi()
{
	bar()->open(SideBar::MIDI_EDITOR_CONSOLE);
}

void MidiFxConsole::setTrack(Track *t)
{
	clear();
	track = t;
	if (track){
		track->subscribe(this, std::bind(&MidiFxConsole::onTrackDelete, this), track->MESSAGE_DELETE);
		track->subscribe(this, std::bind(&MidiFxConsole::onUpdate, this), track->MESSAGE_ADD_MIDI_EFFECT);
		track->subscribe(this, std::bind(&MidiFxConsole::onUpdate, this), track->MESSAGE_DELETE_MIDI_EFFECT);
	}


	if (track){
		foreachi(MidiEffect *e, track->midi.fx, i){
			panels.add(new SingleMidiFxPanel(session, track, e, i));
			embed(panels.back(), id_inner, 0, i*2 + 3);
			addSeparator("!horizontal", 0, i*2 + 4, "separator_" + i2s(i));
		}
		hideControl("comment_no_fx", track->midi.fx.num > 0);
	}else{
		hideControl("comment_no_fx", false);
	}
	//Enable("add", track);
}

