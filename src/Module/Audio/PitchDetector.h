/*
 * PitchDetector.h
 *
 *  Created on: 13.09.2017
 *      Author: michi
 */

#ifndef SRC_MODULE_AUDIO_PITCHDETECTOR_H_
#define SRC_MODULE_AUDIO_PITCHDETECTOR_H_

#include "../Midi/MidiSource.h"

class Port;
class AudioBuffer;

class PitchDetector : public MidiSource
{
public:
	PitchDetector();
	virtual ~PitchDetector();

	void _cdecl __init__();
	void _cdecl __delete__() override;

	int _cdecl read(MidiEventBuffer &midi) override;

	void process(MidiEventBuffer &midi, AudioBuffer &buf);

	Port *source;

	float frequency, volume;
	bool loud_enough;
};

#endif /* SRC_MODULE_AUDIO_PITCHDETECTOR_H_ */
