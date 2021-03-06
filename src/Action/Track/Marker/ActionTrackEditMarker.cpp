/*
 * ActionTrackEditMarker.cpp
 *
 *  Created on: 03.10.2017
 *      Author: michi
 */

#include "ActionTrackEditMarker.h"
#include "../../../Data/Track.h"
#include "../../../Data/TrackMarker.h"
#include <assert.h>

ActionTrackEditMarker::ActionTrackEditMarker(const Track *t, TrackMarker *m, const Range &_range, const string &_text)
{
	track = t;
	marker = m;
	range = _range;
	text = _text;
}

void *ActionTrackEditMarker::execute(Data *d)
{
	string temp = text;
	text = marker->text;
	marker->text = temp;

	Range r = range;
	range = marker->range;
	marker->range = r;

	track->notify();
	return nullptr;
}

void ActionTrackEditMarker::undo(Data *d)
{
	execute(d);
}

