/*
 * AudioViewTrack.cpp
 *
 *  Created on: 25.10.2014
 *      Author: michi
 */

#include "AudioViewTrack.h"
#include "AudioView.h"
#include "Mode/ViewMode.h"
#include "../Tsunami.h"
#include "../Data/Song.h"
#include "../Midi/MidiData.h"
#include "../Midi/Clef.h"
#include "../Audio/Synth/Synthesizer.h"

AudioViewTrack::AudioViewTrack(AudioView *_view, Track *_track)
{
	view = _view;
	track = _track;

	area = rect(0, 0, 0, 0);
	height_min = height_wish = 0;
	clef_dy = 0;
}

AudioViewTrack::~AudioViewTrack()
{
}



color AudioViewTrack::getPitchColor(int pitch)
{
	return SetColorHSB(1, (float)(pitch % 12) / 12.0f, 0.6f, 1);
}

const float AudioViewTrack::MIN_GRID_DIST = 10.0f;

static Array<complex> tt;

inline void draw_line_buffer(Painter *c, int width, double view_pos, double zoom, float hf, float x, float y0, const Array<float> &buf, int offset)
{
	int nl = 0;
	int i0 = max((double) x          / zoom + view_pos - offset    , 0.0);
	int i1 = min((double)(x + width) / zoom + view_pos - offset + 2, (double)buf.num);
	if (i1 < i0)
		return;

	tt.resize(i1 - i0);

	for (int i=i0; i<i1; i++){

		double p = x + ((double)(i + offset) + 0.5 - view_pos) * zoom;
		tt[nl].x = (float)p;
		tt[nl].y = y0 + buf[i] * hf;
		if (zoom > 5)
			c->drawCircle(p, tt[nl].y, 2);
		nl ++;
	}
	c->drawLines(tt);
}

