/*
 * ViewModeDefault.h
 *
 *  Created on: 12.11.2015
 *      Author: michi
 */

#ifndef SRC_VIEW_MODE_VIEWMODEDEFAULT_H_
#define SRC_VIEW_MODE_VIEWMODEDEFAULT_H_

#include "ViewMode.h"

class ActionTrackMoveSample;
class Range;

class ViewModeDefault : public ViewMode
{
public:
	ViewModeDefault(AudioView *view);
	virtual ~ViewModeDefault();

	virtual void onLeftButtonDown();
	virtual void onLeftButtonUp();
	virtual void onLeftDoubleClick();
	virtual void onRightButtonDown();
	virtual void onRightButtonUp();
	virtual void onMouseWheel();
	virtual void onMouseMove();
	virtual void onKeyDown(int k);
	virtual void onKeyUp(int k);
	virtual void updateTrackHeights();


	virtual void drawTrackBackground(Painter *c, AudioViewTrack *t);
	virtual void drawTrackData(Painter *c, AudioViewTrack *t);
	virtual void drawMidi(Painter *c, AudioViewTrack *t, const MidiData &midi, bool as_reference, int shift);

	virtual int which_midi_mode(Track *t);

	virtual SongSelection getSelectionForRange(const Range &r);
	virtual SongSelection getSelectionForRect(const Range &r, int y0, int y1);

	void selectUnderMouse();
	void setCursorPos(int pos);
	virtual Selection getHover();

	virtual void setBarriers(Selection &s);

	ActionTrackMoveSample *cur_action;
};

#endif /* SRC_VIEW_MODE_VIEWMODEDEFAULT_H_ */
