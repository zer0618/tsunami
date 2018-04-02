/*
 * MidiSource.cpp
 *
 *  Created on: 02.04.2018
 *      Author: michi
 */

#include "MidiSource.h"
#include "../Rhythm/BeatSource.h"
#include "../Session.h"
#include "../Plugins/PluginManager.h"
#include "../Plugins/Plugin.h"


MidiSource::Output::Output(MidiSource *s)
{
	source = s;
}

int MidiSource::Output::read(MidiEventBuffer &midi)
{
	return source->read(midi);
}

void  MidiSource::Output::reset()
{
	source->reset();
}

MidiSource::MidiSource() :
	Configurable(Session::GLOBAL, Configurable::Type::MIDI_SOURCE)
{
	beat_source = BeatSource::dummy;
	out = new Output(this);
}

MidiSource::~MidiSource()
{
	delete out;
}

void MidiSource::__init__()
{
	new(this) MidiSource;
}

void MidiSource::__delete__()
{
	this->MidiSource::~MidiSource();
}

void MidiSource::set_beat_source(BeatSource *_beat_source)
{
	beat_source = _beat_source;
}




// TODO: move to PluginManager?
MidiSource *CreateMidiSource(Session *session, const string &name)
{
	Plugin *p = session->plugin_manager->GetPlugin(session, Plugin::Type::MIDI_SOURCE, name);
	MidiSource *s = NULL;
	if (p->usable)
		s = (MidiSource*)p->create_instance(session, "MidiSource");

	// dummy?
	if (!s)
		s = new MidiSource;

	s->name = name;
	s->plugin = p;
	s->usable = p->usable;
	s->song = session->song;
	s->session = session;
	s->reset_config();
	return s;
}

