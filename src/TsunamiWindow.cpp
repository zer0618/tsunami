/*
 * Tsunami.cpp
 *
 *  Created on: 21.03.2012
 *      Author: michi
 */

#include "TsunamiWindow.h"
#include "Session.h"

#include "Module/Audio/SongRenderer.h"
#include "Tsunami.h"
#include "View/Dialog/NewDialog.h"
#include "View/Dialog/SettingsDialog.h"
#include "View/Dialog/MarkerDialog.h"
#include "View/Dialog/BarAddDialog.h"
#include "View/Dialog/BarDeleteDialog.h"
#include "View/Dialog/BarEditDialog.h"
#include "View/Dialog/PauseAddDialog.h"
#include "View/Dialog/PauseEditDialog.h"
#include "View/BottomBar/BottomBar.h"
#include "View/BottomBar/MiniBar.h"
//#include "View/BottomBar/DeviceConsole.h"
#include "View/SideBar/SideBar.h"
#include "View/SideBar/CaptureConsole.h"
#include "View/Mode/ViewModeDefault.h"
#include "View/Mode/ViewModeMidi.h"
#include "View/Mode/ViewModeCapture.h"
#include "View/Mode/ViewModeScaleBars.h"
#include "View/Helper/Slider.h"
#include "View/Helper/Progress.h"
#include "View/AudioView.h"
#include "View/AudioViewTrack.h"
#include "View/AudioViewLayer.h"
#include "Plugins/PluginManager.h"
#include "Plugins/TsunamiPlugin.h"
#include "Plugins/SongPlugin.h"
#include "Storage/Storage.h"
#include "Stuff/Log.h"
#include "Stuff/Clipboard.h"
#include "Stuff/BackupManager.h"
#include "Device/DeviceManager.h"
#include "Data/base.h"
#include "Data/Track.h"
#include "Data/TrackLayer.h"
#include "Data/Song.h"
#include "Data/SongSelection.h"
#include "Data/Rhythm/Bar.h"
#include "Action/ActionManager.h"
#include "Action/Track/Buffer/ActionTrackEditBuffer.h"
#include "Module/Audio/AudioEffect.h"
#include "Module/Audio/AudioSource.h"
#include "Module/Midi/MidiEffect.h"
#include "Module/Midi/MidiSource.h"
#include "Plugins/FastFourierTransform.h"
#include "View/Helper/PeakMeterDisplay.h"
#include "lib/hui/hui.h"

extern const string AppName;

namespace hui{
	extern string file_dialog_default;
}

hui::Timer debug_timer;

