/*
 * AudioViewLayer.cpp
 *
 *  Created on: 25.10.2014
 *      Author: michi
 */

#include "AudioViewLayer.h"
#include "AudioView.h"
#include "Mode/ViewMode.h"
#include "Mode/ViewModeMidi.h"
#include "../Tsunami.h"
#include "../TsunamiWindow.h"
#include "SideBar/SideBar.h"
#include "../Data/base.h"
#include "../Data/Song.h"
#include "../Data/Track.h"
#include "../Data/TrackLayer.h"
#include "../Data/TrackMarker.h"
#include "../Data/SampleRef.h"
#include "../Data/Sample.h"
#include "../Data/CrossFade.h"
#include "../Data/Audio/AudioBuffer.h"
#include "../Data/Rhythm/Bar.h"
#include "../Data/Rhythm/Beat.h"
#include "../Data/Rhythm/BarCollection.h"
#include "../Data/Midi/MidiData.h"
#include "../Data/Midi/Clef.h"
#include "../Module/Synth/Synthesizer.h"
#include "../Module/Audio/SongRenderer.h"
#include "Helper/SymbolRenderer.h"


void draw_str_with_shadow(Painter *c, float x, float y, const string &str, const color &col_text, const color &col_shadow);


const int PITCH_SHOW_COUNT = 30;
const int PITCH_MIN_DEFAULT = 25;
const int PITCH_MAX_DEFAULT = 105;



AudioViewLayer::AudioViewLayer(AudioView *_view, TrackLayer *_layer)
{
	view = _view;
	layer = _layer;
	solo = false;

	edit_pitch_min = 55;
	edit_pitch_max = edit_pitch_min + PITCH_SHOW_COUNT;

	pitch_min = PITCH_MIN_DEFAULT;
	pitch_max = PITCH_MAX_DEFAULT;

	area = rect(0, 0, 0, 0);
	height_min = height_wish = 0;
	clef_dy = 0;
	clef_y0 = 0;

	hidden = false;
	represents_imploded = false;

	if (layer)
		set_midi_mode(view->midi_view_mode);
}

void AudioViewLayer::set_midi_mode(MidiMode wanted)
{
	midi_mode = wanted;
	if ((wanted == MidiMode::TAB) and (layer->track->instrument.string_pitch.num > 0))
		midi_mode = MidiMode::TAB;
	view->thm.dirty = true;
	view->force_redraw();
}

color AudioViewLayer::pitch_color(int pitch)
{
	return SetColorHSB(1, (float)(pitch % 12) / 12.0f, 0.6f, 1);
}

color AudioViewLayer::marker_color(const TrackMarker *m)
{
	return pitch_color(m->text.hash() % MAX_PITCH);
}

static Array<complex> tt;

inline void draw_line_buffer(Painter *c, double view_pos, double zoom, float hf, float x0, float x1, float y0, const Array<float> &buf, int offset)
{
	int nl = 0;
	int i0 = max((double)x0 / zoom + view_pos - offset    , 0.0);
	int i1 = min((double)x1 / zoom + view_pos - offset + 2, (double)buf.num);
	if (i1 < i0)
		return;

	tt.resize(i1 - i0);

	for (int i=i0; i<i1; i++){

		double p = x0 + ((double)(i + offset) + 0.5 - view_pos) * zoom;
		tt[nl].x = (float)p;
		tt[nl].y = y0 + buf[i] * hf;
		if (zoom > 5)
			c->draw_circle(p, tt[nl].y, 2);
		nl ++;
	}
	c->draw_lines(tt);
}

inline void draw_line_buffer_sel(Painter *c, double view_pos, double zoom, float hf, float x0, float x1, float y0, const Array<float> &buf, int offset, const Range &r)
{
	int nl = 0;
	int i0 = max((double)x0 / zoom + view_pos - offset    , 0.0);
	int i1 = min((double)x1 / zoom + view_pos - offset + 2, (double)buf.num);
	if (i1 < i0)
		return;

	tt.resize(i1 - i0);
	c->set_line_width(3.0f);

	for (int i=i0; i<i1; i++){
		if (!r.is_inside(i + offset))
			continue;

		double p = x0 + ((double)(i + offset) + 0.5 - view_pos) * zoom;
		tt[nl].x = (float)p;
		tt[nl].y = y0 + buf[i] * hf;
		if (zoom > 5)
			c->draw_circle(p, tt[nl].y, 4);
		nl ++;
	}
	tt.resize(nl);
	c->draw_lines(tt);
	c->set_line_width(1.0f);
}

inline void draw_peak_buffer(Painter *c, int di, double view_pos_rel, double zoom, float f, float hf, float x0, float x1, float y0, const string &buf, int offset)
{
	if ((x1 - x0) / di < 1)
		return;
	int nl = 0;
	double dpos = 1.0 / zoom;
	// pixel position
	// -> buffer position
	double p0 = view_pos_rel;
	tt.resize((int)((x1 - x0)/di + 10));
	for (int i=x0; i<x1+di; i+=di){

		double p = p0 + dpos * (double)i + 0.5;
		int ip = (int)(p - offset)/f;
		if ((ip >= 0) and (ip < buf.num))
			if (((int)(p) < offset + buf.num*f) and (p >= offset)){
				tt[nl].x = i;
				float dy = ((float)(buf[ip])/255.0f) * hf;
				tt[nl].y  = y0 - dy;
				nl ++;
			}
	}
	if (nl == 0)
		return;
	tt.resize(nl * 2);
	for (int i=0; i<nl; i++){
		tt[nl + i].x = tt[nl - i - 1].x;
		tt[nl + i].y = y0 *2 - tt[nl - i - 1].y + 1;
	}
	c->draw_polygon(tt);
}

