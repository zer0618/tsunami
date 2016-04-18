/*
 * AudioInputAudio.h
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#ifndef AUDIOINPUTAUDIO_H_
#define AUDIOINPUTAUDIO_H_

#include "../lib/base/base.h"
#include "../lib/hui/hui.h"
#include "../Data/Song.h"
#include "../Data/RingBuffer.h"
#include "../View/Helper/PeakMeter.h"

struct pa_stream;
class PluginManager;
class Device;

class AudioInputAudio : public PeakMeterSource
{
	friend class PluginManager;
public:
	AudioInputAudio(int sample_rate);
	virtual ~AudioInputAudio();

	void _cdecl __init__(int sample_rate);
	virtual void _cdecl __delete__();

	static const string MESSAGE_CAPTURE;

	void _startUpdate();
	void _stopUpdate();
	void update();

	void _cdecl setDevice(Device *device);
	Device* _cdecl getDevice();

	bool _cdecl start();
	void _cdecl stop();

	int _cdecl getDelay();
	void _cdecl resetSync();

	int doCapturing();


	bool _cdecl isCapturing();


	void _cdecl accumulate(bool enable);
	void _cdecl resetAccumulation();
	int _cdecl getSampleCount();

	virtual float _cdecl getSampleRate();
	virtual void _cdecl getSomeSamples(BufferBox &buf, int num_samples);
	virtual int _cdecl getState();

	static float getPlaybackDelayConst();
	static void setPlaybackDelayConst(float f);

	static string getDefaultTempFilename();
	static string getTempFilename();
	static void setTempFilename(const string &filename);
	void setSaveMode(bool enable);

	static string temp_filename;

	void _cdecl setChunkSize(int size);
	int chunk_size;
	void _cdecl setUpdateDt(float dt);
	float update_dt;


	RingBuffer current_buffer;
	BufferBox buffer;

private:

	int sample_rate;
	bool accumulating;
	bool capturing;

	bool running;
	int hui_runner_id;

	int num_channels;

	Device *device;

	pa_stream *_stream;

	static bool testError(const string &msg);

	File *temp_file;
	static string cur_temp_filename;
	bool save_mode;

	struct SyncData
	{
		int num_points;
		long long int delay_sum;
		int samples_in, offset_out;

		void reset();
		void add(int samples);
		int getDelay();
	};
	SyncData sync;

	static float playback_delay_const;

	static void input_request_callback(pa_stream *p, size_t nbytes, void *userdata);
};

#endif /* AUDIOINPUTAUDIO_H_ */