TsunamiWindow::TsunamiWindow(Session *_session) :
	hui::Window(AppName, 800, 600)
{
	session = _session;
	session->set_win(this);
	song = session->song;
	app = tsunami;

	int width = hui::Config.get_int("Window.Width", 800);
	int height = hui::Config.get_int("Window.Height", 600);
	bool maximized = hui::Config.get_bool("Window.Maximized", true);

	event("new", [&]{ on_new(); });
	set_key_code("new", hui::KEY_N + hui::KEY_CONTROL, "hui:new");
	event("open", [&]{ on_open(); });
	set_key_code("open", hui::KEY_O + hui::KEY_CONTROL, "hui:open");
	event("save", [&]{ on_save(); });
	set_key_code("save", hui::KEY_S + hui::KEY_CONTROL, "hui:save");
	event("save_as", [&]{ on_save_as(); });
	set_key_code("save_as", hui::KEY_S + hui::KEY_CONTROL + hui::KEY_SHIFT, "hui:save-as");
	event("copy", [&]{ on_copy(); });
	set_key_code("copy", hui::KEY_C + hui::KEY_CONTROL, "hui:copy");
	event("paste", [&]{ on_paste(); });
	set_key_code("paste", hui::KEY_V + hui::KEY_CONTROL, "hui:paste");
	event("paste_as_samples", [&]{ on_paste_as_samples(); });
	set_key_code("paste_as_samples", hui::KEY_V + hui::KEY_CONTROL + hui::KEY_SHIFT, "hui:paste");
	event("paste_time", [&]{ on_paste_time(); });
	event("delete", [&]{ on_delete(); });
	set_key_code("delete", hui::KEY_DELETE, "hui:delete");
	event("render_export_selection", [&]{ on_render_export_selection(); });
	set_key_code("render_export_selection", hui::KEY_X + hui::KEY_CONTROL, "");
	event("export_selection", [&]{ on_export_selection(); });
	event("quick_export", [&]{ on_quick_export(); });
	set_key_code("quick_export", hui::KEY_X + hui::KEY_CONTROL + hui::KEY_SHIFT, "");
	event("undo", [&]{ on_undo(); });
	set_key_code("undo", hui::KEY_Z + hui::KEY_CONTROL, "hui:undo");
	event("redo", [&]{ on_redo(); });
	set_key_code("redo", hui::KEY_Y + hui::KEY_CONTROL, "hui:redo");
	event("track_render", [&]{ on_track_render(); });
	event("add_audio_track_mono", [&]{ on_add_audio_track_mono(); });
	set_key_code("add_audio_track_mono", -1, "hui:add");
	event("add_audio_track_stereo", [&]{ on_add_audio_track_stereo(); });
	set_key_code("add_audio_track_stereo", -1, "hui:add");
	event("add_time_track", [&]{ on_add_time_track(); });
	set_key_code("add_time_track", -1, "hui:add");
	event("add_midi_track", [&]{ on_add_midi_track(); });
	set_key_code("add_midi_track", -1, "hui:add");
	event("delete_track", [&]{ on_delete_track(); });
	set_key_code("delete_track", -1, "hui:delete");
	event("track_edit_midi", [&]{ on_track_edit_midi(); });
	set_key_code("track_edit_midi", -1, "hui:edit");
	event("track_edit_fx", [&]{ on_track_edit_fx(); });
	set_key_code("track_edit_fx", -1, "hui:edit");
	event("track_add_marker", [&]{ on_track_add_marker(); });
	set_key_code("track_add_marker", -1, "hui:add");
	event("track_convert_mono", [&]{ on_track_convert_mono(); });
	event("track_convert_stereo", [&]{ on_track_convert_stereo(); });
	event("delete_buffer", [&]{ on_buffer_delete(); });
	event("make_buffer_movable", [&]{ on_buffer_make_movable(); });


	event("layer_midi_mode_linear", [&]{ on_layer_midi_mode_linear(); });
	event("layer_midi_mode_tab", [&]{ on_layer_midi_mode_tab(); });
	event("layer_midi_mode_classical", [&]{ on_layer_midi_mode_classical(); });

	event("layer_add", [&]{ on_add_layer(); });
	set_key_code("layer_add", -1, "hui:add");
	event("delete_layer", [&]{ on_delete_layer(); });
	set_key_code("delete_layer", -1, "hui:delete");
	event("layer_make_track", [&]{ on_layer_make_track(); });
	event("layer_merge", [&]{ on_layer_merge(); });
	event("layer_mark_dominant", [&]{ on_layer_mark_selection_dominant(); });
	event("add_bars", [&]{ on_add_bars(); });
	set_key_code("add_bars", -1, "hui:add");
	event("add_pause", [&]{ on_add_pause(); });
	set_key_code("add_pause", -1, "hui:add");
	event("delete_bars", [&]{ on_delete_bars(); });
	set_key_code("delete_bars", -1, "hui:delete");
	event("delete_time", [&]{ on_delete_time_interval(); });
	set_key_code("delete_time", -1, "hui:delete");
	event("insert_time", [&]{ on_insert_time_interval(); });
	set_key_code("insert_time", -1, "hui:add");
	event("edit_bars", [&]{ on_edit_bars(); });
	set_key_code("edit_bars", -1, "hui:edit");
	event("scale_bars", [&]{ on_scale_bars(); });
	set_key_code("scale_bars", -1, "hui:scale");
	event("sample_manager", [&]{ on_sample_manager(); });
	event("song_edit_samples", [&]{ on_sample_manager(); });
	event("show_mixing_console", [&]{ on_mixing_console(); });
	event("show_fx_console", [&]{ on_fx_console(); });
	event("sample_from_selection", [&]{ on_sample_from_selection(); });
	set_key_code("sample_from_selection", -1, "hui:cut");
	event("insert_sample", [&]{ on_insert_sample(); });
	set_key_code("insert_sample", hui::KEY_I + hui::KEY_CONTROL, "");
	event("remove_sample", [&]{ on_remove_sample(); });
	event("delete_marker", [&]{ on_delete_marker(); });
	event("edit_marker", [&]{ on_edit_marker(); });
	event("track_import", [&]{ on_track_import(); });
	event("sub_import", [&]{ on_sample_import(); });
	event("song_properties", [&]{ on_song_properties(); });
	set_key_code("song_properties", hui::KEY_F4, "");
	event("track_properties", [&]{ on_track_properties(); });
	event("sample_properties", [&]{ on_sample_properties(); });
	event("settings", [&]{ on_settings(); });
	event("play", [&]{ on_play(); });
	set_key_code("play", -1, "hui:media-play");
	event("play_loop", [&]{ on_play_loop(); });
	event("pause", [&]{ on_pause(); });
	set_key_code("pause", -1, "hui:media-pause");
	event("stop", [&]{ on_stop(); });
	set_key_code("stop", hui::KEY_CONTROL + hui::KEY_T, "hui:media-stop");
	event("record", [&]{ on_record(); });
	set_key_code("record", hui::KEY_CONTROL + hui::KEY_R, "hui:media-record");
	event("show_log", [&]{ on_show_log(); });
	event("about", [&]{ on_about(); });
	set_key_code("run_plugin", hui::KEY_RETURN + hui::KEY_SHIFT, "hui:execute");
	event("exit", [&]{ on_exit(); });
	set_key_code("exit", hui::KEY_Q + hui::KEY_CONTROL, "hui:quit");
	event("select_all", [&]{ on_select_all(); });
	set_key_code("select_all", hui::KEY_A + hui::KEY_CONTROL, "");
	event("select_nothing", [&]{ on_select_none(); });
	event("select_expand", [&]{ on_select_expand(); });
	set_key_code("select_expand", hui::KEY_TAB + hui::KEY_SHIFT, "");
	event("view_midi_default", [&]{ on_view_midi_default(); });
	event("view_midi_tab", [&]{ on_view_midi_tab(); });
	event("view_midi_score", [&]{ on_view_midi_score(); });
	event("view_optimal", [&]{ on_view_optimal(); });
	event("zoom_in", [&]{ on_zoom_in(); });
	event("zoom_out", [&]{ on_zoom_out(); });

	// table structure
	set_size(width, height);
	set_border_width(0);
	add_grid("", 0, 0, "root_table");
	set_target("root_table");
	add_grid("", 0, 0, "main_table");

	// main table
	set_target("main_table");
	add_drawing_area("!grabfocus", 0, 0, "area");


	toolbar[0]->set_by_id("toolbar");
	//ToolbarConfigure(false, true);

	set_menu(hui::CreateResourceMenu("menu"));
	//ToolBarConfigure(true, true);
	set_maximized(maximized);


	app->plugin_manager->add_plugins_to_menu(this);

	// events
	event("hui:close", [&]{ on_exit(); });

	for (int i=0; i<256; i++){
		event("import-backup-"+i2s(i), [&]{ on_import_backup(); });
		event("delete-backup-"+i2s(i), [&]{ on_delete_backup(); });
	}

	auto_delete = false;


	view = new AudioView(session, "area");
	session->view = view;

	// side bar
	side_bar = new SideBar(session);
	embed(side_bar, "root_table", 1, 0);

	// bottom bar
	bottom_bar = new BottomBar(session);
	embed(bottom_bar, "main_table", 0, 1);
	mini_bar = new MiniBar(bottom_bar, session);
	embed(mini_bar, "main_table", 0, 2);

	view->subscribe(this, [&]{ on_update(); });
	song->action_manager->subscribe(this, [&]{ on_update(); });
	app->clipboard->subscribe(this, [&]{ on_update(); });
	bottom_bar->subscribe(this, [&]{ on_bottom_bar_update(); });
	side_bar->subscribe(this, [&]{ on_side_bar_update(); });


	update_menu();
}

