/*
 * ViewModeMidi.cpp
 *
 *  Created on: 12.11.2015
 *      Author: michi
 */

#include "ViewModeMidi.h"

#include "../../Module/Audio/SongRenderer.h"
#include "../../Module/Synth/Synthesizer.h"
#include "../../Module/Port/MidiPort.h"
#include "../../Device/OutputStream.h"
#include "../../Data/base.h"
#include "../../Data/Song.h"
#include "../../Data/Track.h"
#include "../../Data/TrackLayer.h"
#include "../../Data/Sample.h"
#include "../../Data/Midi/Clef.h"
#include "../../Data/SongSelection.h"
#include "../../Data/SampleRef.h"
#include "../../TsunamiWindow.h"
#include "../../Session.h"
#include "../AudioView.h"
#include "../AudioViewLayer.h"
#include "../Helper/MidiPreview.h"
#include "../Helper/ScrollBar.h"

void align_to_beats(Song *s, Range &r, int beat_partition);

const int EDIT_PITCH_SHOW_COUNT = 30;

ViewModeMidi::ViewModeMidi(AudioView *view) :
	ViewModeDefault(view)
{
	beat_partition = 4;
	note_length = 1;
	win->setInt("beat_partition", beat_partition);
	win->setInt("note_length", note_length);
	mode_wanted = MidiMode::CLASSICAL;
	creation_mode = CreationMode::NOTE;
	input_mode = InputMode::DEFAULT;
	midi_interval = 3;
	chord_type = ChordType::MINOR;
	chord_inversion = 0;
	modifier = NoteModifier::NONE;

	moving = false;
	string_no = 0;
	octave = 3;

	scroll = new ScrollBar;
	scroll->update(EDIT_PITCH_SHOW_COUNT, 128);

	preview = new MidiPreview(view->session);

	mouse_pre_moving_pos = -1;
}

ViewModeMidi::~ViewModeMidi()
{
	delete preview;
}


void ViewModeMidi::set_modifier(NoteModifier mod)
{
	modifier = mod;
	notify();
}

void ViewModeMidi::set_mode(MidiMode _mode)
{
	mode_wanted = _mode;
	view->thm.dirty = true;
	view->force_redraw();
	notify();
}

void ViewModeMidi::set_creation_mode(CreationMode _mode)
{
	creation_mode = _mode;
	view->force_redraw();
	notify();
}

void ViewModeMidi::set_input_mode(InputMode _mode)
{
	input_mode = _mode;
	view->force_redraw();
	notify();
}

bool ViewModeMidi::editing(AudioViewLayer *l)
{
	if (view->mode != this)
		return false;
	if (l != view->cur_vlayer)
		return false;
	if (l->layer->type != SignalType::MIDI)
		return false;
	return true;
}

TrackLayer* ViewModeMidi::cur_layer()
{
	return view->cur_layer();
}

AudioViewLayer* ViewModeMidi::cur_vlayer()
{
	return view->cur_vlayer;
}


void ViewModeMidi::start_midi_preview(const Array<int> &pitch, float ttl)
{
	preview->start(view->cur_track()->synth, pitch, view->cur_track()->volume, ttl);
}

