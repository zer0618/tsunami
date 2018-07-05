/*
 * Selection.h
 *
 *  Created on: 12.11.2015
 *      Author: michi
 */

#ifndef SRC_VIEW_SELECTION_H_
#define SRC_VIEW_SELECTION_H_

#include "../lib/base/base.h"
#include "../Data/Range.h"

class AudioViewTrack;
class AudioViewLayer;
class Track;
class TrackLayer;
class SampleRef;
class MidiNote;
class TrackMarker;
class Bar;
class SongSelection;

class Selection
{
public:
	int type;
	AudioViewTrack *vtrack;
	AudioViewLayer *vlayer;
	Track *track;
	TrackLayer *layer;
	SampleRef *sample;
	MidiNote *note;
	TrackMarker *marker;
	Bar *bar;
	int pos;
	Range range;
	int y0;
	int y1;
	int sample_offset;
	Array<int> barrier;
	int pitch;
	int clef_position, modifier;
	int index;

	enum Type
	{
		NONE,
		BACKGROUND,
		SELECTION_START,
		SELECTION_END,
		PLAYBACK,
		TIME,
		//TRACK,
		TRACK_HEADER,
		TRACK_BUTTON_MUTE,
		TRACK_BUTTON_SOLO,
		TRACK_BUTTON_EDIT,
		TRACK_BUTTON_CURVE,
		TRACK_BUTTON_FX,
		LAYER,
		LAYER_HEADER,
		SAMPLE,
		MIDI_NOTE,
		MIDI_PITCH,
		CLEF_POSITION,
		MARKER,
		BAR,
		BAR_GAP,
		SCROLL,
		CURVE_POINT,
		CURVE_POINT_NONE,
	};

	Selection();
	bool allow_auto_scroll() const;
	bool is_in(int type) const;
	void clear();

	SongSelection to_song_sel() const;
};

bool hover_changed(Selection &hover, Selection &hover_old);

#endif /* SRC_VIEW_SELECTION_H_ */
