/*
 * SignalChain.cpp
 *
 *  Created on: 01.04.2018
 *      Author: michi
 */

#include "SignalChain.h"
#include "Module.h"
#include "ModuleFactory.h"
#include "Port/MidiPort.h"
#include "../Session.h"
#include "../Plugins/PluginManager.h"
#include "../lib/file/file.h"



SignalChain::SignalChain(Session *s)
{
	session = s;
}

SignalChain::~SignalChain()
{
	stop();
	for (Module *m: modules)
		delete m;
	for (Cable *c: cables)
		delete c;
}

SignalChain *SignalChain::create_default(Session *session)
{
	SignalChain *chain = new SignalChain(session);

	auto *mod_renderer = chain->addAudioSource("SongRenderer");
	auto *mod_peak = chain->addAudioVisualizer("PeakMeter");
	auto *mod_out = chain->addAudioOutputStream();

	chain->connect(mod_renderer, 0, mod_peak, 0);
	chain->connect(mod_peak, 0, mod_out, 0);

	return chain;
}

Module *SignalChain::add(Module *m)
{
	int i = modules.num;
	m->module_x = 50 + (i % 5) * 230;
	m->module_y = 50 + (i % 2) * 30 + 150*(i / 5);

	m->reset_state();
	modules.add(m);
	return m;
}

int SignalChain::module_index(Module *m)
{
	foreachi(Module *mm, modules, i)
		if (mm == m)
			return i;
	return -1;
}

void SignalChain::remove(Module *m)
{
	int index = module_index(m);
	if (index < 0)
		return;
	if (index < 3){
		session->e(_("not allowed to delete system modules"));
		return;
	}


	bool more = true;
	while (more){
		more = false;
		for (Cable *c: cables)
			if (c->source == m or c->target == m){
				disconnect(c);
				more = true;
				break;
			}
	}

	modules.erase(index);
	delete m;
	notify();
}

void SignalChain::connect(Module *source, int source_port, Module *target, int target_port)
{
	if (source_port < 0 or source_port >= source->port_out.num)
		throw Exception("bla");
	if (target_port < 0 or target_port >= target->port_in.num)
		throw Exception("bla");
	//msg_write("connect " + i2s(source->port_out[source_port].type) + " -> " + i2s(target->port_in[target_port].type));
	if (source->port_out[source_port].type != target->port_in[target_port].type)
		throw Exception("bla");
	// TODO: check ports in use
	Cable *c = new Cable;
	c->type = source->port_out[source_port].type;
	c->source = source;
	c->target = target;
	c->source_port = source_port;
	c->target_port = target_port;
	cables.add(c);

	*target->port_in[target_port].port = *source->port_out[source_port].port;

	notify();
}

void SignalChain::disconnect(SignalChain::Cable *c)
{
	foreachi(Cable *cc, cables, i)
		if (cc == c){
			*c->target->port_in[c->target_port].port = NULL;

			delete(c);
			cables.erase(i);
			notify();
			break;
		}
}

void SignalChain::disconnect_source(Module *source, int source_port)
{
	for (Cable *c: cables)
		if (c->source == source and c->source_port == source_port){
			disconnect(c);
			break;
		}
}

void SignalChain::disconnect_target(Module *target, int target_port)
{
	for (Cable *c: cables)
		if (c->target == target and c->target_port == target_port){
			disconnect(c);
			break;
		}
}

void SignalChain::save(const string& filename)
{
	File* f = FileCreateText(filename);
	f->write_str("1");
	f->write_int(modules.num);
	for (auto *m: modules){
		f->write_str(m->type_to_name(m->module_type));
		f->write_str(m->module_subtype);
		f->write_str("");
		f->write_str(m->config_to_string());
		f->write_int(m->module_x);
		f->write_int(m->module_y);
	}
	f->write_int(cables.num);
	for (auto *c: cables){
		f->write_int(module_index(c->source));
		f->write_int(c->source_port);
		f->write_int(module_index(c->target));
		f->write_int(c->target_port);
	}
	delete f;
}