inline void draw_peak_buffer_sel(Painter *c, int di, double view_pos_rel, double zoom, float f, float hf, float x0, float x1, float y0, const string &buf, int offset)
{
	if ((x1 - x0) / di < 1)
		return;
	int nl = 0;
	double dpos = 1.0 / zoom;
	// pixel position
	// -> buffer position
	double p0 = view_pos_rel;
	tt.resize((int)((x1 - x0)/di + 10));
	for (int i=x0; i<x1+di; i+=di){

		double p = p0 + dpos * (double)i + 0.5;
		int ip = (int)(p - offset)/f;
		if ((ip >= 0) and (ip < buf.num))
			if (((int)(p) < offset + buf.num*f) and (p >= offset)){
				tt[nl].x = i;
				float dy = ((float)(buf[ip])/255.0f) * hf;
				tt[nl].y  = y0 - dy - 1;
				nl ++;
			}
	}
	if (nl == 0)
		return;
	tt.resize(nl * 2);
	for (int i=0; i<nl; i++){
		tt[nl + i].x = tt[nl - i - 1].x;
		tt[nl + i].y = y0 *2 - tt[nl - i - 1].y + 1;
	}
	c->draw_polygon(tt);
}

void AudioViewLayer::draw_buffer(Painter *c, AudioBuffer &b, double view_pos_rel, const color &col, float x0, float x1)
{
	//float w = area.width();
	float h = area.height();
	float hf = h / (2 * b.channels);

	// zero heights of both channels
	float y0[2];
	for (int ci=0; ci<b.channels; ci++)
		y0[ci] = area.y1 + hf * (2*ci + 1);


	int di = view->detail_steps;
	c->set_color(col);

	//int l = min(view->prefered_buffer_layer - 1, b.peaks.num / 4);
	int l = view->prefered_buffer_layer * 4;
	if (l >= 0){
		double bzf = view->buffer_zoom_factor;

		// no peaks yet? -> show dummy
		if (b.peaks.num <= l){
			c->set_color(ColorInterpolate(col, Red, 0.3f));
			c->draw_rect((b.offset - view_pos_rel) * view->cam.scale, area.y1, b.length * view->cam.scale, h);
			return;
		}

		// maximum
		double _bzf = bzf;
		int ll = l;
		color cc = col;
		cc.a *= 0.3f;
		c->set_color(cc);
		for (int ci=0; ci<b.channels; ci++)
			draw_peak_buffer(c, di, view_pos_rel, view->cam.scale, _bzf, hf, x0, x1, y0[ci], b.peaks[ll+ci], b.offset);


		// mean square
		c->set_color(col);
		for (int ci=0; ci<b.channels; ci++)
			draw_peak_buffer(c, di, view_pos_rel, view->cam.scale, bzf, hf, x0, x1, y0[ci], b.peaks[l+2+ci], b.offset);


		// invalid peaks...
		if (b.peaks.num >= b.PEAK_MAGIC_LEVEL4){
			int nn = min(b.length / b.PEAK_CHUNK_SIZE, b.peaks[b.PEAK_MAGIC_LEVEL4].num);
			for (int i=0; i<nn; i++){
				if (b._peaks_chunk_needs_update(i)){
					c->set_color(ColorInterpolate(col, Red, 0.3f));
					float xx0 = max((float)view->cam.sample2screen(b.offset + i*b.PEAK_CHUNK_SIZE), x0);
					float xx1 = min((float)view->cam.sample2screen(b.offset + (i+1)*b.PEAK_CHUNK_SIZE), x1);
					c->draw_rect(xx0, area.y1, xx1 - xx0, h);
				}
			}
		}
	}else{

		// directly show every sample
		for (int ci=0; ci<b.channels; ci++)
			draw_line_buffer(c, view_pos_rel, view->cam.scale, hf, x0, x1, y0[ci], b.c[ci], b.offset);
	}
}

void AudioViewLayer::draw_buffer_selection(Painter *c, AudioBuffer &b, double view_pos_rel, const color &col, const Range &r)
{
	float h = area.height();
	float hf = h / (2 * b.channels);
	float x0 = area.x1;
	float x1 = area.x2;

	// zero heights of both channels
	float y0[2];
	for (int ci=0; ci<b.channels; ci++)
		y0[ci] = area.y1 + hf * (2*ci + 1);


	int di = view->detail_steps;
	color cc = col;
	cc.a = 0.7f;
	c->set_color(cc);

	//int l = min(view->prefered_buffer_layer - 1, b.peaks.num / 4);
	int l = view->prefered_buffer_layer * 4;
	if (l >= 0){
		double bzf = view->buffer_zoom_factor;


		// no peaks yet? -> show dummy
		if (b.peaks.num <= l){
			c->set_color(ColorInterpolate(col, Red, 0.3f));
			c->draw_rect((b.offset - view_pos_rel) * view->cam.scale, area.y1, b.length * view->cam.scale, h);
			return;
		}

		// maximum
		double _bzf = bzf;
		int ll = l;
		/*double _bzf = bzf;
		int ll = l;
		if (ll + 4 < b.peaks.num){
			ll += 4;
			_bzf *= 2;
		}*/
		float xmin = max(x0, (float)view->cam.sample2screen(r.start()));
		float xmax = min(x1, (float)view->cam.sample2screen(r.end()));
		for (int ci=0; ci<b.channels; ci++)
			draw_peak_buffer_sel(c, di, view_pos_rel, view->cam.scale, _bzf, hf, xmin, xmax, y0[ci], b.peaks[ll+ci], b.offset);
	}else{

		// directly show every sample
		for (int ci=0; ci<b.channels; ci++)
			draw_line_buffer_sel(c, view_pos_rel, view->cam.scale, hf, x0, x1, y0[ci], b.c[ci], b.offset, r);
	}
}



