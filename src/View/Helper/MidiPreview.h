/*
 * MidiPreview.h
 *
 *  Created on: 22.07.2018
 *      Author: michi
 */

#ifndef SRC_VIEW_HELPER_MIDIPREVIEW_H_
#define SRC_VIEW_HELPER_MIDIPREVIEW_H_

#include "../../lib/base/base.h"

class AudioOutput;
class Synthesizer;
class MidiPreviewSource;
class Module;
class SignalChain;
class Session;


class MidiPreview : public VirtualBase
{
public:
	MidiPreview(Session *s);
	virtual ~MidiPreview();

	SignalChain *chain;
	Module *synth;
	Module *out;
	MidiPreviewSource *source;
	Session *session;


	void start(Synthesizer *s, const Array<int> &pitch, float volume, float ttl);
	void end();
};

#endif /* SRC_VIEW_HELPER_MIDIPREVIEW_H_ */
