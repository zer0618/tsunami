/*
 * NewDialog.cpp
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#include "NewDialog.h"
#include "../../Data/base.h"
#include "../../Data/Song.h"
#include "../../Data/Rhythm/Bar.h"
#include "../../Action/ActionManager.h"
#include "../../Tsunami.h"
#include "../../TsunamiWindow.h"
#include "../../Session.h"

void set_bar_pattern(BarPattern &b, const string &pat);

NewDialog::NewDialog(hui::Window *_parent):
	hui::Window("new_dialog", _parent)
{
	add_string("sample_rate", "22050");
	add_string("sample_rate", i2s(DEFAULT_SAMPLE_RATE));
	add_string("sample_rate", "48000");
	add_string("sample_rate", "96000");
	set_int("sample_rate", 1);
	hide_control("nd_g_metronome_params", true);

	check("new_track_type:audio-mono", true);

	new_bar = new Bar(1000, 4, 1);
	set_int("num_bars", 32);
	set_int("beats", new_bar->beats.num);
	set_string("pattern", new_bar->pat_str());
	set_int("divisor", 0);

	event("cancel", [=]{ destroy(); });
	event("hui:close", [=]{ destroy(); });
	event("ok", [=]{ on_ok(); });
	event("metronome", [=]{ on_metronome(); });
	event("new_track_type:midi", [=]{ on_type_midi(); });
	event("beats", [=]{ on_beats(); });
	event("divisor", [=]{ on_divisor(); });
	event("pattern", [=]{ on_pattern(); });
	event("complex", [=]{ on_complex(); });
}

void NewDialog::on_ok()
{
	int sample_rate = get_string("sample_rate")._int();
	auto type = SignalType::AUDIO_MONO;
	if (is_checked("new_track_type:midi"))
		type = SignalType::MIDI;
	else if (is_checked("new_track_type:audio-stereo"))
		type = SignalType::AUDIO_STEREO;
	Session *session = tsunami->create_session();
	Song *song = session->song;
	song->sample_rate = sample_rate;
	song->action_manager->enable(false);
	if (is_checked("metronome")){
		song->add_track(SignalType::BEATS, 0);
		int count = get_int("num_bars");
		float bpm = get_float("beats_per_minute");
		new_bar->set_bpm(bpm, song->sample_rate);
		for (int i=0; i<count; i++)
			song->add_bar(-1, *new_bar, false);
	}
	song->add_track(type);

	song->add_tag("title", _("New Audio File"));
	song->add_tag("album", AppName);
	song->add_tag("artist", hui::Config.get_str("DefaultArtist", AppName));

	song->action_manager->enable(true);
	song->notify(song->MESSAGE_NEW);
	song->notify(song->MESSAGE_FINISHED_LOADING);
	destroy();
	session->win->show();
	session->win->activate("");
}

void NewDialog::on_beats()
{
	*new_bar = Bar(100, get_int(""), new_bar->divisor);
	set_string("pattern", new_bar->pat_str());
}

void NewDialog::on_divisor()
{
	new_bar->divisor = 1 << get_int("");
}

void NewDialog::on_pattern()
{
	set_bar_pattern(*new_bar, get_string("pattern"));
	set_int("beats", new_bar->beats.num);
}

void NewDialog::on_complex()
{
	bool complex = is_checked("complex");
	hide_control("beats", complex);
	hide_control("pattern", !complex);
}

void NewDialog::on_metronome()
{
	hide_control("nd_g_metronome_params", !is_checked(""));
}

void NewDialog::on_type_midi()
{
	check("metronome", true);
	hide_control("nd_g_metronome_params", false);
}