TsunamiWindow::~TsunamiWindow()
{
	// all done by onDestroy()
}

void TsunamiCleanUp()
{
	bool again = false;
	do{
		again = false;
		foreachi(Session *s, tsunami->sessions, i)
			if (s->win->got_destroyed() and s->win->auto_delete){
				delete s->win;
				delete s;
				tsunami->sessions.erase(i);
				again = true;
				break;
			}
	}while(again);

	if (tsunami->sessions.num == 0)
		tsunami->end();
}

void TsunamiWindow::on_destroy()
{
	int w, h;
	get_size_desired(w, h);
	hui::Config.set_int("Window.Width", w);
	hui::Config.set_int("Window.Height", h);
	hui::Config.set_bool("Window.Maximized", is_maximized());

	view->unsubscribe(this);
	song->action_manager->unsubscribe(this);
	app->clipboard->unsubscribe(this);
	bottom_bar->unsubscribe(this);
	side_bar->unsubscribe(this);

	delete side_bar;
	delete mini_bar;
	delete bottom_bar;
	delete view;

	hui::RunLater(0.010f, &TsunamiCleanUp);
}


void TsunamiWindow::on_about()
{
	hui::AboutBox(this);
}



void TsunamiWindow::on_add_audio_track_mono()
{
	song->add_track(SignalType::AUDIO_MONO);
}

