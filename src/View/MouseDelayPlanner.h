/*
 * MouseDelayPlanner.h
 *
 *  Created on: 09.06.2019
 *      Author: michi
 */

#ifndef SRC_VIEW_MOUSEDELAYPLANNER_H_
#define SRC_VIEW_MOUSEDELAYPLANNER_H_

#include <functional>

class AudioView;
class Painter;

class MouseDelayAction {
public:
	MouseDelayAction() {}
	virtual ~MouseDelayAction() {}
	virtual void on_start() {}
	virtual void on_update() {}
	virtual void on_finish() {}
	virtual void on_cancel() {}
	virtual void on_clean_up() {}
	virtual void on_draw_post(Painter *p) {}
};

class MouseDelayPlanner {
public:
	MouseDelayPlanner(AudioView *view);

	float dist = -1;
	int pos0 = 0;
	float x0 = 0, y0 = 0;
	int min_move_to_start;
	bool _started_acting = false;
	AudioView *view;
	typedef std::function<void()> Callback;
	MouseDelayAction *action = nullptr;
	void prepare(MouseDelayAction *action);
	void start_acting();
	bool update();
	bool has_focus();
	bool acting();
	void finish();
	void cancel();
	void draw_post(Painter *p);
};

class SongSelection;
class AudioViewLayer;
enum class SelectionMode;

MouseDelayAction* CreateMouseDelayObjectsDnD(AudioViewLayer *l, const SongSelection &s);
MouseDelayAction* CreateMouseDelaySelect(AudioView *v, SelectionMode mode);

#endif /* SRC_VIEW_MOUSEDELAYPLANNER_H_ */