void SignalChain::load(const string& filename)
{
	for (int i=modules.num-1; i>=3; i--)
		remove(modules[i]);
	Array<Cable*> _cables = cables;
	for (auto *c: cables)
		disconnect(c);

	try{
	File *f = FileOpenText(filename);
	f->read_str();
	int n = f->read_int();
	for (int i=0; i<n; i++){
		string type = f->read_str();
		string sub_type = f->read_str();
		string name = f->read_str();
		Module *m = NULL;
		if (i < 3){
			m = modules[i];
		}else{
			int itype = Module::type_from_name(type);
			if (itype < 0)
				throw Exception("unhandled module type: " + type);
			m = add(ModuleFactory::create(session, itype, sub_type));
		}
		string config = f->read_str();
		m->config_from_string(config);
		m->module_x = f->read_int();
		m->module_y = f->read_int();
	}
	n = f->read_int();
	for (int i=0; i<n; i++){
		int s = f->read_int();
		int sp = f->read_int();
		int t = f->read_int();
		int tp = f->read_int();
		connect(modules[s], sp, modules[t], tp);
	}
	delete f;
	}catch(Exception &e){
		session->e(e.message());
	}
}

void SignalChain::reset()
{
	for (int i=modules.num-1; i>=3; i--)
		remove(modules[i]);
	Array<Cable*> _cables = cables;
	for (auto *c: cables)
		disconnect(c);

	connect(modules[0], 0, modules[1], 0);
	connect(modules[1], 0, modules[2], 0);
}

Module* SignalChain::add(int type, const string &sub_type)
{
	return add(ModuleFactory::create(session, type, sub_type));
}

Module* SignalChain::addAudioSource(const string &name)
{
	return add(Module::Type::AUDIO_SOURCE, name);
}

Module* SignalChain::addMidiSource(const string &name)
{
	return add(Module::Type::MIDI_SOURCE, name);
}

Module* SignalChain::addAudioEffect(const string &name)
{
	return add(Module::Type::AUDIO_EFFECT, name);
}

Module* SignalChain::addAudioJoiner()
{
	return add(Module::Type::AUDIO_JOINER, "");
}

Module* SignalChain::addAudioSucker()
{
	return add(Module::Type::AUDIO_SUCKER, "");
}

Module* SignalChain::addPitchDetector()
{
	return add(Module::Type::PITCH_DETECTOR, "");
}

Module* SignalChain::addAudioVisualizer(const string &name)
{
	return add(Module::Type::AUDIO_VISUALIZER, name);
}

Module* SignalChain::addAudioInputStream()
{
	return add(Module::Type::INPUT_STREAM_AUDIO, "");
}

Module* SignalChain::addAudioOutputStream()
{
	return add(Module::Type::OUTPUT_STREAM_AUDIO, "");
}

Module* SignalChain::addMidiEffect(const string &name)
{
	return add(Module::Type::MIDI_EFFECT, name);
}

Module* SignalChain::addSynthesizer(const string &name)
{
	return add(Module::Type::SYNTHESIZER, name);
}

Module* SignalChain::addMidiInputStream()
{
	return add(Module::Type::INPUT_STREAM_MIDI, "");
}

Module* SignalChain::addBeatMidifier()
{
	return add(Module::Type::BEAT_MIDIFIER, "");
}

Module* SignalChain::addBeatSource(const string &name)
{
	return add(Module::Type::BEAT_SOURCE, name);
}

void SignalChain::reset_state()
{
	for (auto *m: modules)
		m->reset_state();
}

void SignalChain::start()
{
	reset_state();
	for (auto *m: modules)
		m->module_start();
}

void SignalChain::stop()
{
	for (auto *m: modules)
		m->module_stop();
}

void SignalChain::pause(bool paused)
{
	for (auto *m: modules)
		m->module_pause(paused);
}