// active | passive | active | ...
Array<Range> version_ranges(TrackLayer *l)
{
	Array<Range> r;
	if (!l->is_main())
		r.add(Range::EMPTY);
	int own_index = l->version_number();
	CrossFade prev;
	prev.position = -2000000000;
	prev.samples = 0;
	prev.target = 0;
	for (auto &f: l->track->fades){
		if (f.target == own_index){
			r.add(RangeTo(prev.range().end(), f.range().start()));
			prev = f;
		}else if (prev.target == own_index){
			r.add(RangeTo(prev.range().start(), f.range().end()));
			prev = f;
		}
	}
	r.add(RangeTo(r.back().end(), 2000000000));
	return r;
}

void AudioViewLayer::draw_track_buffers(Painter *c)
{
	double view_pos_rel = view->cam.pos - view->song_area.x1 / view->cam.scale;
	if (is_playable() and layer->track->has_version_selection()){
		Array<Range> rr = version_ranges(layer);
		for (AudioBuffer &b: layer->buffers){
			foreachi(Range &r, rr, i){
				float x0, x1;
				view->cam.range2screen_clip(r, area, x0, x1);
				draw_buffer(c, b, view_pos_rel, (i % 2) ? view->colors.text_soft3 : view->colors.text, x0, x1);
			}
		}
	}else{
		color col = is_playable() ? view->colors.text : view->colors.text_soft3;
		for (AudioBuffer &b: layer->buffers)
			draw_buffer(c, b, view_pos_rel, col, area.x1, area.x2);

	}

	if (view->sel.has(layer)){
		// selection
		for (AudioBuffer &b: layer->buffers){
			draw_buffer_selection(c, b, view_pos_rel, view->colors.selection_boundary, view->sel.range);
		}
	}
}

void AudioViewLayer::draw_sample_frame(Painter *c, SampleRef *s, const color &col, int delay)
{
	// frame
	Range rr = s->range() + delay;
	float asx, aex;
	view->cam.range2screen_clip(rr, area, asx, aex);

	if (delay == 0)
		s->area = rect(asx, aex, area.y1, area.y2);


	color col2 = col;
	col2.a *= 0.5f;
	c->set_color(col2);
	c->draw_rect(asx, area.y1,                             aex - asx, view->SAMPLE_FRAME_HEIGHT);
	c->draw_rect(asx, area.y2 - view->SAMPLE_FRAME_HEIGHT, aex - asx, view->SAMPLE_FRAME_HEIGHT);

	c->set_color(col);
	c->draw_line(asx, area.y1, asx, area.y2);
	c->draw_line(aex, area.y1, aex, area.y2);
	c->draw_line(asx, area.y1, aex, area.y1);
	c->draw_line(asx, area.y2, aex, area.y2);
}


void AudioViewLayer::draw_sample(Painter *c, SampleRef *s)
{
	color col = view->colors.sample;
	if (view->sel.has(s))
		col = view->colors.sample_selected;
	if (view->hover.sample == s)
		col = ColorInterpolate(col,  view->colors.hover, 0.2f);

	draw_sample_frame(c, s, col, 0);

	// buffer
	if (s->type() == SignalType::AUDIO)
		draw_buffer(c, *s->buf, view->cam.pos - (double)s->pos, col, area.x1, area.x2);
	else if (s->type() == SignalType::MIDI)
		view->mode->draw_midi(c, this, *s->midi, true, s->pos);

	if (view->sel.has(s)){
		int asx = clampi(view->cam.sample2screen(s->pos), area.x1, area.x2);
		draw_str_with_shadow(c, asx, area.y2 - view->SAMPLE_FRAME_HEIGHT, s->origin->name, view->colors.text, view->colors.background_track_selected);
	}
}