void TsunamiWindow::on_add_audio_track_stereo()
{
	song->add_track(SignalType::AUDIO_STEREO);
}

void TsunamiWindow::on_add_time_track()
{
	song->begin_action_group();
	try{
		song->add_track(SignalType::BEATS, 0);

		// some default data
		auto b = BarPattern(0, 4, 1);
		b.set_bpm(90, song->sample_rate);
		for (int i=0; i<10; i++)
			song->add_bar(-1, b, false);
	}catch(Song::Exception &e){
		session->e(e.message);
	}
	song->end_action_group();
}

void TsunamiWindow::on_import_backup()
{
	string id = hui::GetEvent()->id;
	int uuid = id.explode(":").back()._int();
	string filename = BackupManager::get_filename_for_uuid(uuid);
	if (filename == "")
		return;


	if (song->is_empty()){
		session->storage_options = "f32:2:44100";
		session->storage->load(song, filename);
		//BackupManager::set_save_state(session);
		session->storage_options = "";
	}else{
		Session *s = tsunami->create_session();
		s->storage_options = "f32:2:44100";
		s->win->show();
		s->storage->load(s->song, filename);
		s->storage_options = "";
	}

	//BackupManager::del
}

void TsunamiWindow::on_delete_backup()
{
	string id = hui::GetEvent()->id;
	int uuid = id.explode(":").back()._int();
	BackupManager::delete_old(uuid);
}

void TsunamiWindow::on_add_midi_track()
{
	song->add_track(SignalType::MIDI);
}

void TsunamiWindow::on_track_render()
{
	Range range = view->sel.range;
	if (range.empty()){
		session->e(_("Selection range is empty"));
		return;
	}

	auto *p = new ProgressCancelable(_(""), this);
	song->begin_action_group();

	SongRenderer renderer(song);
	renderer.prepare(range, false);
	renderer.allow_tracks(view->sel.tracks);
	renderer.allow_layers(view->get_playable_layers());

	Track *t = song->add_track(SignalType::AUDIO);

	int chunk_size = 1<<12;
	int offset = range.offset;

	while (offset < range.end()){
		p->set((float)(offset - range.offset) / range.length);

		AudioBuffer buf;
		Range r = Range(offset, min(chunk_size, range.end() - offset));
		t->layers[0]->get_buffers(buf, r);

		ActionTrackEditBuffer *a = new ActionTrackEditBuffer(t->layers[0], r);
		renderer.read(buf);
		song->execute(a);

		offset += chunk_size;
		if (p->is_cancelled())
			break;
	}
	song->end_action_group();
	delete p;

}

void TsunamiWindow::on_delete_track()
{
	if (view->cur_track()){
		try{
			song->delete_track(view->cur_track());
		}catch(Song::Exception &e){
			session->e(e.message);
		}
	}else{
		session->e(_("No track selected"));
	}
}

void TsunamiWindow::on_track_edit_midi()
{
	session->set_mode("midi");
}

void TsunamiWindow::on_track_edit_fx()
{
	session->set_mode("default/fx");
}

void TsunamiWindow::on_track_add_marker()
{
	if (view->hover_before_leave.track){
		Range range = view->sel.range;
		if (!range.is_inside(view->hover_before_leave.pos))
			range = Range(view->hover_before_leave.pos, 0);
		MarkerDialog *dlg = new MarkerDialog(this, view->hover_before_leave.track, range, nullptr);
		dlg->run();
		delete dlg;
	}else{
		session->e(_("No track selected"));
	}
}

void TsunamiWindow::on_track_convert_mono()
{
	if (view->cur_track())
		view->cur_track()->set_channels(1);
	else
		session->e(_("No track selected"));
}