void ViewModeMidi::on_left_button_down()
{
	ViewModeDefault::on_left_button_down();
	auto mode = cur_vlayer()->midi_mode;

	bool over_sel_note = false;
	if (hover->note)
		over_sel_note = view->sel.has(hover->note);


	if (creation_mode == CreationMode::SELECT and !over_sel_note){
		set_cursor_pos(hover->pos, true);
		view->msp.start(hover->pos, hover->y0);

	}else{
		//view->msp.start(hover->pos, hover->y0);
		view->hide_selection = true;
	}

	if (hover->type == Selection::Type::MIDI_NOTE){
		view->sel.click(hover->note, win->getKey(hui::KEY_CONTROL));

		dnd_start_soon(view->sel);
	}else if (hover->type == Selection::Type::CLEF_POSITION){
		/*if (creation_mode != CreationMode::SELECT){
			view->msp.stop();
		}*/
		view->msp.start_pos = hover->pos; // TODO ...bad
		if (mode == MidiMode::TAB){
			string_no = clampi(hover->clef_position, 0, view->cur_track()->instrument.string_pitch.num - 1);
		}
	}else if (hover->type == Selection::Type::MIDI_PITCH){
		view->msp.start_pos = hover->pos; // TODO ...bad
		if (mode == MidiMode::TAB){
		}else{ // CLASSICAL/LINEAR
			// create new note
			start_midi_preview(get_creation_pitch(hover->pitch), 1.0f);
		}
	}else if (hover->type == Selection::Type::SCROLLBAR_MIDI){
		scroll->drag_start(view->mx, view->my);
		view->msp.stop();
	}
}

void ViewModeMidi::on_left_button_up()
{
	ViewModeDefault::on_left_button_up();
	view->hide_selection = false;

	auto mode = cur_vlayer()->midi_mode;
	if ((mode == MidiMode::CLASSICAL) or (mode == MidiMode::LINEAR)){
		if (hover->type == Selection::Type::MIDI_PITCH){
			auto notes = get_creation_notes(hover, view->msp.start_pos);
			if (notes.num > 0){
				set_cursor_pos(notes[0]->range.end() + 1, true);
				octave = pitch_get_octave(hover->pitch);
				view->cur_layer()->addMidiNotes(notes);
				notes.clear(); // all notes owned by track now
			}
			preview->end();
		}
	}

	if (moving){
		moving = false;
	}
}

void ViewModeMidi::on_mouse_move()
{
	ViewModeDefault::on_mouse_move();
	auto e = hui::GetEvent();

	if (hover->type == Selection::Type::MIDI_PITCH){
		// creating notes
		//view->forceRedraw();
	}else if (hover->type == Selection::Type::SCROLLBAR_MIDI){
		if (e->lbut){
			scroll->drag_update(view->mx, view->my);
			int _pitch_max = 127 - scroll->offset;
			cur_vlayer()->set_edit_pitch_min_max(_pitch_max - EDIT_PITCH_SHOW_COUNT, _pitch_max);
		}
	}
}

MidiNote *make_note(ViewModeMidi *m, const Range &r, int pitch, NoteModifier mod, float volume = 1.0f)
{
	auto *n = new MidiNote(r, modifier_apply(pitch, mod), volume);
	n->modifier = mod;

	// dirty hack for clef position...
	const Clef& clef = m->view->cur_track()->instrument.get_clef();
	NoteModifier dummy;
	n->clef_position = clef.pitch_to_position(pitch, m->view->midi_scale, dummy);
	return n;
}

void ViewModeMidi::edit_add_pause()
{
	Range r = get_midi_edit_range();
	set_cursor_pos(r.end() + 1, true);
}

void ViewModeMidi::edit_add_note_by_relative(int relative)
{
	Range r = get_midi_edit_range();
	int pitch = pitch_from_octave_and_rel(relative, octave);
	view->cur_layer()->addMidiNote(make_note(this, r, pitch, modifier));
	set_cursor_pos(r.end() + 1, true);
	start_midi_preview(pitch, 0.1f);
}

void ViewModeMidi::edit_add_note_on_string(int hand_pos)
{
	Range r = get_midi_edit_range();
	int pitch = cur_layer()->track->instrument.string_pitch[string_no] + hand_pos;
	MidiNote *n = new MidiNote(r, pitch, 1.0f);
	n->stringno = string_no;
	cur_layer()->addMidiNote(n);
	set_cursor_pos(r.end() + 1, true);
	start_midi_preview(pitch, 0.1f);
}