void AudioViewLayer::draw_marker(Painter *c, const TrackMarker *marker, int index, bool hover)
{
	string text = marker->text;
	if (text.match(":pos *:"))
		text = "🖐 " + text.substr(5, -2);
	if (marker->fx.num > 0)
		text += format(" (%d fx)", marker->fx.num);

	bool sel = view->sel.has(marker);

	if (sel)
		c->set_font("", -1, true, false);

	float w = c->get_str_width(text) + view->CORNER_RADIUS * 2;
	float x0, x1;
	view->cam.range2screen(marker->range, x0, x1);
	float y0 = area.y1;
	float y1 = y0 + 15;

	w = max(w, x1 - x0);

	color col = view->colors.text;
	color col_bg = view->colors.background_track;
	color col2 = marker_color(marker);
	if (sel){
		col = view->colors.selection;
		col_bg = ColorInterpolate(view->colors.background_track, view->colors.selection, 0.2f);
		col2 = ColorInterpolate(col2, view->colors.selection, 0.8f);
	}
	if (hover){
		col = ColorInterpolate(col, view->colors.hover, 0.3f);
		col2 = ColorInterpolate(col2, view->colors.hover, 0.3f);
	}

	color col_bg2 = col2;
	col_bg2.a = 0.65f;
	c->set_color(col_bg2);
	c->draw_rect(x0, y0, x1-x0, y1-y0);
	c->set_color(col2);
	c->set_line_width(2.0f);
	c->draw_line(x0, area.y1, x0, area.y2);
	c->draw_line(x1, area.y1, x1, area.y2);
	c->set_line_width(1.0f);
	view->draw_boxed_str(c,  x0 + view->CORNER_RADIUS, y0 + 10, text, col, col_bg);

	marker_areas[index] = rect(x0, x0 + w, y0, y1);
	marker_label_areas[index] = view->get_boxed_str_rect(c,  x0 + view->CORNER_RADIUS, y0 + 10, text);

	c->set_font("", -1, false, false);
}



void AudioViewLayer::set_edit_pitch_min_max(int _min, int _max)
{
	int diff = _max - _min;
	edit_pitch_min = clampi(_min, 0, MAX_PITCH - 1 - diff);
	edit_pitch_max = edit_pitch_min + diff;
	view->force_redraw();
}

float AudioViewLayer::pitch2y_linear(int pitch)
{
	return area.y2 - area.height() * ((float)pitch - pitch_min) / (pitch_max - pitch_min);
}

float AudioViewLayer::pitch2y_classical(int pitch)
{
	NoteModifier mod;
	const Clef& clef = layer->track->instrument.get_clef();
	int p = clef.pitch_to_position(pitch, view->midi_scale, mod);
	return clef_pos_to_screen(p);
}

int AudioViewLayer::y2pitch_linear(float y)
{
	return pitch_min + ((area.y2 - y) * (pitch_max - pitch_min) / area.height());
}

int AudioViewLayer::y2pitch_classical(float y, NoteModifier modifier)
{
	const Clef& clef = layer->track->instrument.get_clef();
	int pos = screen_to_clef_pos(y);
	return clef.position_to_pitch(pos, view->midi_scale, modifier);
}

int AudioViewLayer::y2clef_classical(float y, NoteModifier &mod)
{
	mod = NoteModifier::UNKNOWN;//modifier;
	return screen_to_clef_pos(y);
}

int AudioViewLayer::y2clef_linear(float y, NoteModifier &mod)
{
	mod = NoteModifier::UNKNOWN;//modifier;

	int pitch = y2pitch_linear(y);
	const Clef& clef = layer->track->instrument.get_clef();
	return clef.pitch_to_position(pitch, view->midi_scale, mod);
}

bool AudioViewLayer::mouse_over()
{
	return !hidden and area.inside(view->mx, view->my);
}

void get_col(color &col, color &col_shadow, const MidiNote *n, AudioViewLayer::MidiNoteState state, AudioView *view, bool playable)
{
	if (playable)
		col = ColorInterpolate(AudioViewLayer::pitch_color(n->pitch), view->colors.text, 0.2f);
	else
		col = view->colors.text_soft3;

	if (state & AudioViewLayer::STATE_HOVER){
		col = ColorInterpolate(col, view->colors.hover, 0.333f);
	}else if (state & AudioViewLayer::STATE_REFERENCE){
		col = ColorInterpolate(col, view->colors.background_track, 0.65f);
	}
	col_shadow = ColorInterpolate(col, view->colors.background_track, 0.3f);
}

inline AudioViewLayer::MidiNoteState note_state(MidiNote *n, bool as_reference, AudioView *view)
{
	AudioViewLayer::MidiNoteState s = AudioViewLayer::STATE_DEFAULT;
	if (view->sel.has(n))
		s = AudioViewLayer::STATE_SELECTED;
	if (as_reference)
		return (AudioViewLayer::MidiNoteState)(AudioViewLayer::STATE_REFERENCE | s);
	if ((view->hover.type == Selection::Type::MIDI_NOTE) and (n == view->hover.note))
		return (AudioViewLayer::MidiNoteState)(AudioViewLayer::STATE_HOVER | s);
	return s;
}

struct NoteData{
	float x, y;
	int offset, length, end;
	MidiNote *n;
	color col;
};

const float NOTE_NECK_LENGTH = 35.0f;
const float NOTE_BAR_DISTANCE = 10.0f;
const float NOTE_BAR_WIDTH = 5.0f;
const float NOTE_NECK_WIDTH = 2.0f;
const float NOTE_FLAG_DX = 10.0f;
const float NOTE_FLAG_DY = 15.0f;

