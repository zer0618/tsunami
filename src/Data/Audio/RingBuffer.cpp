/*
 * RingBuffer.cpp
 *
 *  Created on: 26.01.2015
 *      Author: michi
 */

#include "RingBuffer.h"

RingBuffer::RingBuffer(int size)
{
	buf.resize(size);
	read_pos = 0;
	write_pos = 0;
}

RingBuffer::~RingBuffer()
{
}

void RingBuffer::clear()
{
	write_pos = 0;
	read_pos = 0;
}

// how many samples are readable?
int RingBuffer::available()
{
	if (write_pos < read_pos)
		return write_pos - read_pos + buf.length;
	return write_pos - read_pos;
}

// how many samples are writable?
int RingBuffer::writable_size()
{
	if (read_pos <= write_pos)
		return read_pos - write_pos + buf.length;
	return read_pos - write_pos;
}

void RingBuffer::moveReadPos(int delta)
{
	read_pos += delta;
	if (read_pos >= buf.length)
		read_pos -= buf.length;
}
void RingBuffer::moveWritePos(int delta)
{
	write_pos += delta;
	if (write_pos >= buf.length)
		write_pos -= buf.length;
}

int RingBuffer::read(AudioBuffer& b)
{
	int samples = min(b.length, available());

	int samples_a = min(samples, buf.length - read_pos);
	b.set_x(buf, -read_pos, read_pos + samples_a, 1.0f);
	moveReadPos(samples_a);

	int samples_b = samples - samples_a;
	if (samples_b == 0)
		return samples_a;

	AudioBuffer bb;
	bb.set_as_ref(b, samples_a,  samples_b);
	bb.set_x(buf, 0, samples_b, 1.0f);
	moveReadPos(samples_b);
	return samples;
}

void RingBuffer::write(AudioBuffer& b)
{
	int samples = min(b.length, writable_size());

	int size_a = min(samples, buf.length - write_pos);
	buf.set(b, write_pos, 1.0f);

	int size_b = samples - size_a;
	if (size_b > 0){
		buf.set(b, -size_a, 1.0f);
	}

	moveWritePos(samples);
}

void RingBuffer::peekRef(AudioBuffer &b, int size)
{
	if (size >= 0){
		size = min(size, available());
		size = min(size, buf.length - read_pos);
		b.set_as_ref(buf, read_pos, size);
	}else{
		size = min(-size, write_pos);
		b.set_as_ref(buf, write_pos - size, size);
	}
}

void RingBuffer::readRef(AudioBuffer &b, int size)
{
	peekRef(b, size);
	moveReadPos(b.length);
}

void RingBuffer::writeRef(AudioBuffer &b, int size)
{
	size = min(size, buf.length - write_pos);
	b.set_as_ref(buf, write_pos, size);
	moveWritePos(b.length);
}