void ViewModeMidi::edit_backspace()
{
	int a = song->bars.get_prev_sub_beat(view->sel.range.offset-1, beat_partition);
	Range r = Range(a, view->sel.range.offset-a);
	SongSelection s = SongSelection::from_range(view->song, r, view->cur_layer()->track, view->cur_layer()).filter(SongSelection::Mask::MIDI_NOTES);
	view->song->deleteSelection(s);
	set_cursor_pos(a, true);
}

void ViewModeMidi::jump_string(int delta)
{
	string_no = max(min(string_no + delta, cur_layer()->track->instrument.string_pitch.num - 1), 0);
	view->force_redraw();

}

void ViewModeMidi::jump_octave(int delta)
{
	octave = max(min(octave + delta, 7), 0);
	view->force_redraw();
}

void ViewModeMidi::on_key_down(int k)
{
	auto mode = cur_vlayer()->midi_mode;
	if ((mode == MidiMode::CLASSICAL) or (mode == MidiMode::LINEAR)){
		if (input_mode == InputMode::DEFAULT){
			if (k == hui::KEY_0){
				set_modifier(NoteModifier::NONE);
			}else if (k == hui::KEY_FENCE){
				set_modifier(NoteModifier::SHARP);
			}else if (k == hui::KEY_3){
				set_modifier(NoteModifier::FLAT);
			}else if (k == hui::KEY_4){
				set_modifier(NoteModifier::NATURAL);
			}

			// add note
			if ((k >= hui::KEY_A) and (k <= hui::KEY_G)){
				int number = (k - hui::KEY_A);
				int rel[7] = {9,11,0,2,4,5,7};
				edit_add_note_by_relative(rel[number]);
			}
		}

		// add break
		if (k == hui::KEY_DOT)
			edit_add_pause();

		// remove
		if (k == hui::KEY_BACKSPACE)
			edit_backspace();

		// select octave
		if (k == hui::KEY_UP)
			jump_octave(1);
		if (k == hui::KEY_DOWN)
			jump_octave(-1);
	}else if (mode == MidiMode::TAB){
		if (input_mode == InputMode::DEFAULT){

			// add note
			if (((k >= hui::KEY_0) and (k <= hui::KEY_9)) or ((k >= hui::KEY_A) and (k <= hui::KEY_F))){
				int number = (k - hui::KEY_0);
				if (k >= hui::KEY_A)
					number = 10 + (k - hui::KEY_A);
				edit_add_note_on_string(number);
			}
		}

		// add break
		if (k == hui::KEY_DOT)
			edit_add_pause();

		// remove
		if (k == hui::KEY_BACKSPACE)
			edit_backspace();

		// select string
		if (k == hui::KEY_UP)
			jump_string(1);
		if (k == hui::KEY_DOWN)
			jump_string(-1);
	}

	if (input_mode == InputMode::NOTE_LENGTH){
		if (((k >= hui::KEY_1) and (k <= hui::KEY_9)) or ((k >= hui::KEY_A) and (k <= hui::KEY_F))){
			int number = (k - hui::KEY_0);
			if (k >= hui::KEY_A)
				number = 10 + (k - hui::KEY_A);
			set_note_length(number);
			set_input_mode(InputMode::DEFAULT);
		}
	}else if (input_mode == InputMode::BEAT_PARTITION){
		if (((k >= hui::KEY_1) and (k <= hui::KEY_9)) or ((k >= hui::KEY_A) and (k <= hui::KEY_F))){
			int number = (k - hui::KEY_0);
			if (k >= hui::KEY_A)
				number = 10 + (k - hui::KEY_A);
			set_beat_partition(number);
			set_input_mode(InputMode::DEFAULT);
		}
	}

	if (k == hui::KEY_L){
		set_input_mode(InputMode::NOTE_LENGTH);
	}
	if (k == hui::KEY_P){
		set_input_mode(InputMode::BEAT_PARTITION);
	}
	if (k == hui::KEY_ESCAPE){
		set_input_mode(InputMode::DEFAULT);
	}

	//if (k == hui::KEY_ESCAPE)
		//tsunami->side_bar->open(SideBar::MIDI_EDITOR_CONSOLE);
		//view->setMode(view->mode_default);

	ViewModeDefault::on_key_down(k);
}

