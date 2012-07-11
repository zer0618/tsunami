/*
 * ActionTrackEditBuffer.h
 *
 *  Created on: 30.03.2012
 *      Author: michi
 */

#ifndef ACTIONTRACKEDITBUFFER_H_
#define ACTIONTRACKEDITBUFFER_H_

#include "../Action.h"
#include "../../Data/Track.h"

class ActionTrackEditBuffer : public Action
{
public:
	ActionTrackEditBuffer(Track *t, int _level_no, Range _range);
	virtual ~ActionTrackEditBuffer();

	virtual void *execute(Data *d);
	virtual void undo(Data *d);
	virtual void redo(Data *d);

private:
	int track_no, sub_no;
	Range range;
	int level_no;
	BufferBox box;
	int index;
};

#endif /* ACTIONTRACKEDITBUFFER_H_ */