void TsunamiWindow::on_track_convert_stereo()
{
	if (view->cur_track())
		view->cur_track()->set_channels(2);
	else
		session->e(_("No track selected"));
}
void TsunamiWindow::on_buffer_delete()
{
	foreachi (AudioBuffer &buf, view->cur_layer()->buffers, i)
		if (buf.range().is_inside(view->hover_before_leave.pos)){
			SongSelection s = SongSelection::from_range(song, buf.range(), {}, view->cur_layer()).filter(0);
			song->delete_selection(s);
		}
}

void TsunamiWindow::on_buffer_make_movable()
{
	for (AudioBuffer &buf: view->cur_layer()->buffers){
		if (buf.range().is_inside(view->hover_before_leave.pos)){
			SongSelection s = SongSelection::from_range(song, buf.range(), {}, view->cur_layer()).filter(0);
			song->create_samples_from_selection(s, true);
		}
	}
}

void TsunamiWindow::on_layer_midi_mode_linear()
{
	view->cur_vlayer->set_midi_mode(MidiMode::LINEAR);
}

void TsunamiWindow::on_layer_midi_mode_tab()
{
	view->cur_vlayer->set_midi_mode(MidiMode::TAB);
}

void TsunamiWindow::on_layer_midi_mode_classical()
{
	view->cur_vlayer->set_midi_mode(MidiMode::CLASSICAL);
}

void TsunamiWindow::on_song_properties()
{
	session->set_mode("default/song");
}

void TsunamiWindow::on_track_properties()
{
	session->set_mode("default/track");
}

void TsunamiWindow::on_sample_properties()
{
	if (view->cur_sample)
		session->set_mode("default/sample-ref");
	else
		session->e(_("No sample selected"));
}

void TsunamiWindow::on_delete_marker()
{
	if (view->hover_before_leave.type == Selection::Type::MARKER)
		view->cur_track()->delete_marker(view->hover_before_leave.marker);
	else
		session->e(_("No marker selected"));
}

void TsunamiWindow::on_edit_marker()
{
	if (view->hover_before_leave.type == Selection::Type::MARKER){
		MarkerDialog *dlg = new MarkerDialog(this, view->cur_track(), Range::EMPTY, view->hover_before_leave.marker);
		dlg->run();
		delete dlg;
	}else
		session->e(_("No marker selected"));
}

void TsunamiWindow::on_show_log()
{
	bottom_bar->open(BottomBar::LOG_CONSOLE);
}

void TsunamiWindow::on_undo()
{
	song->undo();
}

void TsunamiWindow::on_redo()
{
	song->redo();
}

void TsunamiWindow::on_send_bug_report()
{
}


string title_filename(const string &filename)
{
	if (filename.num > 0)
		return filename.basename();// + " (" + filename.dirname() + ")";
	return _("No name");
}

bool TsunamiWindow::allow_termination()
{
	if (session->in_mode("capture")){
		if (side_bar->capture_console->is_capturing()){
			string answer = hui::QuestionBox(this, _("Question"), _("Cancel recording?"), true);
			if (answer != "hui:yes")
				return false;
			side_bar->capture_console->on_dump();
			session->set_mode("default");
		}
	}

	if (song->action_manager->is_save())
		return true;
	string answer = hui::QuestionBox(this, _("Question"), format(_("'%s'\nSave file?"), title_filename(song->filename).c_str()), true);
	if (answer == "hui:yes"){
		/*if (!OnSave())
			return false;*/
		on_save();
		return true;
	}else if (answer == "hui:no")
		return true;

	// cancel
	return false;
}

void TsunamiWindow::on_copy()
{
	app->clipboard->copy(view);
}

void TsunamiWindow::on_paste()
{
	app->clipboard->paste(view);
}

void TsunamiWindow::on_paste_as_samples()
{
	app->clipboard->paste_as_samples(view);
}

void TsunamiWindow::on_paste_time()
{
	app->clipboard->paste_with_time(view);
}

void TsunamiWindow::on_menu_execute_audio_effect()
{
	string name = hui::GetEvent()->id.explode("--")[1];

	AudioEffect *fx = CreateAudioEffect(session, name);

	fx->reset_config();
	if (fx->configure(this)){
		song->begin_action_group();
		for (Track *t: song->tracks)
			for (TrackLayer *l: t->layers)
				if (view->sel.has(l) and (t->type == SignalType::AUDIO)){
					fx->reset_state();
					fx->do_process_track(l, view->sel.range);
				}
		song->end_action_group();
	}
	delete fx;
}