float ViewModeMidi::layer_min_height(AudioViewLayer *l)
{
	if (editing(l)){
		auto mode = l->midi_mode;
		if (mode == MidiMode::LINEAR)
			return 500;
		else if (mode == MidiMode::CLASSICAL)
			return view->MAX_TRACK_CHANNEL_HEIGHT * 6;
		else // TAB
			return view->MAX_TRACK_CHANNEL_HEIGHT * 4;
	}

	return ViewModeDefault::layer_min_height(l);
}

float ViewModeMidi::layer_suggested_height(AudioViewLayer *l)
{
	if (editing(l)){
		auto mode = l->midi_mode;
		if (mode == MidiMode::LINEAR)
			return 5000;
		else if (mode == MidiMode::CLASSICAL)
			return view->MAX_TRACK_CHANNEL_HEIGHT * 6;
		else // TAB
			return view->MAX_TRACK_CHANNEL_HEIGHT * 4;
	}

	return ViewModeDefault::layer_suggested_height(l);
}

void ViewModeMidi::on_cur_layer_change()
{
	view->thm.dirty = true;
}


Range get_allowed_midi_range(TrackLayer *l, Array<int> pitch, int start)
{
	Range allowed = Range::ALL;
	for (MidiNote *n: l->midi){
		for (int p: pitch)
			if (n->pitch == p){
				if (n->range.is_inside(start))
					return Range::EMPTY;
			}
	}

	MidiEventBuffer midi = midi_notes_to_events(l->midi);
	for (MidiEvent &e: midi)
		for (int p: pitch)
			if (e.pitch == p){
				if ((e.pos >= start) and (e.pos < allowed.end()))
					allowed.set_end(e.pos);
				if ((e.pos < start) and (e.pos >= allowed.start()))
					allowed.set_start(e.pos);
			}
	return allowed;
}

Array<int> ViewModeMidi::get_creation_pitch(int base_pitch)
{
	Array<int> pitch;
	if (creation_mode == CreationMode::NOTE){
		pitch.add(base_pitch);
	}else if (creation_mode == CreationMode::INTERVAL){
		pitch.add(base_pitch);
		if (midi_interval != 0)
			pitch.add(base_pitch + midi_interval);
	}else if (creation_mode == CreationMode::CHORD){
		pitch = chord_notes(chord_type, chord_inversion, base_pitch);
	}
	return pitch;
}

MidiNoteBuffer ViewModeMidi::get_creation_notes(Selection *sel, int pos0)
{
	int start = min(pos0, sel->pos);
	int end = max(pos0, sel->pos);
	Range r = Range(start, end - start);

	// align to beats
	if (song->bars.num > 0)
		align_to_beats(song, r, beat_partition);

	Array<int> pitch = get_creation_pitch(sel->pitch);

	// collision?
	Range allowed = get_allowed_midi_range(view->cur_layer(), pitch, pos0);

	// create notes
	MidiNoteBuffer notes;
	if (allowed.empty())
		return notes;
	for (int p: pitch)
		notes.add(new MidiNote(r and allowed, p, 1));
	if (notes.num > 0){
		notes[0]->clef_position = sel->clef_position;
		notes[0]->modifier = sel->modifier;
	}
	return notes;
}

void ViewModeMidi::set_beat_partition(int partition)
{
	beat_partition = partition;
	view->force_redraw();
	notify();
}

void ViewModeMidi::set_note_length(int length)
{
	note_length = length;
	view->force_redraw();
	notify();
}

