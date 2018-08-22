/*
 * ViewPort.cpp
 *
 *  Created on: 21.06.2015
 *      Author: michi
 */

#include "ViewPort.h"
#include "AudioView.h"
#include "../lib/math/math.h"

const float ViewPort::BORDER_FACTOR = 1.0f / 15.0f;

ViewPort::ViewPort(AudioView *v)
{
	view = v;
	scale = 1.0f;
	pos = 0;
	pos_target = 0;
	pos_pre_animation = 0;
	animation_time = -1;
	animation_non_linearity = 0;
}


double ViewPort::screen2sample(double _x)
{
	return (_x - view->area.x1) / scale + pos;
}

double ViewPort::sample2screen(double s)
{
	return view->area.x1 + (s - pos) * scale;
}

double ViewPort::dsample2screen(double ds)
{
	return ds * scale;
}

void ViewPort::zoom(float f)
{
	// max zoom: 20 pixel per sample
	double scale_new = clampf(scale * f, 0.000001, 20.0);

	pos += (view->mx - view->area.x1) * (1.0/scale - 1.0/scale_new);
	pos_target = pos_pre_animation = pos;
	scale = scale_new;
	view->notify(view->MESSAGE_VIEW_CHANGE);
	view->forceRedraw();
}

void ViewPort::move(float dpos)
{
	/*pos += dpos;
	view->notify(view->MESSAGE_VIEW_CHANGE);
	view->forceRedraw();*/
	set_target(pos_target + dpos, 0.0f);
}

// nonlin=0   feels "faster", more responsive, good for continuous human controls
// nonlin=0.7 feels smoother, good for automatic single jumps
void ViewPort::set_target(float target, float nonlin)
{
	pos_pre_animation = pos;
	pos_target = target;
	animation_time = 0;
	animation_non_linearity = nonlin;
	view->notify(view->MESSAGE_VIEW_CHANGE);
	view->forceRedraw();
}

void ViewPort::update(float dt)
{
	if (animation_time < 0)
		return;
	animation_time += dt;
	double t = animation_time;
	if (t >= 1){
		pos = pos_target;
		animation_time = -1;
	}else{
		float s = animation_non_linearity;
		t = s*(-2*t*t*t + 3*t*t) + (1-s) * t;
		pos = t * pos_target + (1-t) * pos_pre_animation;
	}
	view->notify(view->MESSAGE_VIEW_CHANGE);
	view->forceRedraw();
}

bool ViewPort::needs_update()
{
	return (animation_time >= 0);
}

Range ViewPort::range()
{
	return Range(pos, view->area.width() / scale);
}

void ViewPort::make_sample_visible(int sample)
{
	double x = sample2screen(sample);
	float dx = view->area.width() * BORDER_FACTOR;
	if ((x > view->area.x2 - dx) or (x < view->area.x1 + dx)){
		//pos = sample - view->area.width() / scale * BORDER_FACTOR;
		set_target(sample - view->area.width() / scale * BORDER_FACTOR, 0.7f);
		//view->notify(view->MESSAGE_VIEW_CHANGE);
		//view->forceRedraw();
	}
}

void ViewPort::show(Range &r)
{
	int border = r.length * BORDER_FACTOR;
	r.offset -= border;
	r.length += border * 2;
	scale = view->area.width() / (double)r.length;
	pos = (double)r.start();
	pos_pre_animation = pos;
	pos_target = pos;
	view->notify(view->MESSAGE_VIEW_CHANGE);
	view->forceRedraw();
}

