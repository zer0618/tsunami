/*
 * ControlGroup.h
 *
 *  Created on: 17.06.2013
 *      Author: michi
 */

#ifndef CONTROLGROUP_H_
#define CONTROLGROUP_H_

#include "Control.h"

namespace hui
{


class ControlGroup : public Control
{
public:
	ControlGroup(const string &text, const string &id);

	virtual void add(Control *child, int x, int y);
};

};

#endif /* CONTROLGROUP_H_ */