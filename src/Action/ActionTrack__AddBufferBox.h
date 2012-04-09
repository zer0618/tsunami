/*
 * ActionTrack__AddBufferBox.h
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#ifndef ACTIONTRACK__ADDBUFFERBOX_H_
#define ACTIONTRACK__ADDBUFFERBOX_H_

#include "Action.h"
#include "../Data/Track.h"

class ActionTrack__AddBufferBox : public Action
{
public:
	ActionTrack__AddBufferBox(Track *t, int index, int _pos, int _length);
	virtual ~ActionTrack__AddBufferBox();

	void *execute(Data *d);
	void undo(Data *d);

private:
	int track_no, sub_no;
	int pos, length, index;
};

#endif /* ACTIONTRACK__ADDBUFFERBOX_H_ */
