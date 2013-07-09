/*
 * ActionTrack__DeleteBufferBox.h
 *
 *  Created on: 09.04.2012
 *      Author: michi
 */

#ifndef ACTIONTRACK__DELETEBUFFERBOX_H_
#define ACTIONTRACK__DELETEBUFFERBOX_H_

#include "../../Action.h"
#include "../../../Data/Track.h"

class ActionTrack__DeleteBufferBox : public Action
{
public:
	ActionTrack__DeleteBufferBox(Track *t, int level_no, int _index);
	virtual ~ActionTrack__DeleteBufferBox();

	virtual void *execute(Data *d);
	virtual void undo(Data *d);

private:
	int track_no, sub_no;
	int level_no;
	int index;
	BufferBox buf;
};

#endif /* ACTIONTRACK__DELETEBUFFERBOX_H_ */