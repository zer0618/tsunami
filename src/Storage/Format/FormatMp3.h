/*
 * FormatMp3.h
 *
 *  Created on: 18.09.2014
 *      Author: michi
 */

#ifndef FORMATMP3_H_
#define FORMATMP3_H_

#include "Format.h"

class FormatMp3: public Format
{
public:
	void load_track(StorageOperationData *od);
	void save_via_renderer(StorageOperationData *od){}
};

class FormatDescriptorMp3 : public FormatDescriptor
{
public:
	FormatDescriptorMp3();
	virtual Format *create(){ return new FormatMp3; }
};

#endif /* FORMATMP3_H_ */
