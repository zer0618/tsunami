/*
 * Module.cpp
 *
 *  Created on: 05.01.2014
 *      Author: michi
 */


#include "Module.h"
#include "ConfigPanel.h"
#include "AutoConfigPanel.h"
#include "ModuleConfiguration.h"
#include "ModuleFactory.h"
#include "Port/Port.h"
#include "../Session.h"
#include "../lib/kaba/kaba.h"
#include "../lib/kaba/syntax/Class.h"
#include "../Plugins/PluginManager.h"
#include "../Plugins/Plugin.h"
#include "../View/AudioView.h"

const string Module::MESSAGE_CHANGE_BY_ACTION = "ChangeByAction";
const string Module::MESSAGE_STATE_CHANGE = "StateChange";
const string Module::MESSAGE_TICK = "Tick";
const string Module::MESSAGE_READ_END_OF_STREAM = "ReadEndOfStream";
const string Module::MESSAGE_PLAY_END_OF_STREAM = "PlayEndOfStream";



Module::Module(ModuleType type, const string &sub_type)
{
	module_type = type;
	module_subtype = sub_type;
	session = Session::GLOBAL;
	usable = true;
	plugin = nullptr;
	enabled = true;
	module_x = module_y = 0;
	allow_config_in_chain = false;
}

Module::~Module()
{
	// unlink sources
	for (auto &pd: port_in)
		*pd.port = nullptr;

	for (Module* c: children)
		delete c;

	for (auto *p: port_out)
		delete p;
}


void Module::__init__(ModuleType type, const string &sub_type)
{
	new(this) Module(type, sub_type);
}

void Module::__delete__()
{
	this->Module::~Module();
}


// internal use for creation
void Module::set_session_etc(Session *_session, const string &sub_type, Plugin *_plugin)
{
	session = _session;
	module_subtype = sub_type;
	plugin = _plugin;
	if (plugin)
		usable = plugin->usable(session);

	reset_config();
	reset_state();
}


// default version:
//   look for a Module.config in the plugin class
ModuleConfiguration *Module::get_config() const
{
	const Kaba::Class *c = Kaba::GetDynamicType(this);
	if (!c)
		return nullptr;
	for (auto &e: c->elements)
		if ((e.name == "config") and (e.type->get_root()->name == "PluginData")){
			ModuleConfiguration *config = (ModuleConfiguration*)((char*)this + e.offset);
			config->_class = e.type;
			return config;
		}
	return nullptr;
}


string Module::config_to_string() const
{
	ModuleConfiguration *config = get_config();
	if (!config)
		return "";

	string s = var_to_string(config->_class, (char*)config);
	return s;
}


void Module::config_from_string(const string &param)
{
	ModuleConfiguration *config = get_config();
	if (!config)
		return;

	config->reset();
	int pos = 0;
	var_from_string(config->_class, (char*)config, param, pos, session);
	on_config();
}


// default version
//   try to execute   Module.config.reset()
void Module::reset_config()
{
	ModuleConfiguration *config = get_config();
	if (config)
		config->reset();
	on_config();
}


// default version
//   try to create an AutoConfigPanel
ConfigPanel *Module::create_panel()
{
	ModuleConfiguration *config = get_config();
	if (!config)
		return nullptr;
	auto aa = get_auto_conf(config, session);
	if (aa.num == 0)
		return nullptr;
	return new AutoConfigPanel(aa, this);
}


string Module::get_error()
{
	if (plugin)
		return plugin->get_error();
	return format(_("Can't load %s: \"%s\""), type_to_name(module_type).c_str(), module_subtype.c_str());
}


// called by the ConfigPanel to signal a config change
void Module::changed()
{
	on_config();
	notify();
}


Module *Module::copy() const
{
	Module *clone = ModuleFactory::create(session, module_type, module_subtype);
	clone->config_from_string(config_to_string());
	return clone;
}


string Module::type_to_name(ModuleType type)
{
	if (type == ModuleType::AUDIO_SOURCE)
		return "AudioSource";
	if (type == ModuleType::AUDIO_EFFECT)
		return "AudioEffect";
	if (type == ModuleType::SYNTHESIZER)
		return "Synthesizer";
	if (type == ModuleType::MIDI_SOURCE)
		return "MidiSource";
	if (type == ModuleType::MIDI_EFFECT)
		return "MidiEffect";
	if (type == ModuleType::BEAT_SOURCE)
		return "BeatSource";
	if (type == ModuleType::AUDIO_VISUALIZER)
		return "AudioVisualizer";
	if (type == ModuleType::PITCH_DETECTOR)
		return "PitchDetector";
	if (type == ModuleType::STREAM)
		return "Stream";
	if (type == ModuleType::PLUMBING)
		return "Plumbing";
	if (type == ModuleType::TSUNAMI_PLUGIN)
		return "TsunamiPlugin";
	return "???";
}


ModuleType Module::type_from_name(const string &str)
{
	if (str == "AudioSource")
		return ModuleType::AUDIO_SOURCE;
	if (str == "Plumbing")
		return ModuleType::PLUMBING;
	if (str == "Stream")
		return ModuleType::STREAM;
	if (str == "AudioEffect" or str == "Effect")
		return ModuleType::AUDIO_EFFECT;
	if (str == "Synthesizer" or str == "Synth")
		return ModuleType::SYNTHESIZER;
	if (str == "MidiEffect")
		return ModuleType::MIDI_EFFECT;
	if (str == "MidiSource")
		return ModuleType::MIDI_SOURCE;
	if (str == "BeatSource")
		return ModuleType::BEAT_SOURCE;
	if (str == "PitchDetector")
		return ModuleType::PITCH_DETECTOR;
	if (str == "AudioVisualizer")
		return ModuleType::AUDIO_VISUALIZER;
	return (ModuleType)-1;
}


void Module::plug(int in_port, Module* source, int out_port)
{
	if (in_port < 0 or in_port >= port_in.num)
		throw Exception("invalid in-port");
	if (out_port < 0 or out_port >= source->port_out.num)
		throw Exception("invalid out-port");
	//msg_write("connect " + i2s(source->port_out[source_port].type) + " -> " + i2s(target->port_in[target_port].type));
	if (source->port_out[out_port]->type != port_in[in_port].type)
		throw Exception("port type mismatch");
	// TODO: check ports in use

	Port *p = source->port_out[out_port];
	*port_in[in_port].port = nullptr;
	*port_in[in_port].port = p;
}


void Module::unplug(int in_port)
{
	if (in_port < 0 or in_port >= port_in.num)
		throw Exception("invalid in-port");
	*port_in[in_port].port = nullptr;
}
