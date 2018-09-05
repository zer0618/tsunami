/*
 * TrackRenderer.cpp
 *
 *  Created on: 02.09.2018
 *      Author: michi
 */

#include "TrackRenderer.h"
#include "SongRenderer.h"
#include "AudioEffect.h"
#include "../Beats/BarStreamer.h"
#include "../Beats/BeatMidifier.h"
#include "../Midi/MidiEffect.h"
#include "../Synth/Synthesizer.h"
#include "../Midi/MidiEventStreamer.h"
#include "../../Data/base.h"
#include "../../Data/Song.h"
#include "../../Data/Track.h"
#include "../../Data/Sample.h"
#include "../../Data/SampleRef.h"
#include "../../Data/Audio/AudioBuffer.h"
#include "../../lib/math/math.h"
#include "../../Session.h"



bool intersect_sub(SampleRef *s, const Range &r, Range &ir, int &bpos)
{
	// intersected intervall (track-coordinates)
	int i0 = max(s->pos, r.start());
	int i1 = min(s->pos + s->buf->length, r.end());

	// beginning of the intervall (relative to sub)
	ir.offset = i0 - s->pos;
	// ~ (relative to old intervall)
	bpos = i0 - r.start();
	ir.length = i1 - i0;

	return !ir.empty();
}


int get_first_usable_layer(Track *t, Set<TrackLayer*> &allowed)
{
	foreachi(TrackLayer *l, t->layers, i)
		if (allowed.contains(l))
			return i;
	return -1;
}

static void add_samples(TrackLayer *l, const Range &range_cur, AudioBuffer &buf)
{
	// subs
	for (SampleRef *s: l->samples){
		if (s->muted)
			continue;

		Range intersect_range;
		int bpos;
		if (!intersect_sub(s, range_cur, intersect_range, bpos))
			continue;

		bpos = s->pos - range_cur.start();
		buf.add(*s->buf, bpos, s->volume * s->origin->volume, 0);
	}
}

void TrackRenderer::reset_state()
{
	for (AudioEffect *fx: fx)
		fx->reset_state();
	synth->reset();
}

TrackRenderer::TrackRenderer(Track *t, SongRenderer *sr)
{
	song_renderer = sr;
	track = t;
	offset = 0;
	direct_mode = song_renderer and song_renderer->direct_mode;
	if (direct_mode)
		synth = t->synth;
	else
		synth = (Synthesizer*)t->synth->copy();
	synth->setSampleRate(t->song->sample_rate);
	synth->setInstrument(t->instrument);
	midi_streamer = nullptr;

	if (direct_mode){
		fx = t->fx;
	}else{
		for (AudioEffect *f: t->fx)
			fx.add((AudioEffect*)f->copy());
	}

	//midi.add(t, t->midi);
	if (t->type == SignalType::MIDI){
		MidiNoteBuffer _midi = t->layers[0]->midi;
		for (TrackLayer *l: t->layers)
			for (auto c: l->samples)
				if (c->type() == SignalType::MIDI)
				_midi.append(*c->midi, c->pos); // TODO: mute/solo....argh
		for (MidiEffect *fx: t->midi_fx){
			fx->prepare();
			fx->process(&_midi);
		}

		MidiEventBuffer raw = midi_notes_to_events(_midi);
		midi_streamer = new MidiEventStreamer(raw);
		midi_streamer->ignore_end = true;

		synth->set_source(midi_streamer->out);
	}else if (t->type == SignalType::BEATS){

		synth->set_source(sr->beat_midifier->out);
	}

	track->subscribe(this, [&]{ on_track_replace_synth(); }, track->MESSAGE_REPLACE_SYNTHESIZER);
	track->subscribe(this, [&]{ on_track_add_or_delete_fx(); }, track->MESSAGE_ADD_EFFECT);
	track->subscribe(this, [&]{ on_track_add_or_delete_fx(); }, track->MESSAGE_DELETE_EFFECT);
	track->subscribe(this, [&]{ on_track_change_data(); }, track->MESSAGE_CHANGE);
}

TrackRenderer::~TrackRenderer()
{
	track->unsubscribe(this);
	if (midi_streamer)
		delete midi_streamer;
	if (!direct_mode){
		for (auto *f: fx)
			delete f;
		delete synth;
	}
}

void TrackRenderer::on_track_add_or_delete_fx()
{
	msg_error("track del/rm fx");
}

void TrackRenderer::on_track_replace_synth()
{
	if (direct_mode){
		synth = track->synth;
	}else{
		delete synth;

		synth = (Synthesizer*)track->synth->copy();
		synth->setSampleRate(track->song->sample_rate);
		synth->setInstrument(track->instrument);
	}

	if (track->type == SignalType::MIDI){
		synth->set_source(midi_streamer->out);
	}else if (track->type == SignalType::BEATS){
		synth->set_source(song_renderer->beat_midifier->out);
	}
}

void TrackRenderer::on_track_change_data()
{
	if (midi_streamer){
		MidiNoteBuffer _midi = track->layers[0]->midi;
		for (TrackLayer *l: track->layers)
			for (auto c: l->samples)
				if (c->type() == SignalType::MIDI)
				_midi.append(*c->midi, c->pos); // TODO: mute/solo....argh
		midi_streamer->midi = _midi.getEvents(Range::ALL);
	}
}

