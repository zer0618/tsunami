/*
 * TrackMarker.h
 *
 *  Created on: 06.09.2018
 *      Author: michi
 */

#ifndef SRC_DATA_TRACKMARKER_H_
#define SRC_DATA_TRACKMARKER_H_

#include "Range.h"

class AudioEffect;


class TrackMarker
{
public:
	Range range;
	string text;
	Array<AudioEffect*> fx;
};



#endif /* SRC_DATA_TRACKMARKER_H_ */
