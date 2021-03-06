/*
 * TrackHeader.cpp
 *
 *  Created on: 08.06.2019
 *      Author: michi
 */

#include "TrackHeader.h"
#include "ViewNode.h"
#include "AudioViewTrack.h"
#include "../AudioView.h"
#include "../MouseDelayPlanner.h"
#include "../../Data/base.h"
#include "../../Data/Song.h"
#include "../../Data/Track.h"
#include "../../Data/TrackLayer.h"
#include "../../Session.h"







class TrackHeaderButton : public ViewNode {
public:
	AudioViewTrack *vtrack;
	TrackHeader *header;
	TrackHeaderButton(TrackHeader *th, float dx, float dy) : ViewNode(th, dx, dy, 16, 16) {
		vtrack = th->vtrack;
		header = th;
	}
	HoverData get_hover_data(float mx, float my) override {
		auto h = header->get_hover_data(mx, my);
		h.node = this;
		return h;
	}
	color get_color() {
		if (view_hover(view->hover))
			return view->colors.text;
		return ColorInterpolate(view->colors.text, view->colors.hover, 0.3f);
	}
};

class TrackButtonMute: public TrackHeaderButton {
public:
	TrackButtonMute(TrackHeader *th, float dx, float dy) : TrackHeaderButton(th, dx, dy) {}
	void draw(Painter *c) override {
		auto *track = vtrack->track;

		c->set_color(get_color());
		//c->drawStr(area.x1, area.y1-2, "\U0001f50a"); // U+1F50A "🔊"
		c->draw_mask_image(area.x1, area.y1, *view->images.speaker);
		if (track->muted)
			c->draw_image(area.x1, area.y1, *view->images.x);
	}
	bool on_left_button_down() override {
		vtrack->set_muted(!vtrack->track->muted);
		return true;
	}
	string get_tip() override {
		return _("toggle mute");
	}
};

class TrackButtonSolo: public TrackHeaderButton {
public:
	TrackButtonSolo(TrackHeader *th, float dx, float dy) : TrackHeaderButton(th, dx, dy) {}
	void draw(Painter *c) override {
		c->set_color(get_color());
		//c->drawStr(area.x1 + 5 + 17, area.y1 + 22-2, "S");
		c->draw_mask_image(area.x1, area.y1, *view->images.solo);
	}
	bool on_left_button_down() override {
		vtrack->set_solo(!vtrack->solo);
		return true;
	}
	string get_tip() override {
		return _("toggle solo");
	}
};

class TrackButtonConfig: public TrackHeaderButton {
public:
	TrackButtonConfig(TrackHeader *th, float dx, float dy) : TrackHeaderButton(th, dx, dy) {}
	void draw(Painter *c) override {
		c->set_color(get_color());
		c->draw_str(area.x1, area.y1, u8"\U0001f527"); // U+1F527 "🔧"

		/*c->setColor(col_but);
		if ((view->hover.track == track) and (view->hover.type == Selection::Type::TRACK_BUTTON_FX))
			c->setColor(col_but_hover);
		c->drawStr(area.x1 + 5 + 17*3, area.y1 + 22-2, "⚡"); // ...*/

		/*c->setColor(col_but);
		if ((view->hover.track == track) and (view->hover.type == Selection::Type::TRACK_BUTTON_CURVE))
			c->setColor(col_but_hover);
		c->drawStr(area.x1 + 5 + 17*4, area.y1 + 22-2, "☊"); // ... */
	}
	bool on_left_button_down() override {
		view->session->set_mode("default/track");
		return true;
	}
	string get_tip() override {
		return _("edit track properties");
	}
};

TrackHeader::TrackHeader(AudioViewTrack *t) : ViewNode(t, 0, 0, AudioView::TRACK_HANDLE_WIDTH, AudioView::TRACK_HANDLE_HEIGHT) {
	z = 10;
	vtrack = t;
	float x0 = 5;
	float dx = 17;
	children.add(new TrackButtonMute(this, x0, 22));
	children.add(new TrackButtonSolo(this, x0+dx, 22));
	children.add(new TrackButtonConfig(this, x0+dx*2, 22));
}

