/*
 * FormatRaw.cpp
 *
 *  Created on: 16.02.2013
 *      Author: michi
 */

#include "FormatRaw.h"
#include "../Tsunami.h"
#include "../View/Helper/Progress.h"
#include "../Stuff/Log.h"
#include "../lib/math/math.h"
#include "Dialog/RawConfigDialog.h"


const int WAVE_BUFFER_SIZE = 1 << 15;

FormatRaw::FormatRaw() :
	Format("raw", FLAG_AUDIO | FLAG_SINGLE_TRACK)
{
}

FormatRaw::~FormatRaw()
{
}

RawConfigData GetRawConfigData()
{
	RawConfigData data;
	RawConfigDialog *dlg = new RawConfigDialog(&data, tsunami);
	dlg->Run();
	return data;
}

void FormatRaw::SaveBuffer(AudioFile *a, BufferBox *b, const string &filename)
{
	msg_db_f("write_raw_file", 1);
	tsunami->progress->Set(_("exportiere raw"), 0);

	Array<short> buf16;
	if (!b->get_16bit_buffer(buf16))
		tsunami->log->Error(_("Amplitude zu gro&s, Signal &ubersteuert."));
	char *data = (char*)buf16.data;

	CFile *f = CreateFile(filename);
	f->SetBinaryMode(true);
	/*ProgressStatus(_("exportiere wave"), 0.5f);
	f->WriteBuffer((char*)PVData,w->length*4);*/
	int size = b->num * 4;
	for (int i=0;i<size / WAVE_BUFFER_SIZE;i++){
		tsunami->progress->Set(float(i * WAVE_BUFFER_SIZE) / (float)size);
		f->WriteBuffer(&data[i * WAVE_BUFFER_SIZE], WAVE_BUFFER_SIZE);
	}
	f->WriteBuffer(&data[(size / WAVE_BUFFER_SIZE) * WAVE_BUFFER_SIZE], size & (WAVE_BUFFER_SIZE - 1));

	FileClose(f);
}

void FormatRaw::LoadTrack(Track *t, const string & filename, int offset, int level)
{
	msg_db_f("load_raw_file", 1);
	tsunami->progress->Set(_("lade raw"), 0);

	RawConfigData config = GetRawConfigData();

	char *data = new char[WAVE_BUFFER_SIZE];
	CFile *f = OpenFile(filename);

	try{

	if (!f)
		throw string("can't open file");
	f->SetBinaryMode(true);
	int byte_per_sample = (format_get_bits(config.format) / 8) * config.channels;
	int size = f->GetSize() - config.offset;
	int samples = size / byte_per_sample;
	tsunami->progress->Set(0.1f);

	if (config.offset > 0)
		f->ReadBuffer(data, config.offset);

	int read = 0;
	int nn = 0;
	int nice_buffer_size = WAVE_BUFFER_SIZE - (WAVE_BUFFER_SIZE % byte_per_sample);
	while (read < size){
		int toread = clampi(nice_buffer_size, 0, size - read);
		int r = f->ReadBuffer(data, toread);
		nn ++;
		if (nn > 16){
			float perc_read = 0.1f;
			float dperc_read = 0.9f;
			tsunami->progress->Set(perc_read + dperc_read * (float)read / (float)size);
			nn = 0;
		}
		if (r > 0){
			int dsamples = r / byte_per_sample;
			int _offset = read / byte_per_sample + offset;
			ImportData(t, data, config.channels, config.format, dsamples, _offset, level);
			read += r;
		}else{
			throw string("could not read in wave file...");
		}
	}

	}catch(const string &s){
		tsunami->log->Error(s);
	}

	delete[](data);

	if (f)
		FileClose(f);
}

void FormatRaw::SaveAudio(AudioFile *a, const string & filename)
{
	ExportAudioAsTrack(a, filename);
}



void FormatRaw::LoadAudio(AudioFile *a, const string & filename)
{
	Track *t = a->AddTrack(Track::TYPE_AUDIO);
	LoadTrack(t, filename);
}



