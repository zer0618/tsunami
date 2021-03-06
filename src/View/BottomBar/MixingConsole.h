/*
 * MixingConsole.h
 *
 *  Created on: 16.03.2014
 *      Author: michi
 */

#ifndef SRC_VIEW_BOTTOMBAR_MIXINGCONSOLE_H_
#define SRC_VIEW_BOTTOMBAR_MIXINGCONSOLE_H_


#include "BottomBar.h"



enum class MixerMode
{
	VOLUME,
	EFFECTS,
	MIDI_EFFECTS
};

class PeakMeterDisplay;
class DeviceManager;

class TrackMixer;
class Module;
class ModulePanel;

class MixingConsole: public BottomBar::Console
{
public:
	MixingConsole(Session *session);
	virtual ~MixingConsole();

	void load_data();

	void on_output_volume();

	void on_tracks_change();
	void update_all();
	void on_update_device_manager();
	void on_chain_state_change();

	void _cdecl on_show() override;
	void _cdecl on_hide() override;

	DeviceManager *device_manager;
	PeakMeterDisplay *peak_meter;
	PeakMeterDisplay *spectrum_meter;

	string id_inner;
	Array<TrackMixer*> mixer;

	int peak_runner_id;

	void set_mode(MixerMode mode);
	MixerMode mode;

	void select_module(Module *m);
	Module *selected_module;
	ModulePanel *config_panel;
};

#endif /* SRC_VIEW_BOTTOMBAR_MIXINGCONSOLE_H_ */
