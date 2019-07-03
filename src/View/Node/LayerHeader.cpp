/*
 * LayerHeader.cpp
 *
 *  Created on: 08.06.2019
 *      Author: michi
 */

#include "LayerHeader.h"
#include "ViewNode.h"
#include "AudioViewLayer.h"
#include "AudioViewTrack.h"
#include "../AudioView.h"
#include "../../Data/base.h"
#include "../../Data/Song.h"
#include "../../Data/Track.h"
#include "../../Data/TrackLayer.h"




class LayerHeaderButton : public ViewNodeRel {
public:
	AudioViewLayer *vlayer;
	LayerHeader *header;
	LayerHeaderButton(LayerHeader *lh, float dx, float dy) : ViewNodeRel(dx, dy, 16, 16) {
		vlayer = lh->vlayer;
		header = lh;
	}
	HoverData get_hover_data(float mx, float my) override {
		auto h = header->get_hover_data(mx, my);
		h.node = this;
		return h;
	}
	color get_color() {
		return header->color_text();
		auto *view = vlayer->view;
		if (is_cur_hover())
			return view->colors.text;
		return ColorInterpolate(view->colors.text, view->colors.hover, 0.3f);
	}
};

class LayerButtonMute: public LayerHeaderButton {
public:
	LayerButtonMute(LayerHeader *th, float dx, float dy) : LayerHeaderButton(th, dx, dy) {}
	void draw(Painter *c) override {
	}
	bool on_left_button_down() override {
		return true;
	}
	string get_tip() override {
		return _("toggle mute");
	}
};

class LayerButtonSolo: public LayerHeaderButton {
public:
	LayerButtonSolo(LayerHeader *th, float dx, float dy) : LayerHeaderButton(th, dx, dy) {}
	void draw(Painter *c) override {
		c->set_color(get_color());
		//c->drawStr(area.x1, area.y1, "S");
		c->draw_mask_image(area.x1, area.y1, *vlayer->view->images.solo);
	}
	bool on_left_button_down() override {
		vlayer->set_solo(!vlayer->solo);
		return true;
	}
	string get_tip() override {
		return _("toggle solo");
	}
};

//_("make main version")...

class LayerButtonExplode: public LayerHeaderButton {
public:
	LayerButtonExplode(LayerHeader *th, float dx, float dy) : LayerHeaderButton(th, dx, dy) {}
	void draw(Painter *c) override {
		c->set_color(get_color());
		if (vlayer->represents_imploded)
			c->draw_str(area.x1, area.y1, u8"\u2b73");
		else
			c->draw_str(area.x1, area.y1, u8"\u2b71");
	}
	bool on_left_button_down() override {
		if (vlayer->represents_imploded)
			vlayer->view->explode_track(vlayer->layer->track);
		else
			vlayer->view->implode_track(vlayer->layer->track);
		return true;
	}
	string get_tip() override {
		return vlayer->represents_imploded ? _("explode") : _("implode");
	}
};

LayerHeader::LayerHeader(AudioViewLayer *l) : ViewNodeRel(0, 0, AudioView::LAYER_HANDLE_WIDTH, AudioView::TRACK_HANDLE_HEIGHT) {
	z = 10;
	align.horizontal = AlignData::Mode::RIGHT;
	vlayer = l;
	float x0 = 5;
	float dx = 17;
	add_child(new LayerButtonMute(this, x0, 22));
	add_child(new LayerButtonSolo(this, x0+dx, 22));
	add_child(new LayerButtonExplode(this, x0+dx*2, 22));
}

color LayerHeader::color_bg() {
	auto *layer = vlayer->layer;
	auto *view = vlayer->view;
	color col;
	if (view->sel.has(layer))
		col = view->colors.blob_bg_selected;
	else
		col = view->colors.blob_bg;
	if (is_cur_hover())
		col = view->colors.hoverify(col);
	return col;
}

bool LayerHeader::playable() {
	return vlayer->view->get_playable_layers().contains(vlayer->layer);
}

color LayerHeader::color_text() {
	auto *layer = vlayer->layer;
	auto *view = vlayer->view;
	if (view->sel.has(layer)) {
		if (playable())
			return view->colors.text;
		return view->colors.text_soft1;
	} else {
		return view->colors.text_soft2;
	}
}

void LayerHeader::draw(Painter *c) {
	auto *view = vlayer->view;
	auto *layer = vlayer->layer;
	bool _hover = is_cur_hover();
	bool extended = _hover or view->editing_layer(vlayer);

	c->set_color(color_bg());
	float h = extended ? view->TRACK_HANDLE_HEIGHT : view->TRACK_HANDLE_HEIGHT_SMALL;
	c->set_roundness(view->CORNER_RADIUS);
	c->draw_rect(area.x1,  area.y1,  area.width(), h);
	c->set_roundness(0);

	// track title
	c->set_font("", view->FONT_SIZE, view->sel.has(layer) and playable(), false);
	c->set_color(color_text());
	string title;
	if (layer->track->has_version_selection()) {
		if (layer->is_main())
			title = _("base");
		else
			title = "v" + i2s(layer->version_number() + 1);
	} else {
		title = "l" + i2s(layer->version_number() + 1);
	}
	if (vlayer->solo)
		title += " (solo)";
	if (vlayer->represents_imploded)
		title = "imploded";
	c->draw_str(area.x1 + 5, area.y1 + 5, title);
	if (!playable()) {
		float ww = c->get_str_width(title);
		c->draw_line(area.x1 + 5, area.y1+5+5, area.x1+5+ww, area.y1+5+5);
	}

	c->set_font("", -1, false, false);

/*	// icons
	if (layer->type == SignalType::BEATS) {
		c->set_color(view->colors.text);
		c->draw_mask_image(area.x2 - view->LAYER_HANDLE_WIDTH + 5, area.y1 + 5, *view->images.track_time); // "⏱"
	} else if (layer->type == SignalType::MIDI) {
		c->set_color(view->colors.text);
		c->draw_mask_image(area.x2 - view->LAYER_HANDLE_WIDTH + 5, area.y1 + 5, *view->images.track_midi); // "♫"
	} else {
		c->set_color(view->colors.text);
		c->draw_mask_image(area.x2 - view->LAYER_HANDLE_WIDTH + 5, area.y1 + 5, *view->images.track_audio); // "∿"
	}*/



	for (auto *c: children)
		c->hidden = !extended;
	children[1]->hidden |= (view->song->tracks.num == 1) or layer->track->has_version_selection();
	children[2]->hidden |= !layer->is_main();
}

bool LayerHeader::on_left_button_down() {
	auto *view = vlayer->view;
	if (view->select_xor) {
		view->toggle_select_layer_with_content_in_cursor(vlayer);
	} else {
		if (view->exclusively_select_layer(vlayer)) {
			view->set_selection(view->sel.restrict_to_layer(vlayer->layer));
		} else {
			view->select_under_cursor();
		}
	}
	return true;
}

bool LayerHeader::on_right_button_down() {
	auto *view = vlayer->view;
	if (!view->sel.has(vlayer->layer)) {
		view->exclusively_select_layer(vlayer);
		view->select_under_cursor();
	}
	view->open_popup(view->menu_layer);
	return true;
}
HoverData LayerHeader::get_hover_data(float mx, float my) {
	auto *view = vlayer->view;
	auto h = ViewNode::get_hover_data(mx, my);
	h.vtrack = view->get_track(vlayer->layer->track);
	h.vlayer = vlayer;
	return h;
}
