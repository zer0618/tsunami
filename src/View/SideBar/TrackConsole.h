/*
 * TrackConsole.h
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#ifndef TRACKCONSOLE_H_
#define TRACKCONSOLE_H_


#include "SideBar.h"
class Track;
class Slider;

class TrackConsole: public SideBarConsole
{
public:
	TrackConsole(Session *session);
	virtual ~TrackConsole();

	void load_data();
	void update_strings();

	void on_name();
	void on_volume();
	void on_panning();
	void on_instrument();
	void on_edit_tuning();
	void on_select_synth();

	void on_edit_song();
	void on_edit_fx();
	void on_edit_curves();
	void on_edit_midi();
	void on_edit_midi_fx();
	void on_edit_synth();

	void set_track(Track *t);

	void on_view_cur_track_change();
	void on_update();

	Track *track;
	bool editing;
};

#endif /* TRACKCONSOLE_H_ */
