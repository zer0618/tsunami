/*
 * SongConsole.cpp
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#include "../../Session.h"
#include "../../View/AudioView.h"
#include "../BottomBar/BottomBar.h"
#include "../../Data/Song.h"
#include "../../Data/base.h"
#include "SongConsole.h"

const int NUM_POSSIBLE_FORMATS = 4;
const SampleFormat POSSIBLE_FORMATS[NUM_POSSIBLE_FORMATS] = {
	SampleFormat::SAMPLE_FORMAT_16,
	SampleFormat::SAMPLE_FORMAT_24,
	SampleFormat::SAMPLE_FORMAT_32,
	SampleFormat::SAMPLE_FORMAT_32_FLOAT
};

SongConsole::SongConsole(Session *session) :
	SideBarConsole(_("File properties"), session)
{
	// dialog
	set_border_width(5);
	embed_dialog("song_dialog", 0, 0);
	set_decimals(1);

	expand("ad_t_tags", 0, true);

	add_string("samplerate", "22050");
	add_string("samplerate", i2s(DEFAULT_SAMPLE_RATE));
	add_string("samplerate", "48000");
	add_string("samplerate", "96000");

	for (int i=0; i<NUM_POSSIBLE_FORMATS; i++)
		add_string("format", format_name(POSSIBLE_FORMATS[i]));

	load_data();

	event("samplerate", [=]{ on_samplerate(); });
	event("format", [=]{ on_format(); });
	event("compress", [=]{ on_compression(); });
	event_x("tags", "hui:select", [=]{ on_tags_select(); });
	event_x("tags", "hui:change", [=]{ on_tags_edit(); });
	event("add_tag", [=]{ on_add_tag(); });
	event("delete_tag", [=]{ on_delete_tag(); });

	event("edit_samples", [=]{ on_edit_samples(); });

	song->subscribe(this, [=]{ on_update(); });
}

SongConsole::~SongConsole()
{
	song->unsubscribe(this);
}

void SongConsole::load_data()
{
	// tags
	reset("tags");
	for (Tag &t: song->tags)
		add_string("tags", t.key + "\\" + t.value);
	enable("delete_tag", false);

	// data
	reset("data_list");
	int samples = song->range().length;
	add_string("data_list", _("Start") + "\\" + song->get_time_str_long(song->range().start()));
	add_string("data_list", _("End") + "\\" + song->get_time_str_long(song->range().end()));
	add_string("data_list", _("Length") + "\\" + song->get_time_str_long(samples));
	add_string("data_list", _("Samples") + "\\" + i2s(samples));
	//addString("data_list", _("Samplerate") + "\\ + i2s(audio->sample_rate) + " Hz");

	set_string("samplerate", i2s(song->sample_rate));
	for (int i=0; i<NUM_POSSIBLE_FORMATS; i++)
		if (song->default_format == POSSIBLE_FORMATS[i])
			set_int("format", i);
	check("compress", song->compression > 0);
}

void SongConsole::on_samplerate()
{
	song->set_sample_rate(get_string("")._int());
}

void SongConsole::on_format()
{
	int i = get_int("");
	if (i >= 0)
		song->set_default_format(POSSIBLE_FORMATS[i]);
}

void SongConsole::on_compression()
{
	song->set_compression(is_checked("") ? 1 : 0);
}

void SongConsole::on_tags_select()
{
	int s = get_int("tags");
	enable("delete_tag", s >= 0);
}

void SongConsole::on_tags_edit()
{
	int r = hui::GetEvent()->row;
	if (r < 0)
		return;
	Tag t = song->tags[r];
	if (hui::GetEvent()->column == 0)
		t.key = get_cell("tags", r, 0);
	else
		t.value = get_cell("tags", r, 1);
	song->edit_tag(r, t.key, t.value);
}

void SongConsole::on_add_tag()
{
	song->add_tag("key", "value");
}

void SongConsole::on_delete_tag()
{
	int s = get_int("tags");
	if (s >= 0)
		song->delete_tag(s);
}

void SongConsole::on_edit_samples()
{
	bar()->open(SideBar::SAMPLE_CONSOLE);
}

void SongConsole::on_update()
{
	load_data();
}
