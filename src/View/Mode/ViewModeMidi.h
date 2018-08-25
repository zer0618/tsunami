/*
 * ViewModeMidi.h
 *
 *  Created on: 12.11.2015
 *      Author: michi
 */

#ifndef SRC_VIEW_MODE_VIEWMODEMIDI_H_
#define SRC_VIEW_MODE_VIEWMODEMIDI_H_

#include "ViewModeDefault.h"
#include "../../lib/math/math.h"

class MidiNoteBuffer;
class MidiNote;
class MidiEvent;
class OutputStream;
class Synthesizer;
class MidiPreview;
class TrackLayer;
class ScrollBar;
enum class NoteModifier;
enum class ChordType;

class ViewModeMidi : public ViewModeDefault
{
public:
	ViewModeMidi(AudioView *view);
	virtual ~ViewModeMidi();

	void on_left_button_down() override;
	void on_left_button_up() override;
	void on_mouse_move() override;
	void on_key_down(int k) override;
	float layer_min_height(AudioViewLayer *l) override;
	float layer_suggested_height(AudioViewLayer *l) override;
	void on_cur_layer_change() override;

	void draw_layer_background(Painter *c, AudioViewLayer *l) override;
	void draw_layer_pitch_grid(Painter *c, AudioViewLayer *l);
	void draw_layer_data(Painter *c, AudioViewLayer *l) override;
	void draw_track_data(Painter *c, AudioViewTrack *t) override;
	void draw_post(Painter *c) override;

	Selection get_hover() override;
	void start_selection() override;

	MidiNoteBuffer get_creation_notes(Selection *sel, int pos0);
	void set_beat_partition(int partition);
	void set_note_length(int length);

	Array<int> get_creation_pitch(int base_pitch);
	Range get_midi_edit_range();
	void start_midi_preview(const Array<int> &pitch, float ttl);

	void edit_add_pause();
	void edit_add_note_on_string(int hand_pos);
	void edit_add_note_by_relative(int relative);
	void edit_backspace();
	void jump_string(int delta);
	void jump_octave(int delta);

	int beat_partition;
	int note_length;
	ChordType chord_type;
	int chord_inversion;
	int midi_interval;

	void set_modifier(NoteModifier mod);
	NoteModifier modifier;

	void set_mode(MidiMode mode);
	MidiMode mode_wanted;

	enum class CreationMode
	{
		SELECT,
		NOTE,
		INTERVAL,
		CHORD
	};
	void set_creation_mode(CreationMode mode);
	CreationMode creation_mode;

	enum class InputMode
	{
		DEFAULT,
		NOTE_LENGTH,
		BEAT_PARTITION
	};
	void set_input_mode(InputMode mode);
	InputMode input_mode;

	MidiPreview *preview;

	AudioViewLayer *cur_vlayer();
	TrackLayer *cur_layer();
	bool editing(AudioViewLayer *l);

	ScrollBar *scroll;

	bool moving;
	Array<int> pre_moving_offsets;
	int mouse_pre_moving_pos;
	int string_no;
	int octave;
};

#endif /* SRC_VIEW_MODE_VIEWMODEMIDI_H_ */
