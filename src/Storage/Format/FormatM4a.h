/*
 * FormatM4a.h
 *
 *  Created on: 21.09.2014
 *      Author: michi
 */

#ifndef FORMATM4A_H_
#define FORMATM4A_H_

#include "Format.h"

class FormatM4a: public Format
{
public:
	virtual void load_track(StorageOperationData *od);
	virtual void save_via_renderer(StorageOperationData *od){}
};

class FormatDescriptorM4a : public FormatDescriptor
{
public:
	FormatDescriptorM4a();
	virtual Format *create(){ return new FormatM4a; }
};

#endif /* FORMATMP3_H_ */