void draw_single_ndata(Painter *c, NoteData &d)
{
	int base_length = d.length;
	bool punctured = ((base_length % 9) == 0);
	if (punctured)
		base_length = (base_length / 3) * 2;
	bool tripplets = ((base_length == 2) or (base_length == 4) or (base_length == 8));
	if (tripplets)
		base_length = (base_length / 2) * 3;

	c->set_color(d.col);
	if (base_length == 3){
		// 1/16
		c->set_line_width(NOTE_NECK_WIDTH);
		c->draw_line(d.x, d.y, d.x, d.y - NOTE_NECK_LENGTH);
		c->set_line_width(NOTE_BAR_WIDTH);
		c->draw_line(d.x, d.y - NOTE_NECK_LENGTH, d.x + NOTE_FLAG_DX, d.y - NOTE_NECK_LENGTH + NOTE_FLAG_DY);
		c->draw_line(d.x, d.y - NOTE_NECK_LENGTH + NOTE_BAR_DISTANCE, d.x + NOTE_FLAG_DX, d.y - NOTE_NECK_LENGTH + NOTE_BAR_DISTANCE + NOTE_FLAG_DY);
	}else if (base_length == 6){
		// 1/8
		c->set_line_width(NOTE_NECK_WIDTH);
		c->draw_line(d.x, d.y, d.x, d.y - NOTE_NECK_LENGTH);
		c->set_line_width(NOTE_BAR_WIDTH);
		c->draw_line(d.x, d.y - NOTE_NECK_LENGTH, d.x + NOTE_FLAG_DX, d.y - NOTE_NECK_LENGTH + NOTE_FLAG_DY);
	}else if (base_length == 12){
		// 1/4
		c->set_line_width(NOTE_NECK_WIDTH);
		c->draw_line(d.x, d.y, d.x, d.y - NOTE_NECK_LENGTH);
	}

	if (punctured)
		c->draw_circle(d.x + 8, d.y + 5, 2);
	if (tripplets)
		c->draw_str(d.x, d.y - NOTE_NECK_LENGTH - 15, "3");
}

void draw_group_ndata(Painter *c, const Array<NoteData> &d)
{
	int base_length = d[0].length;
	bool tripplets = ((base_length == 2) or (base_length == 4) or (base_length == 8));
	if (tripplets)
		base_length = (base_length / 2) * 3;

	float x0 = d[0].x, y0 = d[0].y-NOTE_NECK_LENGTH;
	float x1 = d.back().x, y1 = d.back().y-NOTE_NECK_LENGTH;
	float m = (y1 - y0) / (x1 - x0);

	c->set_line_width(NOTE_NECK_WIDTH);
	for (auto &dd: d){
		c->set_color(dd.col);
		c->draw_line(dd.x, dd.y, dd.x, y0 + m * (dd.x - x0));
	}
	c->set_line_width(NOTE_BAR_WIDTH);
	for (int i=0; i<d.num; i++){
		float t0 = (float)i     / (float)d.num;
		float t1 = (float)(i+1) / (float)d.num;
		c->set_color(d[i].col);
		c->draw_line(x0 + (x1-x0)*t0, y0 + (y1-y0)*t0, x0 + (x1-x0)*t1, y0 + (y1-y0)*t1);
		if (base_length == 3)
			c->draw_line(x0 + (x1-x0)*t0, y0 + (y1-y0)*t0 + NOTE_BAR_DISTANCE, x0 + (x1-x0)*t1, y0 + (y1-y0)*t1 + NOTE_BAR_DISTANCE);
	}
	if (tripplets)
		c->draw_str((x0 + x1)/2, (y0 + y1)/2 - 17, "3");
}

void draw_rhythm(Painter *c, AudioViewLayer *vlayer, const MidiNoteBuffer &midi, const Range &range, int shift, bool as_reference, std::function<float(MidiNote*)> y_func)
{
	Song *song = vlayer->layer->song();
	AudioView *view = vlayer->view;
	if (view->cam.scale * song->sample_rate < 20)
		return;

	auto bars = song->bars.get_bars(range);
	for (auto *b: bars){
		const int BEAT_PARTITION = 12;

		// samples per 16th / 3
		float spu = (float)b->range().length / (float)b->num_beats / (float)BEAT_PARTITION;

		MidiNoteBufferRef bnotes = midi.get_notes(b->range());
		//c->set_color(view->colors.text_soft3);

		Array<NoteData> ndata;
		for (MidiNote *n: bnotes){
			Range r = n->range and b->range();
			NoteData d;
			d.n = n;
			d.offset = int((float)(r.offset - b->range().offset) / spu + 0.5f);
			d.length = int((float)(r.end() - b->range().offset) / spu + 0.5f) - d.offset;
			if (d.length == 0 or d.offset < 0)
				continue;
			d.end = d.offset + d.length;

			d.x = view->cam.sample2screen(n->range.offset + shift);
			d.y = y_func(n);


			color col_shadow;
			get_col(d.col, col_shadow, n, note_state(n, as_reference, view), view, vlayer->is_playable());

			// prevent double notes
			if (ndata.num > 0)
				if (ndata.back().offset == d.offset){
					// use higher note...
					if (d.n->pitch > ndata.back().n->pitch)
						ndata[ndata.num - 1] = d;
					continue;
				}

			ndata.add(d);
		}

		//int offset = 0;
		for (int i=0; i<ndata.num; i++){
			NoteData &d = ndata[i];

			if ((d.length == 6 or d.length == 3 or d.length == 4) and ((d.offset % BEAT_PARTITION) == 0)){ //((d.offset % (d.length * 2)) == 0)){
				int max_group_length = 2;
				if (d.length == 3 and ((d.offset % 12) == 0))
					max_group_length = 4;
				if (d.length == 4)
					max_group_length = 3;
				Array<NoteData> group = d;
				for (int j=i+1; j<min(ndata.num, i+max_group_length); j++){
					if (ndata[j].length == d.length and ndata[j].offset == ndata[j-1].end){
						group.add(ndata[j]);
					}else
						break;
				}
				if (group.num > 1){
					draw_group_ndata(c, group);
					i += group.num - 1;
					continue;
				}
			}

			draw_single_ndata(c, d);
		}

	}
}

