/*
 * ActionTrackAddMidiEffect.cpp
 *
 *  Created on: 23.09.2014
 *      Author: michi
 */

#include "ActionTrackAddMidiEffect.h"
#include "../../../Data/Track.h"
#include "../../../Module/Midi/MidiEffect.h"

ActionTrackAddMidiEffect::ActionTrackAddMidiEffect(Track *t, MidiEffect *_effect)
{
	track = t;
	effect = _effect;
}

ActionTrackAddMidiEffect::~ActionTrackAddMidiEffect()
{
	if (effect)
		delete effect;
}

void *ActionTrackAddMidiEffect::execute(Data *d)
{
	track->midi_fx.add(effect);
	track->notify(track->MESSAGE_ADD_MIDI_EFFECT);
	effect = nullptr;

	return nullptr;
}

void ActionTrackAddMidiEffect::undo(Data *d)
{
	effect = track->midi_fx.pop();
	effect->Observable::notify(effect->MESSAGE_DELETE);
	track->notify(track->MESSAGE_DELETE_MIDI_EFFECT);
}

