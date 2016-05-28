/*
 * MidiSource.h
 *
 *  Created on: 29.04.2016
 *      Author: michi
 */

#ifndef SRC_MIDI_MIDISOURCE_H_
#define SRC_MIDI_MIDISOURCE_H_

#include "MidiData.h"

class MidiSource : public VirtualBase
{
public:
	MidiSource();
	virtual ~MidiSource(){}

	void _cdecl __init__();
	virtual void _cdecl __delete__();

	virtual int read(MidiRawData &midi){ return 0; };
};

class MidiDataSource : public MidiSource
{
public:
	MidiDataSource(const MidiRawData &midi);
	virtual ~MidiDataSource();

	virtual int read(MidiRawData &midi);

	void setData(const MidiRawData &midi);

	void seek(int pos);

	MidiRawData midi;
	int offset;
};

#endif /* SRC_MIDI_MIDISOURCE_H_ */