void AudioViewLayer::draw_complex_note(Painter *c, const MidiNote *n, MidiNoteState state, float x1, float x2, float y, float r)
{
	if (state & AudioViewLayer::STATE_SELECTED){
		color col1 = view->colors.selection;
		draw_simple_note(c, x1, x2, y, r, 2, col1, col1, false);
	}

	color col, col_shadow;
	get_col(col, col_shadow, n, state, view, is_playable());

	draw_simple_note(c, x1, x2, y, r, 0, col, col_shadow, false);

}


void AudioViewLayer::draw_midi_note_linear(Painter *c, const MidiNote &n, int shift, MidiNoteState state)
{
	float x1, x2;
	view->cam.range2screen(n.range, x1, x2);
	float y1 = pitch2y_linear(n.pitch + 1);
	float y2 = pitch2y_linear(n.pitch);

	float y = (y1 + y2) / 2;
	n.y = y;
	float r = max((y2 - y1) / 2.3f, 2.0f);

	draw_complex_note(c, &n, state, x1, x2, y, r);
}

void AudioViewLayer::draw_midi_linear(Painter *c, const MidiNoteBuffer &midi, bool as_reference, int shift)
{
	Range range = view->cam.range() - shift;
	MidiNoteBufferRef notes = midi.get_notes(range);
	//c->setLineWidth(3.0f);

	if (view->mode_midi->editing(this)){
		pitch_min = edit_pitch_min;
		pitch_max = edit_pitch_max;
	}else{
		pitch_min = PITCH_MIN_DEFAULT;
		pitch_max = PITCH_MAX_DEFAULT;
	}

	// draw notes
	for (MidiNote *n: midi){
		if ((n->pitch < pitch_min) or (n->pitch >= pitch_max))
			continue;
		draw_midi_note_linear(c, *n, shift, note_state(n, as_reference, view));
	}
}

void AudioViewLayer::draw_simple_note(Painter *c, float x1, float x2, float y, float r, float rx, const color &col, const color &col_shadow, bool force_circle)
{
	//x1 += r;
	// "shadow" to indicate length
	if (x2 - x1 > r*1.5f){
		c->set_color(col_shadow);
		c->draw_rect(x1, y - r*0.7f - rx, x2 - x1 + rx, r*2*0.7f + rx*2);
	}

	// the note circle
	c->set_color(col);
	if ((x2 - x1 > 6) or force_circle)
		c->draw_circle(x1, y, r+rx);
	else
		c->draw_rect(x1 - r*0.8f - rx, y - r*0.8f - rx, r*1.6f + rx*2, r*1.6f + rx*2);
}

void AudioViewLayer::draw_midi_clef_tab(Painter *c)
{
	if (is_playable())
		c->set_color(view->colors.text_soft1);
	else
		c->set_color(view->colors.text_soft3);

	// clef lines
	float dy = (area.height() * 0.7f) / layer->track->instrument.string_pitch.num;
	dy = min(dy, 40.0f);
	clef_dy = dy;

	float h = dy * layer->track->instrument.string_pitch.num;
	float y0 = area.y2 - (area.height() - h) / 2 - dy/2;
	clef_y0 = y0;
	for (int i=0; i<layer->track->instrument.string_pitch.num; i++){
		float y = y0 - i*dy;
		c->draw_line(area.x1, y, area.x2, y);
	}
	c->set_antialiasing(true);


	if (is_playable())
		c->set_color(view->colors.text);
	else
		c->set_color(view->colors.text_soft3);

	c->set_font_size(h / 6);
	c->draw_str(10, area.y1 + area.height() / 2 - h * 0.37f, "T\nA\nB");
	c->set_font_size(view->FONT_SIZE);
}

void AudioViewLayer::draw_midi_note_tab(Painter *c, const MidiNote *n, int shift, MidiNoteState state)
{
	float r = min(clef_dy/2, 13.0f);

	float x1, x2;
	view->cam.range2screen(n->range + shift, x1, x2);


	int p = n->stringno;
	float y = string_to_screen(p);
	n->y = y;

	draw_complex_note(c, n, state, x1, x2, y, r);

	if (x2 - x1 > r/4 and r > 5){
		float font_size = r * 1.2f;
		c->set_color(view->colors.high_contrast_b);//text);
		SymbolRenderer::draw(c, x1, y - font_size*0.75f, font_size, i2s(n->pitch - layer->track->instrument.string_pitch[n->stringno]), 0);
	}
}

