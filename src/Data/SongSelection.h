/*
 * SongSelection.h
 *
 *  Created on: 06.03.2016
 *      Author: michi
 */

#ifndef SRC_DATA_SONGSELECTION_H_
#define SRC_DATA_SONGSELECTION_H_

#include "../lib/base/base.h"
#include "../lib/base/set.h"
#include "Range.h"

class Song;
class Track;
class TrackLayer;
class SampleRef;
class TrackMarker;
class MidiNote;
class Bar;

class SongSelection
{
public:
	SongSelection();

	void clear();
	void clear_data();
	void all(Song *s);
	void _update_bars(Song *s);
	static SongSelection from_range(Song *s, const Range &r);
	static SongSelection from_range(Song *s, const Range &r, Set<const Track*> tracks, Set<const TrackLayer*> layers);

	void make_consistent(Song *s);
	void _update_tracks_from_layers(Song *s);

	enum Mask{
		SAMPLES = 1,
		MARKERS = 2,
		MIDI_NOTES = 4,
		BARS = 8,
		ALL = -1
	};
	SongSelection filter(int mask) const;

	Range range;

	Set<const Track*> tracks;
	Set<const TrackLayer*> track_layers;
	Set<const SampleRef*> samples;
	Set<const TrackMarker*> markers;
	Set<const MidiNote*> notes;
	Set<const Bar*> bars;
	int bar_gap;

	void add(const Track *t);
	void set(const Track *t, bool selected);
	bool has(const Track *t) const;
	void click(const Track *t, bool control_pressed);

	void add(const TrackLayer *l);
	void set(const TrackLayer *l, bool selected);
	bool has(const TrackLayer *l) const;
	void click(const TrackLayer *l, bool control_pressed);

	void add(const SampleRef *s);
	void set(const SampleRef *s, bool selected);
	bool has(const SampleRef *s) const;
	void click(const SampleRef *s, bool control_pressed);

	void add(const TrackMarker *m);
	void set(const TrackMarker *m, bool selected);
	bool has(const TrackMarker *m) const;
	void click(const TrackMarker *m, bool control_pressed);

	void add(const MidiNote *n);
	void set(const MidiNote *n, bool selected);
	bool has(const MidiNote *n) const;
	void click(const MidiNote *n, bool control_pressed);

	void add(const Bar *b);
	void set(const Bar *b, bool selected);
	bool has(const Bar *b) const;
	void click(const Bar *b, bool control_pressed);

	int num_samples() const;
	bool is_empty() const;
	Array<int> bar_indices(Song *song) const;

	SongSelection restrict_to_track(Track *t) const;
	SongSelection operator||(const SongSelection &s) const;
	SongSelection minus(const SongSelection &s) const;
};

#endif /* SRC_DATA_SONGSELECTION_H_ */
