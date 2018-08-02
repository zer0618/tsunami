/*
 * BarStreamer.cpp
 *
 *  Created on: 07.10.2017
 *      Author: michi
 */

#include "BarStreamer.h"

#include "../../Data/Rhythm/Beat.h"

BarStreamer::BarStreamer(BarCollection &_bars)
{
	bars = _bars;
	offset = 0;
	_cur_beat = 0;
	_beat_fraction = 0;
}

int BarStreamer::read(Array<Beat> &beats, int samples)
{
	beats = bars.get_beats(Range(offset, samples), false, false);
	for (Beat &b: beats)
		if (b.range.offset >= offset){
			b.range.offset -= offset;
			_cur_beat = b.beat_no;
			_beat_fraction = 0;
		}
	offset += samples;
	return samples;
}

void BarStreamer::seek(int pos)
{
	offset = pos;
}

void BarStreamer::reset()
{
	offset = 0;
	_cur_beat = 0;
	_beat_fraction = 0;
}

int BarStreamer::cur_beat()
{
	return _cur_beat;
}

float BarStreamer::beat_fraction()
{
	return _beat_fraction;
}
