/*
 * MixingConsole.cpp
 *
 *  Created on: 16.03.2014
 *      Author: michi
 */

#include "MixingConsole.h"
#include "../../Data/AudioFile.h"
#include "../../Audio/AudioOutput.h"
#include "../Helper/PeakMeter.h"
#include <math.h>

const float TrackMixer::DB_MIN = -100;
const float TrackMixer::DB_MAX = 10;
const float TrackMixer::TAN_SCALE = 10.0f;

TrackMixer::TrackMixer()
{
	string id_grid = "mixing_track_table";
	id_separator = "mixing_track_separator";
	AddControlTable("", 0, 0, 1, 4, id_grid);
	SetTarget(id_grid, 0);
	id_name = "track_name";
	AddText("...", 0, 0, 0, 0, id_name);
	vol_slider_id = "volume";
	pan_slider_id = "panning";
	mute_id = "mute";
	AddSlider("!width=80,noorigin", 0, 1, 0, 0, pan_slider_id);
	AddString(pan_slider_id, "0\\L");
	AddString(pan_slider_id, "0.5\\");
	AddString(pan_slider_id, "1\\R");
	SetTooltip(pan_slider_id, "Balance");
	AddSlider("!vertical,expandy", 0, 2, 0, 0, vol_slider_id);
	AddString(vol_slider_id, format("%f\\%d", db2slider(DB_MAX), (int)DB_MAX));
	AddString(vol_slider_id, format("%f\\%d", db2slider(5), (int)5));
	AddString(vol_slider_id, format("%f\\%d", db2slider(0), 0));
	AddString(vol_slider_id, format("%f\\%d", db2slider(-5), (int)-5));
	AddString(vol_slider_id, format("%f\\%d", db2slider(-10), (int)-10));
	AddString(vol_slider_id, format("%f\\%d", db2slider(DB_MIN), (int)DB_MIN));
	SetTooltip(vol_slider_id, _("Lautst&arke in dB"));
	AddCheckBox("Stumm", 0, 3, 0, 0, mute_id);

	EventM(vol_slider_id, this, &TrackMixer::OnVolume);
	EventM(pan_slider_id, this, &TrackMixer::OnPanning);
	EventM(mute_id, this, &TrackMixer::OnMute);
}

TrackMixer::~TrackMixer()
{
}

float TrackMixer::slider2db(float val)
{
	return tan(atan(DB_MIN / TAN_SCALE) + val * (atan(DB_MAX / TAN_SCALE)- atan(DB_MIN / TAN_SCALE))) * TAN_SCALE;
}

float TrackMixer::db2slider(float db)
{
	return (atan(db / TAN_SCALE) - atan(DB_MIN / TAN_SCALE)) / (atan(DB_MAX / TAN_SCALE) - atan(DB_MIN / TAN_SCALE));
}

float TrackMixer::slider2vol(float val)
{
	return db2amplitude(slider2db(val));
}

float TrackMixer::vol2slider(float vol)
{
	return db2slider(amplitude2db(vol));
}

void TrackMixer::OnVolume()
{
	track->SetVolume(slider2vol(GetFloat("")));
}

void TrackMixer::OnMute()
{
	track->SetMuted(IsChecked(""));
}

void TrackMixer::OnPanning()
{
	track->SetPanning(GetFloat("")*2 - 1);
}

void TrackMixer::SetTrack(Track* t)
{
	track = t;
	Update();
}

void TrackMixer::Update()
{
	SetFloat(vol_slider_id, vol2slider(track->volume));
	SetFloat(pan_slider_id, track->panning * 0.5f + 0.5f);
	Check(mute_id, track->muted);
	SetString(id_name, track->name);
}


MixingConsole::MixingConsole(AudioFile *_audio, AudioOutput *_output, HuiWindow* _win, const string &id) :
	Observable("MixingConsole")
{
	win = _win;
	enabled = true;
	audio = _audio;
	output = _output;
	id_outer = id;
	id_inner = "mixing_inner_table";


	win->SetTarget(id_outer, 0);
	win->AddControlTable("", 0, 0, 1, 2, "mixing_console_button_grid");
	win->AddGroup(_("Mischpult"), 1, 0, 0, 0, "mixing_console_group");
	win->SetTarget("mixing_console_button_grid", 0);
	win->AddButton("!noexpandy", 0, 0, 0, 0, "mixing_console_close");
	win->SetImage("mixing_console_close", "hui:close");
	win->SetTarget("mixing_console_group", 0);
	win->AddControlTable("", 0, 0, 1, 20, id_inner);
	win->SetTarget(id_inner, 0);
	win->AddControlTable("", 0, 0, 1, 5, "mc_output");
	win->AddSeparator("!vertical", 1, 0, 0, 0, "");


	win->SetTarget("mc_output", 0);
	win->AddText(_("Ausgabe"), 0, 0, 0, 0, "");
	win->AddDrawingArea("!width=100,height=30,noexpandx,noexpandy", 0, 1, 0, 0, "mc_output_peaks");
	win->AddSlider("!vertical,expandy", 0, 2, 0, 0, "mc_output_volume");

	peak_meter = new PeakMeter(win, "mc_output_peaks", output);
	win->SetFloat("mc_output_volume", output->GetVolume());

	Show(false);

	win->EventM("mixing_console_close", this, &MixingConsole::OnClose);
	win->EventM("mc_output_volume", this, &MixingConsole::OnOutputVolume);

	Subscribe(audio);
	Subscribe(output);
}

MixingConsole::~MixingConsole()
{
	Unsubscribe(audio);
	Unsubscribe(output);
	foreach(TrackMixer *m, mixer)
		delete(m);
}

void MixingConsole::Show(bool show)
{
	enabled = show;
	win->HideControl("mixing_table", !enabled);
	Notify("Show");
}

void MixingConsole::OnClose()
{
	Show(false);
}

void MixingConsole::OnOutputVolume()
{
	output->SetVolume(win->GetFloat(""));
}

void MixingConsole::LoadData()
{
	for (int i=mixer.num; i<audio->track.num; i++){
		TrackMixer *m = new TrackMixer();
		mixer.add(m);
		win->Embed(m, id_inner, i*2 + 2, 0);
		win->AddSeparator("!vertical", i*2 + 3, 0, 1, 4, "separator_" + i2s(i));
	}
	for (int i=audio->track.num; i<mixer.num; i++){
		delete(mixer[i]);
		win->RemoveControl("separator_" + i2s(i));
	}
	mixer.resize(audio->track.num);

	foreachi(Track *t, audio->track, i)
		mixer[i]->SetTrack(t);
}

void MixingConsole::OnUpdate(Observable* o)
{
	if (o == output)
		win->SetFloat("mc_output_volume", output->GetVolume());
	else
		LoadData();
}