void TsunamiWindow::on_menu_execute_audio_source()
{
	string name = hui::GetEvent()->id.explode("--")[1];

	AudioSource *s = CreateAudioSource(session, name);

	s->reset_config();
	if (s->configure(this)){
		song->begin_action_group();
		for (Track *t: song->tracks)
			for (TrackLayer *l: t->layers)
				if (view->sel.has(l) and (t->type == SignalType::AUDIO)){
					s->reset_state();
					AudioBuffer buf;
					l->get_buffers(buf, view->sel.range);
					s->read(buf);
				}
		song->end_action_group();
	}
	delete s;
}

void TsunamiWindow::on_menu_execute_midi_effect()
{
	string name = hui::GetEvent()->id.explode("--")[1];

	MidiEffect *fx = CreateMidiEffect(session, name);

	fx->reset_config();
	if (fx->configure(this)){
		song->action_manager->group_begin();
		for (Track *t : song->tracks)
			for (TrackLayer *l : t->layers)
			if (view->sel.has(l) and (t->type == SignalType::MIDI)){
				fx->reset_state();
				fx->process_layer(l, view->sel);
			}
		song->action_manager->group_end();
	}
	delete fx;
}

void TsunamiWindow::on_menu_execute_midi_source()
{
	string name = hui::GetEvent()->id.explode("--")[1];

	MidiSource *s = CreateMidiSource(session, name);

	s->reset_config();
	if (s->configure(this)){
		song->begin_action_group();
		for (Track *t : song->tracks)
			for (TrackLayer *l : t->layers)
			if (view->sel.has(l) and (t->type == SignalType::MIDI)){
				s->reset_state();
				MidiEventBuffer buf;
				buf.samples = view->sel.range.length;
				s->read(buf);
				l->insert_midi_data(view->sel.range.offset, midi_events_to_notes(buf));
			}
		song->end_action_group();
	}
	delete s;
}

void TsunamiWindow::on_menu_execute_song_plugin()
{
	string name = hui::GetEvent()->id.explode("--")[1];

	SongPlugin *p = CreateSongPlugin(session, name);

	p->apply();
	delete p;
}

void TsunamiWindow::on_menu_execute_tsunami_plugin()
{
	string name = hui::GetEvent()->id.explode("--")[1];

	session->execute_tsunami_plugin(name);
}

void TsunamiWindow::on_delete()
{
	if (!view->sel.is_empty())
		song->delete_selection(view->sel);
}

void TsunamiWindow::on_sample_manager()
{
	session->set_mode("default/samples");
}

void TsunamiWindow::on_mixing_console()
{
	bottom_bar->open(BottomBar::MIXING_CONSOLE);
}

void TsunamiWindow::on_fx_console()
{
	session->set_mode("default/fx");
}

void TsunamiWindow::on_sample_import()
{
}

void TsunamiWindow::on_command(const string & id)
{
}

void TsunamiWindow::on_settings()
{
	SettingsDialog *dlg = new SettingsDialog(view, this);
	dlg->run();
	delete dlg;
}

void TsunamiWindow::on_track_import()
{
	if (session->storage->ask_open_import(this)){
		Track *t = song->add_track(SignalType::AUDIO_STEREO);
		session->storage->load_track(t->layers[0], hui::Filename, view->sel.range.start());
	}
}

void TsunamiWindow::on_remove_sample()
{
	song->delete_selected_samples(view->sel);
}

void TsunamiWindow::on_play_loop()
{
	view->renderer->loop_if_allowed = !view->renderer->loop_if_allowed;
	update_menu();
}

void TsunamiWindow::on_play()
{
	if (session->in_mode("capture"))
		return;
	if (view->is_paused()){
		view->pause(false);
	}else{
		view->prepare_playback(view->get_playback_selection(false), true);
		view->play();
	}
}

void TsunamiWindow::on_pause()
{
	if (session->in_mode("capture"))
		return;
	view->pause(true);
}

void TsunamiWindow::on_stop()
{
	if (session->in_mode("capture")){
		session->set_mode("default");
	}else
		view->stop();
}

void TsunamiWindow::on_insert_sample()
{
	song->insert_selected_samples(view->sel);
}

void TsunamiWindow::on_record()
{
	session->set_mode("capture");
}

void TsunamiWindow::on_add_layer()
{
	view->cur_track()->add_layer();
}

void TsunamiWindow::on_delete_layer()
{
	if (view->cur_track()->layers.num > 1)
		view->cur_track()->delete_layer(view->cur_layer());
	else
		session->e(_("can not delete the only version of a track"));
}