inline void draw_peak_buffer(Painter *c, int width, int di, double view_pos_rel, double zoom, float f, float hf, float x, float y0, const string &buf, int offset)
{
	int nl = 0;
	double dpos = 1.0 / zoom;
	// pixel position
	// -> buffer position
	double p0 = view_pos_rel;
	tt.resize(width/di + 10);
	for (int i=0; i<width+di; i+=di){

		double p = p0 + dpos * (double)i + 0.5;
		int ip = (int)(p - offset)/f;
		if ((ip >= 0) and (ip < buf.num))
		if (((int)(p) < offset + buf.num*f) and (p >= offset)){
			tt[nl].x = (float)x+i;
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
	c->drawPolygon(tt);
}

void AudioViewTrack::drawBuffer(Painter *c, BufferBox &b, double view_pos_rel, const color &col)
{
	msg_db_f("DrawBuffer", 1);

	float w = area.width();
	float h = area.height();
	float hf = h / 4;
	float x1 = area.x1;
	float y1 = area.y1;

	// zero heights of both channels
	float y0r = y1 + hf;
	float y0l = y1 + hf * 3;

	if (view->show_mono){
		y0r = y1 + h / 2;
		hf *= 2;
	}


	int di = view->detail_steps;
	c->setColor(col);

	// no peaks yet? -> show dummy
	if (b.peaks.num <= 4){
		c->drawRect((b.offset - view_pos_rel) * view->cam.scale, y1, b.length * view->cam.scale, h);
		return;
	}

	int l = min(view->prefered_buffer_level - 1, b.peaks.num / 4);
	if (l >= 1){//f < MIN_MAX_FACTOR){

		if ((view->peak_mode == BufferBox::PEAK_MAXIMUM) or (view->peak_mode == BufferBox::PEAK_BOTH)){
			double bzf = view->buffer_zoom_factor;
			int ll = l;
			if (view->peak_mode == BufferBox::PEAK_BOTH){
				color cc = col;
				cc.a *= 0.3f;
				c->setColor(cc);
				if (ll < b.peaks.num / 4){
					ll ++;
					bzf *= 2;
				}
			}else{
				c->setColor(col);
			}
			draw_peak_buffer(c, w, di, view_pos_rel, view->cam.scale, bzf, hf, x1, y0r, b.peaks[ll*4-4], b.offset);
			if (!view->show_mono)
				draw_peak_buffer(c, w, di, view_pos_rel, view->cam.scale, bzf, hf, x1, y0l, b.peaks[ll*4-3], b.offset);
		}

		if ((view->peak_mode == BufferBox::PEAK_SQUAREMEAN) or (view->peak_mode == BufferBox::PEAK_BOTH)){
			c->setColor(col);
			draw_peak_buffer(c, w, di, view_pos_rel, view->cam.scale, view->buffer_zoom_factor, hf, x1, y0r, b.peaks[l*4-2], b.offset);
			if (!view->show_mono)
				draw_peak_buffer(c, w, di, view_pos_rel, view->cam.scale, view->buffer_zoom_factor, hf, x1, y0l, b.peaks[l*4-1], b.offset);
		}
	}else{
		draw_line_buffer(c, w, view_pos_rel, view->cam.scale, hf, x1, y0r, b.c[0], b.offset);
		if (!view->show_mono)
			draw_line_buffer(c, w, view_pos_rel, view->cam.scale, hf, x1, y0l, b.c[1], b.offset);
	}
}

void AudioViewTrack::drawTrackBuffers(Painter *c, double view_pos_rel)
{
	msg_db_f("DrawTrackBuffers", 1);

	// non-current levels
	foreachi(TrackLevel &lev, track->levels, level_no){
		if (level_no == view->cur_level)
			continue;
		for (BufferBox &b : lev.buffers)
			drawBuffer(c, b, view_pos_rel, view->colors.text_soft2);
	}

	// current
	for (BufferBox &b : track->levels[view->cur_level].buffers)
		drawBuffer(c, b, view_pos_rel, view->colors.text);
}

void AudioViewTrack::drawSampleFrame(Painter *c, SampleRef *s, const color &col, int delay)
{
	// frame
	Range rr = s->range() + delay;
	int asx = clampi(view->cam.sample2screen(rr.start()), area.x1, area.x2);
	int aex = clampi(view->cam.sample2screen(rr.end()), area.x1, area.x2);

	if (delay == 0)
		s->area = rect(asx, aex, area.y1, area.y2);


	color col2 = col;
	col2.a *= 0.2f;
	c->setColor(col2);
	c->drawRect(asx, area.y1,                          aex - asx, view->SAMPLE_FRAME_HEIGHT);
	c->drawRect(asx, area.y2 - view->SAMPLE_FRAME_HEIGHT, aex - asx, view->SAMPLE_FRAME_HEIGHT);

	c->setColor(col);
	c->drawLine(asx, area.y1, asx, area.y2);
	c->drawLine(aex, area.y1, aex, area.y2);
	c->drawLine(asx, area.y1, aex, area.y1);
	c->drawLine(asx, area.y2, aex, area.y2);
}

void AudioViewTrack::drawSample(Painter *c, SampleRef *s)
{
	color col = view->colors.sample;
	//bool is_cur = ((s == cur_sub) and (t->IsSelected));
	if (!view->sel.has(s))
		col = view->colors.sample_selected;
	if (view->hover.sample == s)
		col = view->colors.sample_hover;
	//col.a = 0.2f;

	drawSampleFrame(c, s, col, 0);

	color col2 = col;
	col2.a *= 0.5f;
	for (int i=0;i<s->rep_num;i++)
		drawSampleFrame(c, s, col2, (i + 1) * s->rep_delay);

	// buffer
	if (s->type() == Track::TYPE_AUDIO)
		drawBuffer(c, *s->buf, view->cam.pos - (double)s->pos, col);
	else if (s->type() == Track::TYPE_MIDI)
		drawMidi(c, *s->midi, true, s->pos);

	int asx = clampi(view->cam.sample2screen(s->pos), area.x1, area.x2);
	if (view->sel.has(s))//((is_cur) or (a->sub_mouse_over == s))
		c->drawStr(asx, area.y2 - view->SAMPLE_FRAME_HEIGHT, s->origin->name);
}

void AudioViewTrack::drawMarker(Painter *c, const TrackMarker &marker, int index, bool hover)
{
	string text = marker.text;
	if (text.match(":pos *:"))
		text = "🖐 " + text.substr(5, -2);

	bool sel = view->sel.has(&marker);

	if (sel)
		c->setFont("", -1, true, false);

	float w = c->getStrWidth(text);
	float x = view->cam.sample2screen(marker.pos);
	float y = area.y1;

	c->setFill(false);
	c->setLineWidth(3);
	if (sel)
		c->setColor(ColorInterpolate(view->colors.background_track, view->colors.selection, 0.2f));
	else
		c->setColor(view->colors.background_track);

	c->drawStr(x, area.y1, text);
	c->setFill(true);
	c->setLineWidth(1);

	marker_areas[index] = rect(x, x + w, y, y + 16);

	color col = view->colors.text;
	if (sel)
		col = view->colors.selection;
	if (hover)
		col = ColorInterpolate(col, view->colors.hover, 0.3f);

	c->setColor(col);
	c->drawStr(x, y, text);

	c->setFont("", -1, false, false);
}


void AudioViewTrack::drawMidi(Painter *c, const MidiData &midi, bool as_reference, int shift)
{
	int mode = which_midi_mode(false);
	if (mode == view->MIDI_MODE_MIDI)
		drawMidiDefault(c, midi, as_reference, shift);
	else if (mode == view->MIDI_MODE_TAB)
		drawMidiTab(c, midi, as_reference, shift);
	else // if (mode == view->VIEW_MIDI_SCORE)
		drawMidiScore(c, midi, as_reference, shift);
}

void AudioViewTrack::drawMidiDefault(Painter *c, const MidiData &midi, bool as_reference, int shift)
{
	Range range = view->cam.range() - shift;
	MidiDataRef notes = midi.getNotes(range);
	c->setLineWidth(3.0f);
	for (MidiNote &n : notes){
		float x1 = view->cam.sample2screen(n.range.offset + shift);
		float x2 = view->cam.sample2screen(n.range.end() + shift);
		x2 = max(x2, x1 + 4);
		float h = area.y2 - clampf((float)n.pitch / 80.0f - 0.3f, 0, 1) * area.height();

		//color col = getPitchColor(n.pitch);
		color col = ColorInterpolate(getPitchColor(n.pitch), view->colors.text, 0.2f);
		if (view->sel.has(&n)){
			c->setLineWidth(7.0f);
			c->setColor(ColorInterpolate(view->colors.selection, view->colors.background_track, 0.5f));
			c->drawLine(x1-1, h, x2+1, h);
			c->setLineWidth(3.0f);
		}
		c->setColor(col);
		//c->drawRect(rect(x1, x2, r.y1, r.y2));
		c->drawLine(x1, h, x2, h);
	}
	c->setLineWidth(view->LINE_WIDTH);
}

void AudioViewTrack::draw_score_note(Painter *c, float x1, float x2, float y, float r, float rx, const color &col, const color &col_shadow, bool force_circle)
{
	float x = x1 + r;

	// "shadow" to indicate length
	if (x2 - x1 > r*2){
		c->setColor(col_shadow);
		c->drawRect(x, y - r - rx, x2 - x + rx, r*2 + rx*2);
	}

	// the note circle
	c->setColor(col);
	if ((x2 - x1 > 6) or force_circle)
		c->drawCircle(x, y, r+rx);
	else
		c->drawRect(x - r*0.8f - rx, y - r*0.8f - rx, r*1.6f + rx*2, r*1.6f + rx*2);
}

void AudioViewTrack::drawMidiTab(Painter *c, const MidiData &midi, bool as_reference, int shift)
{
	Range range = view->cam.range() - shift;
	MidiDataRef notes = midi.getNotes(range);
	notes.update_meta(track, view->midi_scale);



	c->setColor(view->colors.text);

	// clef lines
	float h = area.height() * 0.7f;
	float dy = h / track->instrument.string_pitch.num;
	float y0 = area.y2 - (area.height() - h) / 2 - dy/2;
	for (int i=0; i<track->instrument.string_pitch.num; i++){
		float y = y0 - i*dy;
		c->drawLine(area.x1, y, area.x2, y);
	}
	c->setAntialiasing(true);

	c->setFontSize(h / 6);
	c->drawStr(10, area.y1 + area.height() * 0.22f, "T\nA\nB");
	float r = dy/2;//min(dy/2, 8.0f);
	float font_size = r * 1.4f;
	c->setFontSize(font_size);

	for (MidiNote &n : notes){

		float x1 = view->cam.sample2screen(n.range.offset + shift);
		float x2 = view->cam.sample2screen(n.range.end() + shift);

		float y = y0 - n.stringno * dy;


		if (view->sel.has(&n)){
			//col = ColorInterpolate(col, Red, 0.3f);
			color col2 = view->colors.selection;
			draw_score_note(c, x1, x2, y, r, 2, col2, col2, false);
		}

		color col = ColorInterpolate(getPitchColor(n.pitch), view->colors.text, 0.2f);
		col = ColorInterpolate(col, view->colors.background_track, 0.3f);

		draw_score_note(c, x1, x2, y, r, 0, col, ColorInterpolate(col, view->colors.background_track, 0.3f), false);

		if (x2 - x1 > r/2){
			c->setColor(view->colors.text);
			c->drawStr(x1 + font_size*0.2f, y - font_size*0.75f, i2s(n.pitch - track->instrument.string_pitch[n.stringno]));
		}

	}
	c->setAntialiasing(false);
	c->setFontSize(view->FONT_SIZE);
}

float AudioViewTrack::clef_pos_to_screen(int pos)
{
	return area.y2 - area.height() / 2 - (pos - 4) * clef_dy / 2.0f;
}

int AudioViewTrack::screen_to_clef_pos(float y)
{
	return (int)floor((area.y2 - y - area.height() / 2) * 2.0f / clef_dy + 0.5f) + 4;
}

void AudioViewTrack::drawMidiNoteScore(Painter *c, MidiNote &n, int shift, MidiNoteState state, const Clef &clef)
{
	float r = clef_dy/2;

	float x1 = view->cam.sample2screen(n.range.offset + shift);
	float x2 = view->cam.sample2screen(n.range.end() + shift);
	float x = x1 + r;

	// checked before...
//	if (n.clef_position < 0)
//		n.update_meta(track->instrument, view->midi_scale, 0);

	int p = n.clef_position;
	float y = clef_pos_to_screen(p);

	// auxiliary lines
	for (int i=10; i<=p; i+=2){
		c->setColor(view->colors.text_soft2);
		float y = clef_pos_to_screen(i);
		c->drawLine(x - clef_dy, y, x + clef_dy, y);
	}
	for (int i=-2; i>=p; i-=2){
		c->setColor(view->colors.text_soft2);
		float y = clef_pos_to_screen(i);
		c->drawLine(x - clef_dy, y, x + clef_dy, y);
	}


	color col = ColorInterpolate(getPitchColor(n.pitch), view->colors.text, 0.2f);
	if (view->sel.has(&n)){
		color col1 = view->colors.selection;
		draw_score_note(c, x1, x2, y, r, 2, col1, col1, false);
	}
	if (state == STATE_HOVER)
		col = ColorInterpolate(col, view->colors.hover, 0.333f);
	else if (state == STATE_REFERENCE)
		col = ColorInterpolate(col, view->colors.background_track, 0.65f);

	if (n.modifier != MODIFIER_NONE){
		c->setColor(ColorInterpolate(col, view->colors.text, 0.5f));
		float size = r*2.8f;
		c->setFontSize(size);
		c->drawStr(x1 - size*0.7f, y - size*0.8f , modifier_symbol(n.modifier));
	}

	draw_score_note(c, x1, x2, y, r, 0, col, ColorInterpolate(col, view->colors.background_track, 0.4f), (state == STATE_HOVER));
}

void AudioViewTrack::drawMidiScoreClef(Painter *c, const Clef &clef, const Scale &scale)
{
	// clef lines
	float dy = min(area.height() / 13, 30.0f);
	clef_dy = dy;
	c->setColor(view->colors.text);
	for (int i=0; i<10; i+=2){
		float y = clef_pos_to_screen(i);
		c->drawLine(area.x1, y, area.x2, y);
	}

	// clef symbol
	c->setFontSize(dy*4);
	c->drawStr(10, clef_pos_to_screen(10), clef.symbol);
	c->setFontSize(dy);

	for (int i=0; i<7; i++)
		if (scale.modifiers[i] != MODIFIER_NONE)
			c->drawStr(18 + dy*3.0f + dy*0.6f*(i % 3), clef_pos_to_screen((i - clef.offset + 7*20) % 7) - dy*0.8f, modifier_symbol(scale.modifiers[i]));
	c->setFontSize(view->FONT_SIZE);
}

void AudioViewTrack::drawMidiScore(Painter *c, const MidiData &midi, bool as_reference, int shift)
{
	Range range = view->cam.range() - shift;
	MidiDataRef notes = midi.getNotes(range);
	notes.update_meta(track, view->midi_scale);

	const Clef& clef = track->instrument.get_clef();

	drawMidiScoreClef(c, clef, view->midi_scale);

	c->setAntialiasing(true);

	for (MidiNote &n : notes)
		drawMidiNoteScore(c, n, shift, as_reference ? STATE_REFERENCE : STATE_DEFAULT, clef);

	c->setFontSize(view->FONT_SIZE);
	c->setAntialiasing(false);
}

void AudioViewTrack::draw(Painter *c)
{
	view->mode->drawTrackData(c, this);

	drawHeader(c);
}

void AudioViewTrack::drawHeader(Painter *c)
{
	if (view->hover.show_track_controls == track){
		c->setColor(color(0.4f, 1, 1, 1));
		c->drawRect(0, area.y1, view->TRACK_HANDLE_WIDTH, area.height());
	}

	// track title
	c->setFont("", -1, (track == view->cur_track), false);
	c->setFill(false);
	c->setLineWidth(3);
	c->setColor(view->colors.background_track);
	c->drawStr(area.x1 + 23, area.y1 + 3, track->getNiceName());
	c->setFill(true);
	c->setLineWidth(1);
	c->setColor(view->colors.text);
	c->drawStr(area.x1 + 23, area.y1 + 3, track->getNiceName());
	c->setFont("", -1, false, false);

	// icons
	if (track->type == track->TYPE_TIME){
		c->setColor(view->colors.background_track);
		c->drawMaskImage(area.x1 + 5, area.y1 + 5, *view->images.track_time_bg);
		c->setColor(view->colors.text);
		c->drawMaskImage(area.x1 + 5, area.y1 + 5, *view->images.track_time);
	}else if (track->type == track->TYPE_MIDI){
		c->setColor(view->colors.background_track);
		c->drawMaskImage(area.x1 + 5, area.y1 + 5, *view->images.track_midi_bg);
		c->setColor(view->colors.text);
		c->drawMaskImage(area.x1 + 5, area.y1 + 5, *view->images.track_midi);
	}else{
		c->setColor(view->colors.background_track);
		c->drawMaskImage(area.x1 + 5, area.y1 + 5, *view->images.track_audio_bg);
		c->setColor(view->colors.text);
		c->drawMaskImage(area.x1 + 5, area.y1 + 5, *view->images.track_audio);
	}

	c->setColor(view->colors.text);
	if ((track->muted) or (view->hover.show_track_controls == track)){
		c->setColor(view->colors.background_track);
		c->drawMaskImage(area.x1 + 5, area.y1 + 22, *view->images.speaker_bg);
		if (track->muted)
			c->drawMaskImage(area.x1 + 5, area.y1 + 22, *view->images.x_bg);
		c->setColor(view->colors.text);
		if (view->hover.type == Selection::TYPE_MUTE)
			c->setColor(view->colors.text_soft2);
		c->drawMaskImage(area.x1 + 5, area.y1 + 22, *view->images.speaker);
		if (track->muted)
			c->drawImage(area.x1 + 5, area.y1 + 22, *view->images.x);
	}
	if ((view->song->tracks.num > 1) and (view->hover.show_track_controls == track)){
		c->setColor(view->colors.background_track);
		c->drawMaskImage(area.x1 + 22, area.y1 + 22, *view->images.solo_bg);
		c->setColor(view->colors.text);
		if (view->hover.type == Selection::TYPE_SOLO)
			c->setColor(view->colors.text_soft2);
		c->drawMaskImage(area.x1 + 22, area.y1 + 22, *view->images.solo);
	}
}


int AudioViewTrack::which_midi_mode(bool editing)
{
	if (view->midi_view_mode == view->MIDI_MODE_SCORE)
		return view->MIDI_MODE_SCORE;
	if (view->midi_view_mode == view->MIDI_MODE_MIDI)
		return editing ? view->MIDI_MODE_MIDI : view->MIDI_MODE_SCORE;
	if ((view->midi_view_mode == view->MIDI_MODE_TAB) and (track->instrument.string_pitch.num > 0))
		return view->MIDI_MODE_TAB;
	return view->MIDI_MODE_SCORE;
}