void ViewModeMidi::draw_layer_background(Painter *c, AudioViewLayer *l)
{
	l->draw_blank_background(c);

	color cc = l->background_color();
	color cc_sel = l->background_selection_color();
	color fg = view->colors.grid;
	color fg_sel = (view->sel.has(l->layer)) ? view->colors.grid_selected : view->colors.grid;
	if (song->bars.num > 0)
		view->draw_grid_bars(c, l->area, fg, fg_sel, cc, cc_sel, beat_partition);
	else
		view->draw_grid_time(c, l->area, fg, fg_sel, cc, cc_sel, false);

	if (l->layer->type == SignalType::MIDI){
		auto mode = l->midi_mode;
		if (l == view->cur_vlayer){
			if (mode == MidiMode::LINEAR)
				draw_layer_pitch_grid(c, l);
		}

		if (mode == MidiMode::CLASSICAL){
			const Clef& clef = l->layer->track->instrument.get_clef();
			l->draw_midi_clef_classical(c, clef, view->midi_scale);
		}else if (mode == MidiMode::TAB){
			l->draw_midi_clef_tab(c);
		}
	}
}

void ViewModeMidi::draw_layer_pitch_grid(Painter *c, AudioViewLayer *l)
{
	// pitch grid
	c->set_color(color(0.25f, 0, 0, 0));
	for (int i=l->pitch_min; i<l->pitch_max; i++){
		float y0 = l->pitch2y_linear(i + 1);
		float y1 = l->pitch2y_linear(i);
		if (!view->midi_scale.contains(i)){
			c->set_color(color(0.2f, 0, 0, 0));
			c->draw_rect(l->area.x1, y0, l->area.width(), y1 - y0);
		}
	}


	// pitch names
	color cc = view->colors.text;
	cc.a = 0.4f;
	Array<SampleRef*> *p = nullptr;
	if ((l->layer->track->synth) and (l->layer->track->synth->module_subtype == "Sample")){
		auto *c = l->layer->track->synth->get_config();
		p = (Array<SampleRef*> *)&c[1];
	}
	bool is_drum = (l->layer->track->instrument.type == Instrument::Type::DRUMS);
	for (int i=l->pitch_min; i<l->pitch_max; i++){
		c->set_color(cc);
		if (((hover->type == Selection::Type::MIDI_PITCH) or (hover->type == Selection::Type::MIDI_NOTE)) and (i == hover->pitch))
			c->set_color(view->colors.text);

		string name = pitch_name(i);
		if (is_drum){
			name = drum_pitch_name(i);
		}else if (p){
			if (i < p->num)
				if ((*p)[i])
					name = (*p)[i]->origin->name;
		}
		c->draw_str(20, l->area.y1 + l->area.height() * (l->pitch_max - i - 1) / EDIT_PITCH_SHOW_COUNT, name);
	}
}

inline bool hover_note_classical(const MidiNote &n, Selection &s, ViewModeMidi *vmm)
{
	if (n.clef_position != s.clef_position)
		return false;
	return n.range.is_inside(s.pos);
}

inline bool hover_note_tab(const MidiNote &n, Selection &s, ViewModeMidi *vmm)
{
	if (n.stringno != s.clef_position)
		return false;
	return n.range.is_inside(s.pos);
}

inline bool hover_note_linear(const MidiNote &n, Selection &s, ViewModeMidi *vmm)
{
	if (n.pitch != s.pitch)
		return false;
	return n.range.is_inside(s.pos);
}