void TsunamiWindow::on_layer_make_track()
{
	view->cur_layer()->make_own_track();
}

void TsunamiWindow::on_layer_merge()
{
	view->cur_track()->merge_layers();
}

void TsunamiWindow::on_layer_mark_selection_dominant()
{
	view->cur_layer()->mark_dominant(view->sel.range);
}

void TsunamiWindow::on_sample_from_selection()
{
	song->create_samples_from_selection(view->sel, false);
}

void TsunamiWindow::on_view_optimal()
{
	view->optimize_view();
}

void TsunamiWindow::on_select_none()
{
	view->select_none();
}

void TsunamiWindow::on_select_all()
{
	view->select_all();
}

void TsunamiWindow::on_select_expand()
{
	view->select_expand();
}

void TsunamiWindow::on_view_midi_default()
{
	view->set_midi_view_mode(MidiMode::LINEAR);
}

void TsunamiWindow::on_view_midi_tab()
{
	view->set_midi_view_mode(MidiMode::TAB);
}

void TsunamiWindow::on_view_midi_score()
{
	view->set_midi_view_mode(MidiMode::CLASSICAL);
}

void TsunamiWindow::on_zoom_in()
{
	view->zoom_in();
}

void TsunamiWindow::on_zoom_out()
{
	view->zoom_out();
}

void TsunamiWindow::update_menu()
{
// menu / toolbar
	// edit
	enable("undo", song->action_manager->undoable());
	enable("redo", song->action_manager->redoable());
	enable("copy", app->clipboard->can_copy(view));
	enable("paste", app->clipboard->has_data());
	enable("delete", !view->sel.is_empty());
	// file
	//Enable("export_selection", true);
	// bars
	enable("delete_time", !view->sel.range.empty());
	enable("delete_bars", view->sel.bars.num > 0);
	enable("edit_bars", view->sel.bars.num > 0);
	enable("scale_bars", view->sel.bars.num > 0);
	// sample
	enable("sample_from_selection", !view->sel.range.empty());
	enable("insert_sample", view->sel.num_samples() > 0);
	enable("remove_sample", view->sel.num_samples() > 0);
	enable("sample_properties", view->cur_sample);
	// sound
	enable("play", !session->in_mode("capture"));
	enable("stop", view->is_playback_active() or session->in_mode("capture"));
	enable("pause", view->is_playback_active() and !view->is_paused());
	check("play_loop", view->renderer->loop_if_allowed);
	enable("record", !session->in_mode("capture"));
	// view
	check("show_mixing_console", bottom_bar->is_active(BottomBar::MIXING_CONSOLE));
	check("show_fx_console", session->in_mode("default/fx"));
	check("sample_manager", session->in_mode("default/samples"));

	string title = title_filename(song->filename) + " - " + AppName;
	if (!song->action_manager->is_save())
		title = "*" + title;
	set_title(title);
}

void TsunamiWindow::on_side_bar_update()
{
	if (!side_bar->visible)
		activate(view->id);
	update_menu();
}

void TsunamiWindow::on_bottom_bar_update()
{
	if (!bottom_bar->visible)
		activate(view->id);
	update_menu();
}

void TsunamiWindow::on_update()
{
	// "Clipboard", "AudioFile" or "AudioView"
	update_menu();
}


void TsunamiWindow::on_exit()
{
	if (allow_termination()){
		BackupManager::set_save_state(session);
		destroy();
	}
}


void TsunamiWindow::on_new()
{
	NewDialog *dlg = new NewDialog(this);
	dlg->run();
	delete dlg;
	//BackupManager::set_save_state();
}


void TsunamiWindow::on_open()
{
	if (session->storage->ask_open(this)){
		if (song->is_empty()){
			if (session->storage->load(song, hui::Filename))
				BackupManager::set_save_state(session);
		}else{
			Session *s = tsunami->create_session();
			s->win->show();
			s->storage->load(s->song, hui::Filename);
		}
	}
}


void TsunamiWindow::on_save()
{
	if (song->filename == ""){
		on_save_as();
	}else{
		if (session->storage->save(song, song->filename)){
			view->set_message(_("file saved"));
			BackupManager::set_save_state(session);
		}
	}
}

