/*
 * NewDialog.h
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#ifndef NEWDIALOG_H_
#define NEWDIALOG_H_


#include "../../lib/hui/hui.h"

class Song;
class Bar;

class NewDialog: public hui::Window
{
public:
	NewDialog(hui::Window *_parent);

	Bar *new_bar;

	void load_data();
	void apply_data();

	void on_beats();
	void on_complex();
	void on_pattern();
	void on_divisor();
	void on_ok();
	void on_metronome();
	void on_type_midi();
};

#endif /* NEWDIALOG_H_ */