Selection ViewModeMidi::get_hover()
{
	Selection s = ViewModeDefault::get_hover();
	if (s.type != s.Type::LAYER)
		return s;

	int mx = view->mx;
	int my = view->my;

	// midi
	if ((s.layer) and (s.layer->type == SignalType::MIDI) and (s.vlayer == view->cur_vlayer)){
		auto mode = s.vlayer->midi_mode;

		// scroll bar
		if ((mode == MidiMode::LINEAR) and (scroll->area.inside(view->mx, view->my))){
			s.type = Selection::Type::SCROLLBAR_MIDI;
			return s;
		}

		/*if (creation_mode != CreationMode::SELECT)*/{
			if ((mode == MidiMode::CLASSICAL)){
				s.pitch = cur_vlayer()->y2pitch_classical(my, modifier);
				s.clef_position = cur_vlayer()->screen_to_clef_pos(my);
				s.modifier = modifier;
				s.type = Selection::Type::MIDI_PITCH;
				s.index = randi(100000); // quick'n'dirty fix to force view update every time the mouse moves

				foreachi(MidiNote *n, s.layer->midi, i)
					if (hover_note_classical(*n, s, this)){
						s.note = n;
						s.index = i;
						s.type = Selection::Type::MIDI_NOTE;
						return s;
					}
			}else if ((mode == MidiMode::TAB)){
				//s.pitch = cur_track->y2pitch_classical(my, modifier);
				s.clef_position = cur_vlayer()->screen_to_string(my);
				s.modifier = modifier;
				s.type = Selection::Type::CLEF_POSITION;
				s.index = randi(100000); // quick'n'dirty fix to force view update every time the mouse moves

				foreachi(MidiNote *n, s.layer->midi, i)
					if (hover_note_tab(*n, s, this)){
						s.note = n;
						s.index = i;
						s.type = Selection::Type::MIDI_NOTE;
						return s;
					}
			}else if (mode == MidiMode::LINEAR){
				s.pitch = cur_vlayer()->y2pitch_linear(my);
				s.clef_position = cur_vlayer()->y2clef_linear(my, s.modifier);
				//s.modifier = modifier;
				s.type = Selection::Type::MIDI_PITCH;
				s.index = randi(100000); // quick'n'dirty fix to force view update every time the mouse moves

				foreachi(MidiNote *n, s.layer->midi, i)
					if (hover_note_linear(*n, s, this)){
						s.note = n;
						s.index = i;
						s.type = Selection::Type::MIDI_NOTE;
						return s;
					}
			}
		}
		/*if (creation_mode == CreationMode::SELECT){
			if ((s.type == Selection::Type::MIDI_PITCH) or (s.type == Selection::Type::CLEF_POSITION)){
				s.type = Selection::Type::TRACK;
			}
		}*/
	}

	return s;
}

void ViewModeMidi::draw_layer_data(Painter *c, AudioViewLayer *l)
{
	// midi
	if (editing(l)){

		/*for (int n: t->reference_tracks)
			if ((n >= 0) and (n < song->tracks.num) and (song->tracks[n] != t->track))
				drawMidi(c, t, song->tracks[n]->midi, true, 0);*/

		draw_midi(c, l, l->layer->midi, false, 0);

		auto mode = l->midi_mode;

		if ((mode == MidiMode::CLASSICAL) or (mode == MidiMode::LINEAR)){

			// current creation
			if ((hui::GetEvent()->lbut) and (hover->type == Selection::Type::MIDI_PITCH)){
				auto notes = get_creation_notes(hover, view->msp.start_pos);
				draw_midi(c, l, notes, false, 0);
				//c->setFontSize(view->FONT_SIZE);
			}


			// creation preview
			if ((!hui::GetEvent()->lbut) and (hover->type == Selection::Type::MIDI_PITCH)){
				auto notes = get_creation_notes(hover, hover->pos);
				draw_midi(c, l, notes, false, 0);
			}
		}


		if (mode == MidiMode::CLASSICAL){

		}else if (mode == MidiMode::LINEAR){

			// scrollbar
			scroll->offset = 127 - cur_vlayer()->pitch_max;
			scroll->set_area(rect(l->area.x2 - view->SCROLLBAR_WIDTH, l->area.x2, l->area.y1, l->area.y2));
			scroll->draw(c, hover->type == Selection::Type::SCROLLBAR_MIDI);
		}
	}else{

		// not editing -> just draw
		if (l->layer->type == SignalType::MIDI)
			draw_midi(c, l, l->layer->midi, false, 0);
	}


	// samples
	for (SampleRef *s: l->layer->samples)
		l->draw_sample(c, s);


	if (l->layer->is_main()){

		Track *t = l->layer->track;

		// marker
		l->marker_areas.resize(t->markers.num);
		l->marker_label_areas.resize(t->markers.num);
		foreachi(TrackMarker *m, t->markers, i)
			l->draw_marker(c, m, i, (view->hover.type == Selection::Type::MARKER) and (view->hover.track == t) and (view->hover.index == i));
	}

}

