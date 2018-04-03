/*
 * MidiEffect.h
 *
 *  Created on: 10.09.2014
 *      Author: michi
 */

#ifndef MIDIEFFECT_H_
#define MIDIEFFECT_H_


#include "../lib/base/base.h"
#include "../Data/Range.h"
#include "../Module/Module.h"

class Plugin;
class Track;
class AudioBuffer;
class MidiNoteBuffer;
class SongSelection;
class Song;

namespace Script{
class Script;
class Type;
};

class MidiEffect : public Module
{
public:
	MidiEffect();
	virtual ~MidiEffect();

	void _cdecl __init__();
	virtual void _cdecl __delete__();

	bool only_on_selection;
	Range range;

	virtual void _cdecl process(MidiNoteBuffer *midi){};

	void process_track(Track *t, const SongSelection &sel);

	void prepare();
	void apply(MidiNoteBuffer &midi, Track *t, bool log_error);

	int bh_offset;
	void note(float pitch, float volume, int beats);
	void note_x(float pitch, float volume, int beats, int sub_beats, int beat_partition);
	void skip(int beats);
	void skip_x(int beats, int sub_beats, int beat_partition);
	Song *bh_song;
	MidiNoteBuffer *bh_midi;
};

MidiEffect *_cdecl CreateMidiEffect(Session *session, const string &name);

#endif /* MIDIEFFECT_H_ */