void TrackHeader::draw(Painter *c) {
	auto *track = vtrack->track;
	bool _hover = view_hover(view->hover);
	bool extended = _hover or view->editing_track(track);
	bool playable = view->get_playable_tracks().contains(track);

	color col = view->colors.background_track_selected;
	if (view->sel.has(track))
		col = ColorInterpolate(col, view->colors.selection, 0.4f);
	if (_hover)
		col = ColorInterpolate(col, view->colors.hover, 0.2f);
	c->set_color(col);
	float h = extended ? view->TRACK_HANDLE_HEIGHT : view->TRACK_HANDLE_HEIGHT_SMALL;
	//update_area();
	c->set_roundness(view->CORNER_RADIUS);
	c->draw_rect(rect(area.x1, area.x2, area.y1, area.y1 + h));
	c->set_roundness(0);

	// track title
	c->set_font("", view->FONT_SIZE, view->sel.has(track) and playable, false);
	if (playable)
		c->set_color(view->colors.text);
	else
		c->set_color(view->colors.text_soft2);
	c->draw_str(area.x1 + 23, area.y1 + 5, track->nice_name() + (vtrack->solo ? " (solo)" : ""));

	c->set_font("", -1, false, false);

	// icons
	auto *icon = view->images.track_audio;
	if (track->type == SignalType::BEATS)
		icon = view->images.track_time; // "⏱"
	else if (track->type == SignalType::MIDI)
		icon = view->images.track_midi; // "♫"
	c->set_color(view->colors.text);
	c->draw_mask_image(area.x1 + 5, area.y1 + 5, *icon);
//	if (track->muted and !extended)
//		c->draw_image(area.x1 + 5, area.y1 + 5, *view->images.x);

	for (auto *c: children)
		c->hidden = !extended;
	children[1]->hidden |= (view->song->tracks.num == 1);
}

class MouseDelayDndTrack : public MouseDelayAction {
public:
	AudioViewTrack *track;
	MouseDelayDndTrack(AudioViewTrack *t) { track = t; }
	void on_draw_post(Painter *c) override {
		auto *view = track->view;
		//int orig = get_track_index(moving_track);
		int t = get_track_move_target(true);
		int y = view->vtrack.back()->area.y2;
		if (t < view->vtrack.num)
			y = view->vtrack[t]->area.y1;

		c->set_color(view->colors.selection_boundary);
		c->set_line_width(2.0f);
		c->draw_line(view->area.x1,  y,  view->area.x2,  y);
		c->set_line_width(1.0f);

		/*c->setColor(view->colors.selection_internal);
		rect r = view->vtrack[orig]->area;
		r.x2 = view->TRACK_HANDLE_WIDTH;
		c->drawRect(r);*/

		view->draw_cursor_hover(c, track->track->nice_name());
	}
	void on_finish() override {
		int target = get_track_move_target(false);
		track->track->move(target);
	}

	int get_track_move_target(bool visual) {
		auto *view = track->view;
		int orig = get_track_index(track->track);
		foreachi(auto vt, view->vtrack, i) {
			int y = (vt->area.y1 + vt->area.y2) / 2;
			if (y > view->my) {
				if (visual or (i <= orig))
					return i;
				else
					return i - 1;
			}
		}
		return visual ? view->song->tracks.num : (view->song->tracks.num-1);
	}
};

bool TrackHeader::on_left_button_down() {
	auto *l = vtrack->first_layer();
	if (view->select_xor) {
		view->toggle_select_layer_with_content_in_cursor(l);
	} else {
		if (view->sel.has(vtrack->track)) {
			view->set_selection(view->sel.restrict_to_track(vtrack->track));
		} else {
			view->exclusively_select_layer(l);
			view->select_under_cursor();
		}
	}
	view->mdp_prepare(new MouseDelayDndTrack(vtrack));
	return true;
}
bool TrackHeader::on_left_double_click() {
	view->session->set_mode("default/track");
	return true;
}

bool TrackHeader::on_right_button_down() {
	if (!view->sel.has(vtrack->track)) {
		view->exclusively_select_layer(vtrack->first_layer());
		view->select_under_cursor();
	}
	view->open_popup(view->menu_track);
	return true;
}

HoverData TrackHeader::get_hover_data(float mx, float my) {
	auto h = ViewNode::get_hover_data(mx, my);
	h.vtrack = vtrack;
	h.vlayer = vtrack->first_layer();
	return h;
}
