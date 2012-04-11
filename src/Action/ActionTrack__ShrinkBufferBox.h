/*
 * ActionTrack__ShrinkBufferBox.h
 *
 *  Created on: 09.04.2012
 *      Author: michi
 */

#ifndef ACTIONTRACK__SHRINKBUFFERBOX_H_
#define ACTIONTRACK__SHRINKBUFFERBOX_H_

#include "Action.h"
#include "../Data/Track.h"

class ActionTrack__ShrinkBufferBox : public Action
{
public:
	ActionTrack__ShrinkBufferBox(Track *t, int _index, int _length);
	virtual ~ActionTrack__ShrinkBufferBox();

	void *execute(Data *d);
	void undo(Data *d);

private:
	int track_no, sub_no;
	int index;
	int old_length, new_length;
	BufferBox buf;
};

#endif /* ACTIONTRACK__SHRINKBUFFERBOX_H_ */