void AudioViewLayer::draw_midi_tab(Painter *c, const MidiNoteBuffer &midi, bool as_reference, int shift)
{
	Range range = view->cam.range() - shift;
	midi.update_meta(layer->track, view->midi_scale);
	MidiNoteBufferRef notes = midi.get_notes(range);


	draw_rhythm(c, this, midi, range, shift, as_reference, [&](MidiNote *n){ return string_to_screen(n->stringno); });

	for (MidiNote *n: notes)
		draw_midi_note_tab(c,  n,  shift,  note_state(n, as_reference, view));

	c->set_antialiasing(false);
	c->set_font_size(view->FONT_SIZE);
}

float AudioViewLayer::clef_pos_to_screen(int pos)
{
	return area.y2 - area.height() / 2 - (pos - 4) * clef_dy / 2.0f;
}

int AudioViewLayer::screen_to_clef_pos(float y)
{
	return (int)floor((area.y2 - y - area.height() / 2) * 2.0f / clef_dy + 0.5f) + 4;
}

float AudioViewLayer::string_to_screen(int string_no)
{
	return clef_y0 - string_no * clef_dy;
}

int AudioViewLayer::screen_to_string(float y)
{
	return (int)floor((clef_y0 - y) / clef_dy + 0.5f);
}

void AudioViewLayer::draw_midi_note_classical(Painter *c, const MidiNote *n, int shift, MidiNoteState state, const Clef &clef)
{
	float r = min(clef_dy/2, 10.0f);

	float x1, x2;
	view->cam.range2screen(n->range + shift, x1, x2);

	// checked before...
//	if (n.clef_position < 0)
//		n.update_meta(track->instrument, view->midi_scale, 0);

	int p = n->clef_position;
	float y = clef_pos_to_screen(p);
	n->y = y;

	// auxiliary lines
	for (int i=10; i<=p; i+=2){
		c->set_color(view->colors.text_soft2);
		float y = clef_pos_to_screen(i);
		c->draw_line(x1 - clef_dy, y, x1 + clef_dy, y);
	}
	for (int i=-2; i>=p; i-=2){
		c->set_color(view->colors.text_soft2);
		float y = clef_pos_to_screen(i);
		c->draw_line(x1 - clef_dy, y, x1 + clef_dy, y);
	}

	draw_complex_note(c, n, state, x1, x2, y, r);

	if ((n->modifier != NoteModifier::NONE) and (r >= 3)){
		c->set_color(view->colors.text);
		//c->setColor(ColorInterpolate(col, view->colors.text, 0.5f));
		float size = r*2.8f;
		SymbolRenderer::draw(c, x1 - size*0.7f, y - size*0.8f , size, modifier_symbol(n->modifier));
	}
}

bool AudioViewLayer::is_playable()
{
	return view->get_playable_layers().contains(layer);
}

void AudioViewLayer::draw_midi_clef_classical(Painter *c, const Clef &clef, const Scale &scale)
{
	// clef lines
	float dy = min(area.height() / 13, 30.0f);
	clef_dy = dy;

	if (is_playable())
		c->set_color(view->colors.text_soft1);
	else
		c->set_color(view->colors.text_soft3);

	for (int i=0; i<10; i+=2){
		float y = clef_pos_to_screen(i);
		c->draw_line(area.x1, y, area.x2, y);
	}

	if (is_playable())
		c->set_color(view->colors.text);
	else
		c->set_color(view->colors.text_soft3);

	// clef symbol
	c->set_font_size(dy*4);
	c->draw_str(area.x1 + 10, clef_pos_to_screen(10), clef.symbol);
	c->set_font_size(dy);

	for (int i=0; i<7; i++)
		if (scale.modifiers[i] != NoteModifier::NONE)
			c->draw_str(area.x1 + 18 + dy*3.0f + dy*0.6f*(i % 3), clef_pos_to_screen((i - clef.offset + 7*20) % 7) - dy*0.8f, modifier_symbol(scale.modifiers[i]));
	c->set_font_size(view->FONT_SIZE);
}



void AudioViewLayer::draw_midi_classical(Painter *c, const MidiNoteBuffer &midi, bool as_reference, int shift)
{
	Range range = view->cam.range() - shift;
	midi.update_meta(layer->track, view->midi_scale);
	MidiNoteBufferRef notes = midi.get_notes(range);


	const Clef& clef = layer->track->instrument.get_clef();

	c->set_antialiasing(true);



	draw_rhythm(c, this, midi, range, shift, as_reference, [&](MidiNote *n){ return clef_pos_to_screen(n->clef_position); });

	for (MidiNote *n: notes)
		draw_midi_note_classical(c, n, shift, note_state(n, as_reference, view), clef);

	c->set_font_size(view->FONT_SIZE);
	c->set_antialiasing(false);
}

color AudioViewLayer::background_color()
{
	return (view->sel.has(layer)) ? view->colors.background_track_selected : view->colors.background_track;
}

color AudioViewLayer::background_selection_color()
{
	return (view->sel.has(layer)) ? view->colors.background_track_selection : view->colors.background_track;
}

void AudioViewLayer::draw_blank_background(Painter *c)
{
	color cc = background_color();
	if ((view->sel.range.length > 0) and (view->sel.has(layer))){
		c->set_color(cc);
		c->draw_rect(area);

		color cs = background_selection_color();
		float x1, x2;
		view->cam.range2screen_clip(view->sel.range, area, x1, x2);
		c->set_color(cs);
		c->draw_rect(x1, area.y1, x2-x1, area.height());

	}else{
		c->set_color(cc);
		c->draw_rect(area);
	}
}

