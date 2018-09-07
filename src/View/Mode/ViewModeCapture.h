/*
 * ViewModeCapture.h
 *
 *  Created on: 14.02.2016
 *      Author: michi
 */

#ifndef SRC_VIEW_MODE_VIEWMODECAPTURE_H_
#define SRC_VIEW_MODE_VIEWMODECAPTURE_H_

#include "ViewModeDefault.h"
#include "../../Data/Range.h"
#include "../../lib/base/base.h"

class InputStreamAudio;
class InputStreamMidi;

//class CaptureConsoleMode;

class ViewModeCapture : public ViewModeDefault
{
public:
	ViewModeCapture(AudioView *view);
	virtual ~ViewModeCapture();

	Selection get_hover() override;
	void on_left_button_down() override {}
	void on_left_double_click() override {}

	void draw_post(Painter *c) override;


	virtual Set<Track*> prevent_playback() override;

	//CaptureConsoleMode *console_mode;

	InputStreamAudio *input_audio;
	InputStreamMidi *input_midi;
	void set_input_audio(InputStreamAudio *input);
	void set_input_midi(InputStreamMidi *input);
	Track *capturing_track;

	void on_input_update();
};

#endif /* SRC_VIEW_MODE_VIEWMODECAPTURE_H_ */
