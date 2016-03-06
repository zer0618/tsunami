/*
 * SampleRef.h
 *
 *  Created on: 29.05.2015
 *      Author: michi
 */

#ifndef SAMPLEREF_H_
#define SAMPLEREF_H_

#include "../Stuff/Observable.h"
#include "../lib/math/rect.h"

class Song;
class Track;
class Sample;
class MidiNoteData;
class BufferBox;
class Range;

class SampleRef : public Observable
{
public:
	SampleRef(Sample *sample);
	virtual ~SampleRef();
	void _cdecl __init__(Sample *sample);
	virtual void _cdecl __delete__();

	static const string MESSAGE_CHANGE_BY_ACTION;

	Track *getParent();
	Range getRange();

	int get_index();

	int pos;
	Sample *origin;
	BufferBox *buf;
	MidiNoteData *midi;
	bool muted;
	float volume;

	// repetition
	int rep_num;
	int rep_delay;

	// editing
	rect area;
	int track_no;
	Song *owner;
};

#endif /* SAMPLEREF_H_ */
