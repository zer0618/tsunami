/*
 * ActionTrackLayer__Delete.h
 *
 *  Created on: 27.08.2016
 *      Author: michi
 */

#ifndef SRC_ACTION_TRACK_LAYER_ACTIONTRACKLAYER__DELETE_H_
#define SRC_ACTION_TRACK_LAYER_ACTIONTRACKLAYER__DELETE_H_

#include "../../Action.h"

class Track;

class ActionTrackLayer__Delete : public Action
{
public:
	ActionTrackLayer__Delete(Track *t, int index);

	virtual void *execute(Data *d);
	virtual void undo(Data *d);
private:
	Track *track;
	int index;
	int type;
};

#endif /* SRC_ACTION_TRACK_LAYER_ACTIONTRACKLAYER__DELETE_H_ */
