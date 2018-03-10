/*
 * MiniBar.cpp
 *
 *  Created on: 23.03.2014
 *      Author: michi
 */

#include "BottomBar.h"
#include "../Helper/PeakMeter.h"
#include "../Helper/CpuDisplay.h"
#include "../../Device/OutputStream.h"
#include "../../Device/DeviceManager.h"
#include "../../Session.h"
#include "../AudioView.h"
#include "MiniBar.h"

MiniBar::MiniBar(BottomBar *_bottom_bar, Session *_session)
{
	session = _session;
	view = session->view;
	dev_manager = session->device_manager;
	bottom_bar = _bottom_bar;

	fromResource("mini_bar");

	peak_meter = new PeakMeter(this, "peaks", view->stream, view);
	setFloat("volume", dev_manager->getOutputVolume());

	cpu_display = new CpuDisplay(this, "cpu", session);

	event("show_bottom_bar", std::bind(&MiniBar::onShowBottomBar, this));
	event("volume", std::bind(&MiniBar::onVolume, this));

	bottom_bar->subscribe(this, std::bind(&MiniBar::onBottomBarUpdate, this));
	dev_manager->subscribe(this, std::bind(&MiniBar::onVolumeChange, this));
	view->subscribe(this, std::bind(&MiniBar::onViewOutputChange, this), view->MESSAGE_OUTPUT_CHANGE);
}

MiniBar::~MiniBar()
{
	view->unsubscribe(this);
	dev_manager->unsubscribe(this);
	bottom_bar->unsubscribe(this);
	delete(peak_meter);
	delete(cpu_display);
}

void MiniBar::onShowBottomBar()
{
	bottom_bar->_show();
	hide();
}

void MiniBar::onVolume()
{
	dev_manager->setOutputVolume(getFloat(""));
}

void MiniBar::onShow()
{
	peak_meter->enable(true);
}

void MiniBar::onHide()
{
	peak_meter->enable(false);
}

void MiniBar::onBottomBarUpdate()
{
	if (bottom_bar->visible)
		hide();
	else
		show();
}

void MiniBar::onVolumeChange()
{
	setFloat("volume", dev_manager->getOutputVolume());
}

void MiniBar::onViewOutputChange()
{
	peak_meter->setSource(view->stream);
}

