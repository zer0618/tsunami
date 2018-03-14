/*
 * ActionTrackEditMarker.h
 *
 *  Created on: 03.10.2017
 *      Author: michi
 */

#ifndef SRC_ACTION_TRACK_MARKER_ACTIONTRACKEDITMARKER_H_
#define SRC_ACTION_TRACK_MARKER_ACTIONTRACKEDITMARKER_H_

#include "../../Action.h"
#include "../../../Data/Range.h"
class Track;

class ActionTrackEditMarker: public Action
{
public:
	ActionTrackEditMarker(Track *t, int index, const Range &range, const string &text);
	virtual ~ActionTrackEditMarker(){}

	virtual void *execute(Data *d);
	virtual void undo(Data *d);

private:
	int index;
	Range range;
	string text;
	int track_no;
};

#endif /* SRC_ACTION_TRACK_MARKER_ACTIONTRACKEDITMARKER_H_ */
