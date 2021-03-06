/*
 * Tsunami.h
 *
 *  Created on: 13.08.2014
 *      Author: michi
 */

#ifndef TSUNAMI_H_
#define TSUNAMI_H_

#include "lib/hui/hui.h"

extern const string AppName;
extern const string AppVersion;


class Song;
class PluginManager;
class Slider;
class Log;
class AudioInput;
class DeviceManager;
class AudioView;
class Storage;
class Clipboard;
class TsunamiWindow;
class Session;
class PerformanceMonitor;

class Tsunami : public hui::Application
{
public:
	Tsunami();
	virtual ~Tsunami();

	bool on_startup(const Array<string> &arg) override;
	void on_end() override;

	bool handle_arguments(Array<string> &arg);
	void load_key_codes();

	Session* create_session(const string &chain_filename = "");
	bool allow_termination();

	Array<Session*> sessions;

	Log *log;

	DeviceManager *device_manager;
	PluginManager *plugin_manager;
	Clipboard *clipboard;
	PerformanceMonitor *perf_mon;
};


extern Tsunami *tsunami;

#endif /* TSUNAMI_H_ */