void ViewModeMidi::draw_track_data(Painter *c, AudioViewTrack *t)
{
}

void ViewModeMidi::draw_post(Painter *c)
{
	ViewModeDefault::draw_post(c);

	auto *l = cur_vlayer();
	auto mode = l->midi_mode;
	Range r = get_midi_edit_range();
	int x1 = view->cam.sample2screen(r.start());
	int x2 = view->cam.sample2screen(r.end());

	c->set_color(view->colors.selection_internal);
	if (mode == MidiMode::TAB){
		int y = l->string_to_screen(string_no);
		int y1 = y - l->clef_dy/2;
		int y2 = y + l->clef_dy/2;
		c->draw_rect(x1,  y1,  x2 - x1,  y2 - y1);
	}else if (mode == MidiMode::CLASSICAL){
		int p1 = pitch_from_octave_and_rel(0, octave);
		int p2 = pitch_from_octave_and_rel(0, octave+1);
		int y1 = l->pitch2y_classical(p2);
		int y2 = l->pitch2y_classical(p1);
		c->draw_rect(x1,  y1,  x2 - x1,  y2 - y1);
	}else if (mode == MidiMode::LINEAR){
		int p1 = pitch_from_octave_and_rel(0, octave);
		int p2 = pitch_from_octave_and_rel(0, octave+1);
		int y1 = l->pitch2y_linear(p2);
		int y2 = l->pitch2y_linear(p1);
		c->draw_rect(x1,  y1,  x2 - x1,  y2 - y1);
	}

	string message = _("add pause (.)    delete (⟵)    note length (L)    beat partition (P)");
	if (mode == MidiMode::TAB)
		message += "    " + _("string (↑,↓)    add note (0-9, A-F)");
	else if ((mode == MidiMode::CLASSICAL) or (mode == MidiMode::LINEAR))
		message += "    " + _("octave (↑,↓)    modifiers (#,3,0)    add note (A-G)");
	if (input_mode == InputMode::NOTE_LENGTH)
		message = _("enter note length (1-9, A-F)    cancel (Esc)");
	else if (input_mode == InputMode::BEAT_PARTITION)
		message = _("enter beat partition (1-9, A-F)    cancel (Esc)");
	view->draw_boxed_str(c, (view->song_area.x1 + view->song_area.x2)/2, view->area.y2 - 30, message, view->colors.text_soft1, view->colors.background_track_selected, 0);

}

Range ViewModeMidi::get_midi_edit_range()
{
	int a = song->bars.get_prev_sub_beat(view->sel.range.offset+1, beat_partition);
	int b = song->bars.get_next_sub_beat(view->sel.range.end()-1, beat_partition);
	if (a == b)
		b = song->bars.get_next_sub_beat(b, beat_partition);
	for (int i=1; i<note_length; i++)
		b = song->bars.get_next_sub_beat(b, beat_partition);
	return Range(a, b - a);
}

void ViewModeMidi::start_selection()
{
	hover->range.set_start(view->msp.start_pos);
	hover->range.set_end(hover->pos);
	if (hover->type == Selection::Type::TIME){
		hover->type = Selection::Type::SELECTION_END;
		view->selection_mode = view->SelectionMode::TIME;
	}else{
		hover->y0 = view->msp.start_y;
		hover->y1 = view->my;
		view->selection_mode = view->SelectionMode::RECT;
	}
	view->set_selection(get_selection(hover->range));
}
