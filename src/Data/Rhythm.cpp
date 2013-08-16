/*
 * Rhythm.cpp
 *
 *  Created on: 23.02.2013
 *      Author: michi
 */

#include "Rhythm.h"

Beat::Beat(int p, int bar, int beat)
{
	pos = p;
	bar_no = bar;
	beat_no = beat;
};

Bar::Bar(const Range &r, int _num_beats)
{
	range = r;
	num_beats = _num_beats;
}

Array<Beat> BarCollection::GetBeats(const Range &r)
{
	Array<Beat> beats;

	int pos0 = 0;
	int bar_no = 0;
	foreach(BarPattern &b, *this)
		if (b.type == b.TYPE_BAR){
			for (int j=0;j<b.count;j++){
				for (int i=0;i<b.num_beats;i++){
					int pos = pos0 + i * b.length / b.num_beats;
					if (r.is_inside(pos))
						beats.add(Beat(pos, bar_no, i));
				}
				pos0 += b.length;
				bar_no ++;
			}
		}else if (b.type == b.TYPE_PAUSE){
			pos0 += b.length;
		}
	return beats;
}

Array<Bar> BarCollection::GetBars(const Range &r)
{
	Array<Bar> bars;

	int pos0 = 0;
	int bar_no = 0;
	foreach(BarPattern &b, *this)
		if (b.type == b.TYPE_BAR){
			for (int j=0;j<b.count;j++){
				Range rr = Range(pos0, b.length);
				if (rr.overlaps(r))
					bars.add(Bar(rr, b.num_beats));
				pos0 += b.length;
				bar_no ++;
			}
		}else if (b.type == b.TYPE_PAUSE){
			pos0 += b.length;
		}
	return bars;
}

int BarCollection::GetNextBeat(int pos)
{
	int p0 = 0;
	if (p0 > pos)
		return p0;
	foreach(BarPattern &b, *this){
		if (b.type == b.TYPE_BAR){
			for (int i=0;i<b.count;i++){
				int pp = p0;
				for (int j=0;j<b.num_beats;j++){
					pp += b.length / b.num_beats;
					if (pp > pos)
						return pp;
				}
				p0 += b.length;
			}
		}else if (b.type == b.TYPE_PAUSE){
			p0 += b.length;
			if (p0 > pos)
				return p0;
		}
	}
	return pos;
}

Range BarCollection::GetRange()
{
	int pos0 = 0;
	foreach(BarPattern &b, *this)
		if (b.type == b.TYPE_BAR){
			pos0 += b.length * b.count;
		}else if (b.type == b.TYPE_PAUSE){
			pos0 += b.length;
		}
	return Range(0, pos0);
}