string _suggest_filename(Song *s, const string &dir)
{
	if (s->filename != "")
		return s->filename.basename();
	string base = get_current_date().format("%Y-%m-%d");

	string ext = "nami";
	if ((s->tracks.num == 1) and (s->tracks[0]->type == SignalType::AUDIO))
		ext = "ogg";
	bool allow_midi = true;
	for (Track* t: s->tracks)
		if ((t->type != SignalType::MIDI) and (t->type != SignalType::BEATS))
			allow_midi = false;
	if (allow_midi)
		ext = "midi";

	for (int i=0; i<26; i++){
		string name = base + "a." + ext;
		name[name.num - ext.num - 2] += i;
		msg_write(dir + name);
		if (!file_test_existence(dir + name))
			return name;
	}
	return "";
}

void TsunamiWindow::on_save_as()
{
	if (song->filename == "")
		hui::file_dialog_default = _suggest_filename(song, session->storage->current_directory);

	if (session->storage->ask_save(this)){
		if (session->storage->save(song, hui::Filename))
			view->set_message(_("file saved"));
	}

	hui::file_dialog_default = "";
}

void TsunamiWindow::on_render_export_selection()
{
	if (session->storage->ask_save_render_export(this)){
		if (session->storage->render_export_selection(song, &view->sel, hui::Filename))
			view->set_message(_("file exported"));
	}
}

void TsunamiWindow::on_export_selection()
{
	// TODO
	if (session->storage->ask_save(this)){
		if (session->storage->render_export_selection(song, &view->sel, hui::Filename))
			view->set_message(_("file exported"));
	}
}

void TsunamiWindow::on_quick_export()
{
	string dir = hui::Config.get_str("QuickExportDir", hui::Application::directory);
	if (session->storage->save(song, dir + _suggest_filename(song, dir)))
		view->set_message(_("file saved"));
}

int pref_bar_index(AudioView *view)
{
	if (view->sel.bar_gap >= 0)
		return view->sel.bar_gap;
	if (!view->sel.bar_indices.empty())
		return view->sel.bar_indices.end() + 1;
	if (view->hover_before_leave.pos > 0)
		return view->song->bars.num;
	return 0;
}

void TsunamiWindow::on_add_bars()
{
	auto dlg = new BarAddDialog(win, song, pref_bar_index(view));
	dlg->run();
	delete dlg;
}

void TsunamiWindow::on_add_pause()
{
	auto *dlg = new PauseAddDialog(win, song, pref_bar_index(view));
	dlg->run();
	delete dlg;
}

void TsunamiWindow::on_delete_bars()
{
	auto *dlg = new BarDeleteDialog(win, song, view->sel.bar_indices);
	dlg->run();
	delete dlg;
}

void TsunamiWindow::on_delete_time_interval()
{
	hui::ErrorBox(this, "todo", "todo");
	/*song->action_manager->beginActionGroup();

	for (int i=view->sel.bars.end()-1; i>=view->sel.bars.start(); i--){
		song->deleteBar(i, view->bars_edit_data);
	}
	song->action_manager->endActionGroup();*/
}

void TsunamiWindow::on_insert_time_interval()
{
	hui::ErrorBox(this, "todo", "todo");
	/*song->action_manager->beginActionGroup();

	for (int i=view->sel.bars.end()-1; i>=view->sel.bars.start(); i--){
		song->deleteBar(i, view->bars_edit_data);
	}
	song->action_manager->endActionGroup();*/
}

void TsunamiWindow::on_edit_bars()
{
	if (view->sel.bars.num == 0){
		return;
	}
	int num_bars = 0;
	int num_pauses = 0;
	for (int i=view->sel.bar_indices.offset; i<view->sel.bar_indices.end(); i++)
		if (song->bars[i]->is_pause())
			num_pauses ++;
		else
			num_bars ++;
	if (num_bars > 0 and num_pauses == 0){
		hui::Dialog *dlg = new BarEditDialog(win, song, view->sel.bar_indices);
		dlg->run();
		delete dlg;
	}else if (num_bars == 0 and num_pauses == 1){
		hui::Dialog *dlg = new PauseEditDialog(win, song, view->sel.bar_indices.start());
		dlg->run();
		delete dlg;
	}else{
		hui::ErrorBox(this, _("Error"), _("Can only edit bars or a single pause at a time."));
	}
}

void TsunamiWindow::on_scale_bars()
{
	session->set_mode("scale-bars");
	Set<int> s;
	for (int i=view->sel.bar_indices.start(); i<view->sel.bar_indices.end(); i++)
		s.add(i);
	view->mode_scale_bars->start_scaling(s);
}