void TrackRenderer::seek(int pos)
{
	offset = pos;
	if (midi_streamer)
		midi_streamer->seek(pos);
	if (track->type == SignalType::BEATS)
		song_renderer->bar_streamer->seek(pos);
}

static void copy_direct(TrackLayer *l, const Range &r, AudioBuffer &buf, const Range &cur)
{
	Range r1 = r and cur;
	AudioBuffer tbuf1;
	tbuf1.set_as_ref(buf, r1.start() - cur.start(), r1.length);
	l->read_buffers_fixed(tbuf1, r1);
}

static void apply_fade(TrackLayer *l1, TrackLayer *l2, const Range &r, AudioBuffer &buf, const Range &cur)
{
	Range r1 = r and cur;
	AudioBuffer tbuf1;
	tbuf1.set_as_ref(buf, r1.start() - cur.start(), r1.length);
	l1->read_buffers_fixed(tbuf1, r1);

	AudioBuffer tbuf2;
	tbuf2.resize(r1.length);
	l2->read_buffers_fixed(tbuf2, r1);

	// perform (linear) fade
	for (int i=r1.start(); i<r1.end(); i++){
		float a = (float)(r.end() - i) / (float)r.length;
		for (int c=0; c<buf.channels; c++)
			tbuf1.c[c][i - r1.start()] *= a;
	}
	for (int i=r1.start(); i<r1.end(); i++){
		float a = (float)(i - r.start()) / (float)r.length;
		for (int c=0; c<buf.channels; c++)
			tbuf2.c[c][i - r1.start()] *= a;
	}

	tbuf1.add(tbuf2, 0, 1, 0);
}


void TrackRenderer::render_audio_no_fx(AudioBuffer &buf)
{
	/*bool has_solo = false;
	for (auto *l: track->layers)
		if (l->)*/

	Range cur = Range(offset, buf.length);

	int index_before = 0;
	Array<Track::Fade*> fades;
	foreachi(auto &f, track->fades, i){
		Range r = f.range();
		if (r.end() <= cur.start())
			index_before = f.target;
		if (r.overlaps(cur)){
			fades.add(&f);
		}
	}
	if (fades.num > 0){
		int prev_end = offset;
		for (auto *ff: fades){
			Range r = ff->range();

			// before
			copy_direct(track->layers[index_before], RangeTo(prev_end, r.start()), buf, cur);

			apply_fade(track->layers[index_before], track->layers[ff->target], r, buf, cur);

			index_before = ff->target;
			prev_end = r.end();
		}

		// after
		auto *ff = fades.back();
		copy_direct(track->layers[ff->target], RangeTo(prev_end, cur.end()), buf, cur);

	}else{
		// direct mode
		track->layers[index_before]->read_buffers_fixed(buf, cur);
		add_samples(track->layers[index_before], cur, buf);
	}

		/*
		return;
	//}

	// any un-muted layer?
	int i0 = get_first_usable_layer(track, song_renderer->allowed_layers);
	if (i0 < 0){
		// no -> return silence
		buf.scale(0);
	}else{

		// first (un-muted) layer
		track->layers[i0]->read_buffers_fixed(buf, song_renderer->range_cur);
		// TODO: allow_ref if no other layers + no fx
		add_samples(track->layers[i0], song_renderer->range_cur, buf);

		// other layers
		AudioBuffer tbuf;
		for (int i=i0+1;i<track->layers.num;i++){
			if (!song_renderer->allowed_layers.contains(track->layers[i]))
				continue;
			if (track->layers[i]->muted)
				continue;
			track->layers[i]->readBuffers(tbuf, song_renderer->range_cur, true);
			add_samples(track->layers[i], song_renderer->range_cur, tbuf);
			buf.add(tbuf, 0, 1.0f, 0.0f);
		}
	}*/
}

void TrackRenderer::render_time_no_fx(AudioBuffer &buf)
{
	synth->out->read(buf);
}

void TrackRenderer::render_midi_no_fx(AudioBuffer &buf)
{
	synth->out->read(buf);
}

void TrackRenderer::render_no_fx(AudioBuffer &buf)
{
	if (track->type == SignalType::AUDIO)
		render_audio_no_fx(buf);
	else if (track->type == SignalType::BEATS)
		render_time_no_fx(buf);
	else if (track->type == SignalType::MIDI)
		render_midi_no_fx(buf);
	offset += buf.length;
}

void TrackRenderer::apply_fx(AudioBuffer &buf, Array<AudioEffect*> &fx_list)
{
	// apply fx
	for (AudioEffect *f: fx_list)
		if (f->enabled){
			f->process(buf);
		}
}

void TrackRenderer::render_fx(AudioBuffer &buf)
{
	render_no_fx(buf);

	Array<AudioEffect*> _fx = fx;
	if (song_renderer and song_renderer->preview_effect)
		_fx.add(song_renderer->preview_effect);
	if (_fx.num > 0)
		apply_fx(buf, _fx);
}