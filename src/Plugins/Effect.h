/*
 * Effect.h
 *
 *  Created on: 10.12.2012
 *      Author: michi
 */

#ifndef EFFECT_H_
#define EFFECT_H_

#include "../lib/base/base.h"
#include "../Data/Range.h"

class Plugin;
class Track;
class BufferBox;

namespace Script{
class Script;
class Type;
};

class EffectParam
{
public:
	string name, type;
	string value;
};

class Effect : public VirtualBase
{
public:
	Effect();
	Effect(Plugin *p);
	virtual ~Effect();

	void __init__();
	virtual void __delete__();

	string name;
	Array<EffectParam> param;
	bool only_on_selection;
	Range range;
	Plugin *plugin;
	bool usable;

	virtual void ProcessTrack(BufferBox *buf){};
	virtual void ResetConfig(){};
	virtual void ResetState(){};
	virtual void Configure(){};
	virtual void UpdateDialog(){};


	void DoProcessTrack(Track *t, int level_no, const Range &r);
	bool DoConfigure(bool previewable);

	void ConfigToString();
	void ConfigFromString();
	void Prepare();
	void CleanUp();
	void Apply(BufferBox &buf, Track *t, bool log_error);

	void WriteConfigToFile(const string &name);
	void LoadConfigFromFile(const string &name);

	string GetError();
};

Effect *CreateEffect(const string &name);

#endif /* EFFECT_H_ */