bool AudioView::editing_layer(AudioViewLayer *l)
{
	if (cur_vlayer != l)
		return false;
	if (win->side_bar->is_active(SideBar::MIDI_EDITOR_CONSOLE))
		return true;
	if (win->side_bar->is_active(SideBar::CAPTURE_CONSOLE))
		return true;
	return false;
}


void AudioViewLayer::draw_version_header(Painter *c)
{
	bool hover = (view->hover.layer == layer) and view->hover.is_in(Selection::Type::LAYER_HEADER);
	bool visible = hover or view->editing_layer(this);
	bool playable = view->get_playable_layers().contains(layer);

	color col = view->colors.background_track_selected;
	if (view->sel.has(layer))
		col = ColorInterpolate(col, view->colors.selection, 0.2f);
	if (hover)
		col = ColorInterpolate(col, view->colors.hover, 0.2f);
	c->set_color(col);
	float h = visible ? view->TRACK_HANDLE_HEIGHT : view->TRACK_HANDLE_HEIGHT_SMALL;
	c->set_roundness(view->CORNER_RADIUS);
	c->draw_rect(area.x2 - view->LAYER_HANDLE_WIDTH,  area.y1,  view->LAYER_HANDLE_WIDTH, h);
	c->set_roundness(0);

	// track title
	c->set_font("", view->FONT_SIZE, view->sel.has(layer) and playable, false);
	if (playable)
		c->set_color(view->colors.text);
	else
		c->set_color(view->colors.text_soft2);
	string title;
	if (layer->track->has_version_selection()){
		if (layer->is_main())
			title = _("base");
		else
			title = "v" + i2s(layer->version_number() + 1);
	}else{
		title = "l" + i2s(layer->version_number() + 1);
	}
	if (solo)
		title += " (solo)";
	c->draw_str(area.x2 - view->LAYER_HANDLE_WIDTH + 23, area.y1 + 3, title);

	c->set_font("", -1, false, false);

	// icons
	if (layer->type == SignalType::BEATS){
		c->set_color(view->colors.text);
		c->draw_mask_image(area.x2 - view->LAYER_HANDLE_WIDTH + 5, area.y1 + 5, *view->images.track_time); // "⏱"
	}else if (layer->type == SignalType::MIDI){
		c->set_color(view->colors.text);
		c->draw_mask_image(area.x2 - view->LAYER_HANDLE_WIDTH + 5, area.y1 + 5, *view->images.track_midi); // "♫"
	}else{
		c->set_color(view->colors.text);
		c->draw_mask_image(area.x2 - view->LAYER_HANDLE_WIDTH + 5, area.y1 + 5, *view->images.track_audio); // "∿"
	}

	color col_but = ColorInterpolate(view->colors.text, view->colors.hover, 0.3f);
	color col_but_hover = view->colors.text;

	if (visible and !layer->track->has_version_selection()){
		/*c->setColor(col_but);
		if ((view->hover.layer == layer) and (view->hover.type == Selection::Type::LAYER_BUTTON_DOMINANT))
			c->setColor(col_but_hover);
		//c->drawStr(area.x1 + 5, area.y1 + 22-2, "\U0001f50a"); // U+1F50A "🔊"
		c->drawMaskImage(area.x2 - view->LAYER_HANDLE_WIDTH + 5, area.y1 + 22, *view->images.speaker);*/


		c->set_color(col_but);
		if ((view->hover.layer == layer) and (view->hover.type == Selection::Type::LAYER_BUTTON_SOLO))
			c->set_color(col_but_hover);
		//c->drawStr(area.x1 + 5 + 17, area.y1 + 22-2, "S");
		c->draw_mask_image(area.x2 - view->LAYER_HANDLE_WIDTH + 22, area.y1 + 22, *view->images.solo);
	}

	if (visible and layer->is_main()){
		if (represents_imploded){
			c->set_color(col_but);
			if ((view->hover.layer == layer) and (view->hover.type == Selection::Type::LAYER_BUTTON_EXPLODE))
				c->set_color(col_but_hover);
			c->draw_str(area.x2 - view->LAYER_HANDLE_WIDTH + 22+17, area.y1 + 22-2, "+");
		}else{
			c->set_color(col_but);
			if ((view->hover.layer == layer) and (view->hover.type == Selection::Type::LAYER_BUTTON_IMPLODE))
				c->set_color(col_but_hover);
			c->draw_str(area.x2 - view->LAYER_HANDLE_WIDTH + 22+17, area.y1 + 22-2, "-");
		}
	}
}

void AudioViewLayer::set_solo(bool _solo)
{
	solo = _solo;
	view->renderer->allow_layers(view->get_playable_layers());
	view->force_redraw();
	notify();
	view->notify(view->MESSAGE_SOLO_CHANGE);
}


void AudioViewLayer::draw(Painter *c)
{
	if (!on_screen() or hidden)
		return;

	if (!represents_imploded)
		view->mode->draw_layer_data(c, this);

	if (layer->track->layers.num > 1)
		draw_version_header(c);
}

bool AudioViewLayer::on_screen()
{
	return (area.y1 < view->song_area.y2) and (area.y2 > view->song_area.y1);
}
