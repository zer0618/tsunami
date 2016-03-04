/*
 * FormatWave.h
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#ifndef FORMATWAVE_H_
#define FORMATWAVE_H_

#include "Format.h"

class FormatWave: public Format
{
public:
	virtual void loadTrack(StorageOperationData *od);
	virtual void saveViaRenderer(StorageOperationData *od);
};

class FormatDescriptorWave : public FormatDescriptor
{
public:
	FormatDescriptorWave();
	virtual Format *create(){ return new FormatWave; }
};

#endif /* FORMATWAVE_H_ */
