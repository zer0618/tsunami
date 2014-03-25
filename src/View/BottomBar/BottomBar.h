/*
 * BottomBar.h
 *
 *  Created on: 23.03.2014
 *      Author: michi
 */

#ifndef BOTTOMBAR_H_
#define BOTTOMBAR_H_

#include "../../lib/hui/hui.h"
#include "../../Stuff/Observable.h"

class AudioFile;
class AudioOutput;
class Track;
class FxConsole;
class MixingConsole;
class LogDialog;
class SampleManager;
class Log;

class BottomBarConsole : public HuiPanel
{
public:
	BottomBarConsole(const string &_title)
	{ title = _title; }
	string title;
};

class BottomBar : public HuiPanel, public Observable
{
public:
	BottomBar(AudioFile *audio, AudioOutput *output, Log *log);
	virtual ~BottomBar();

	void OnClose();
	void OnNext();
	void OnPrevious();
	virtual void OnShow();
	virtual void OnHide();

	enum
	{
		MIXING_CONSOLE,
		FX_CONSOLE,
		SAMPLE_CONSOLE,
		LOG_CONSOLE,
		NUM_CONSOLES
	};

	void SetTrack(Track *t);
	void Choose(int console);
	bool IsActive(int console);
	int active_console;
	bool visible;

	FxConsole *fx_console;
	MixingConsole *mixing_console;
	LogDialog *log_dialog;
	SampleManager *sample_manager;
};

#endif /* BOTTOMBAR_H_ */