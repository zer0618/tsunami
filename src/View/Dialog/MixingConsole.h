/*
 * MixingConsole.h
 *
 *  Created on: 16.03.2014
 *      Author: michi
 */

#ifndef MIXINGCONSOLE_H_
#define MIXINGCONSOLE_H_


#include "../../lib/hui/hui.h"
#include "../../Stuff/Observer.h"
class Track;
class Slider;
class AudioFile;
class MixingConsole;
class PeakMeter;
class AudioOutput;

class TrackMixer: public HuiPanel
{
public:
	TrackMixer();
	~TrackMixer();
	void OnVolume();
	void OnMute();
	void OnPanning();
	void SetTrack(Track *t);
	void Update();

	static const float DB_MIN;
	static const float DB_MAX;
	static const float TAN_SCALE;
	static float db2slider(float db);
	static float slider2db(float val);
	static float vol2slider(float vol);
	static float slider2vol(float val);

	Track *track;
	//Slider *volume_slider;
	//Slider *panning_slider;
	string id_name;
	string vol_slider_id;
	string pan_slider_id;
	string mute_id;
	string id_separator;
};

class MixingConsole: public Observer, public Observable
{
public:
	MixingConsole(AudioFile *audio, AudioOutput *output, HuiWindow *win, const string &id);
	virtual ~MixingConsole();

	void LoadData();
	void Show(bool show);

	void OnClose();
	void OnOutputVolume();

	virtual void OnUpdate(Observable *o);

	HuiWindow *win;
	AudioFile *audio;
	AudioOutput *output;
	PeakMeter *peak_meter;

	string id_outer;
	string id_inner;
	Array<TrackMixer*> mixer;
	bool enabled;
};

#endif /* MIXINGCONSOLE_H_ */
