/*
 * ActionTrackEditMarker.cpp
 *
 *  Created on: 03.10.2017
 *      Author: michi
 */

#include "ActionTrackEditMarker.h"
#include "../../../Data/Track.h"
#include <assert.h>

ActionTrackEditMarker::ActionTrackEditMarker(Track *t, int _index, const string &_text)
{
	track_no = get_track_index(t);
	index = _index;
	text = _text;
}

void *ActionTrackEditMarker::execute(Data *d)
{
	Song *a = dynamic_cast<Song*>(d);

	Track *t = a->get_track(track_no);

	assert(index >= 0);
	assert(index < t->markers.num);

	string temp = text;
	text = t->markers[index]->text;
	t->markers[index]->text = temp;

	return NULL;
}

void ActionTrackEditMarker::undo(Data *d)
{
	execute(d);
}
