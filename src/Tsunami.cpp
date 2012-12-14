/*
 * Tsunami.cpp
 *
 *  Created on: 21.03.2012
 *      Author: michi
 */

#include "lib/hui/hui.h"
#include "Tsunami.h"
#include "View/Dialog/NewDialog.h"
#include "View/Dialog/CaptureDialog.h"
#include "View/Dialog/SettingsDialog.h"

#include "Plugins/FastFourierTransform.h"

Tsunami *tsunami = NULL;
extern string AppName;
extern string AppVersion;

int debug_timer;

Tsunami::Tsunami(Array<string> arg) :
	CHuiWindow(AppName, -1, -1, 800, 600, NULL, false, HuiWinModeResizable | HuiWinModeControls, true)
{
	tsunami = this;

	progress = new Progress;
	log = new Log(this);

	output = new AudioOutput;
	input = new AudioInput;
	renderer = new AudioRenderer;


	int width = HuiConfigReadInt("Window.Width", 800);
	int height = HuiConfigReadInt("Window.Height", 600);
	bool maximized = HuiConfigReadBool("Window.Maximized", true);

	//HuiAddKeyCode("insert_added", KEY_RETURN);
	//HuiAddKeyCode("remove_added", KEY_BACKSPACE);
	HuiAddKeyCode("jump_other_file", KEY_TAB);

	HuiAddCommandM("new", "hui:new", KEY_N + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnNew);
	HuiAddCommandM("open", "hui:open", KEY_O + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnOpen);
	HuiAddCommandM("save", "hui:save", KEY_S + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnSave);
	HuiAddCommandM("save_as", "hui:save-as", KEY_S + KEY_CONTROL + KEY_SHIFT, this, (void(HuiEventHandler::*)())&Tsunami::OnSaveAs);
	HuiAddCommandM("copy", "hui:copy", KEY_C + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnCopy);
	HuiAddCommandM("paste", "hui:paste", KEY_V + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnPaste);
	HuiAddCommandM("delete", "hui:delete", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnDelete);
	HuiAddCommandM("export_selection", "", KEY_X + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnExport);
	HuiAddCommandM("undo", "hui:undo", KEY_Z + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnUndo);
	HuiAddCommandM("redo", "hui:redo", KEY_Y + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnRedo);
	HuiAddCommandM("add_track", "hui:add", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnAddTrack);
	HuiAddCommandM("add_time_track", "hui:add", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnAddTimeTrack);
	HuiAddCommandM("delete_track", "hui:delete", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnDeleteTrack);
	HuiAddCommandM("level_add", "hui:add", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnAddLevel);
	HuiAddCommandM("sub_from_selection", "hui:cut", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnSubFromSelection);
	HuiAddCommandM("insert_added", "", KEY_I + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnInsertAdded);
	HuiAddCommandM("remove_added", "", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnRemoveAdded);
	HuiAddCommandM("track_import", "", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnTrackImport);
	HuiAddCommandM("sub_import", "", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnSubImport);
	HuiAddCommandM("wave_properties", "", KEY_F4, this, (void(HuiEventHandler::*)())&Tsunami::OnAudioProperties);
	HuiAddCommandM("track_properties", "", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnTrackProperties);
	HuiAddCommandM("sub_properties", "", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnSubProperties);
	HuiAddCommandM("settings", "", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnSettings);
	HuiAddCommandM("close_file", "hui:close", KEY_W + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnCloseFile);
	HuiAddCommandM("play", "hui:media-play", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnPlay);
	HuiAddCommandM("play_loop", "", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnPlayLoop);
	HuiAddCommandM("pause", "hui:media-pause", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnPause);
	HuiAddCommandM("stop", "hui:media-stop", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnStop);
	HuiAddCommandM("record", "hui:media-record", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnRecord);
	HuiAddCommandM("show_log", "", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnShowLog);
	HuiAddCommandM("about", "", -1, this, (void(HuiEventHandler::*)())&Tsunami::OnAbout);
	HuiAddCommandM("run_plugin", "hui:execute", KEY_RETURN + KEY_SHIFT, this, (void(HuiEventHandler::*)())&Tsunami::OnFindAndExecutePlugin);
	HuiAddCommandM("exit", "hui:quit", KEY_Q + KEY_CONTROL, this, (void(HuiEventHandler::*)())&Tsunami::OnExit);


	// create the window
	SetSize(width, height);
	SetBorderWidth(0);
	/*AddControlTable("", 0, 0, 1, 2, "main_table");
	SetTarget("main_table", 0);
	SetBorderWidth(8);
	AddControlTable("!noexpandy", 0, 1, 9, 1, "audio_table");
	SetTarget("audio_table", 0);
	AddButton("", 0, 0, 0, 0, "play");
	AddButton("", 1, 0, 0, 0, "pause");
	AddButton("", 2, 0, 0, 0, "stop");
	AddDrawingArea("!width=100", 3, 0, 0, 0, "peaks");
	peak_meter = new PeakMeter(this, "peaks", output);
	AddSlider("!width=100", 4, 0, 0, 0, "volume_slider");
	AddSpinButton("!width=50\\0\\0\\100", 5, 0, 0, 0, "volume");
	AddText("!width=20\\%", 6, 0, 0, 0, "label_percent");
	volume_slider = new Slider(this, "volume_slider", "volume", 0, 1, 100, (void(HuiEventHandler::*)())&Tsunami::OnVolume, output->GetVolume());
	AddButton("", 7, 0, 0, 0, "record");
	AddComboBox("!width=100", 8, 0, 0, 0, "cur_level");*/
	AddControlTable("", 0, 0, 2, 1, "main_table");
	SetTarget("main_table", 0);
	AddControlTable("!noexpandx", 1, 0, 1, 2, "tool_table");
	HideControl("tool_table", true);
	AllowEvents("key");
	ToolbarSetByID("toolbar");
	//ToolbarConfigure(false, true);

	SetMenu(HuiCreateResourceMenu("menu"));
	//ToolBarConfigure(true, true);
	SetMaximized(maximized);

	// events
	EventM("hui:close", this, (void(HuiEventHandler::*)())&Tsunami::OnExit);
	EventM("cur_level", this, (void(HuiEventHandler::*)())&Tsunami::OnCurLevel);

	plugins = new PluginManager;
	plugins->AddPluginsToMenu();

	audio= new AudioFile;

	storage = new Storage;

	view = new AudioView(this, audio);

	Subscribe(view);
	Subscribe(audio);
	Subscribe(output);

	UpdateMenu();

	debug_timer = HuiCreateTimer();

	log->Info("Tsunami " + AppVersion + _(" - viel Erfolg!"));

	audio->NewWithOneTrack(DEFAULT_SAMPLE_RATE);

	HandleArguments(arg);

	Update();
}

Tsunami::~Tsunami()
{
	Unsubscribe(view);
	Unsubscribe(audio);
	Unsubscribe(output);

	irect r = GetOuteriorDesired();
	HuiConfigWriteInt("Window.Width", r.x2 - r.x1);
	HuiConfigWriteInt("Window.Height", r.y2 - r.y1);
	HuiConfigWriteInt("Window.X", r.x1);
	HuiConfigWriteInt("Window.Y", r.y1);
	HuiConfigWriteBool("Window.Maximized", IsMaximized());

	delete(plugins);
	delete(storage);
	delete(view);
	delete(output);
	delete(input);
	delete(audio);
	HuiEnd();
}


int Tsunami::Run()
{
	return HuiRun();
}

void Tsunami::ForceRedraw()
{
	view->ForceRedraw();
}



void Tsunami::OnAbout()
{
	HuiAboutBox(this);
}



void Tsunami::OnAddTrack()
{
	audio->AddEmptyTrack();
}

void Tsunami::OnAddTimeTrack()
{
	Track *t = audio->AddTimeTrack();
}

void Tsunami::OnDeleteTrack()
{
	if (audio->used){
		if (audio->track.num < 2){
			log->Error(_("Es muss mindestens eine Spur existieren"));
			return;
		}
		audio->DeleteTrack(get_track_index(view->cur_track));
	}
}

void Tsunami::OnCloseFile()
{
	audio->Reset();
}

void Tsunami::LoadKeyCodes()
{
}

void Tsunami::OnAudioProperties()
{
	view->ExecuteAudioDialog(this);
}

void Tsunami::OnTrackProperties()
{
	view->ExecuteTrackDialog(this);
}

void Tsunami::OnSubProperties()
{
	view->ExecuteSubDialog(this);
}

void Tsunami::OnShowLog()
{
	log->Show();
}

void Tsunami::OnUndo()
{
	audio->action_manager->Undo();
}

void Tsunami::OnRedo()
{
	audio->action_manager->Redo();
}

void Tsunami::OnSendBugReport()
{
}


string title_filename(const string &filename)
{
	if (filename.num > 0)
		return filename.basename();// + " (" + filename.dirname() + ")";
	return _("Unbenannt");
}

bool Tsunami::AllowTermination()
{
	if (!audio->action_manager->IsSave()){
		string answer = HuiQuestionBox(this, _("Frage"), format(_("'%s'\nDatei speichern?"), title_filename(audio->filename).c_str()), true);
		if (answer == "yes"){
			/*if (!OnSave())
				return false;*/
			OnSave();
		}else if (answer == "cancel")
			return false;
	}
	return true;
}

void Tsunami::OnCopy()
{
}

void Tsunami::OnFindAndExecutePlugin()
{
}

void Tsunami::OnDelete()
{
	if (audio->used)
		audio->DeleteSelection(view->cur_level, false);
}

void Tsunami::OnSubImport()
{
}

void Tsunami::OnCommand(const string & id)
{
}

void Tsunami::OnSettings()
{
	SettingsDialog *dlg = new SettingsDialog(this, false);
	dlg->Update();

	HuiWaitTillWindowClosed(dlg);
}

void Tsunami::OnTrackImport()
{
	if (!audio->used)
		return;
	if (storage->AskOpenImport(this)){
		Track *t = audio->AddEmptyTrack();
		storage->LoadTrack(t, HuiFilename);
	}
}

bool Tsunami::HandleArguments(Array<string> arg)
{
	if (arg.num > 1)
		return storage->Load(audio, arg[1]);
	return false;
}

void Tsunami::OnRemoveAdded()
{
}

void Tsunami::OnPlayLoop()
{
	output->SetLoop(!output->GetLoop());
	UpdateMenu();
}

void Tsunami::OnPlay()
{
	output->Play(audio, true);
}

void Tsunami::OnPause()
{
	output->Pause();
}

void Tsunami::OnStop()
{
	output->Stop();
}

void Tsunami::OnVolume()
{
	output->SetVolume(volume_slider->Get());
}

void Tsunami::OnPaste()
{
}

void Tsunami::OnInsertAdded()
{
	if (audio->used)
		audio->InsertSelectedSubs(view->cur_level);
}

void Tsunami::OnRecord()
{
	CaptureDialog *dlg = new CaptureDialog(this, false, audio);
	dlg->Update();
	HuiWaitTillWindowClosed(dlg);
}

void Tsunami::OnAddLevel()
{
	if (audio->used)
		audio->AddLevel();
}

void Tsunami::OnCurLevel()
{
	view->cur_level = GetInt("");
	ForceRedraw();
}

void Tsunami::OnSubFromSelection()
{
	if (audio->used)
		audio->CreateSubsFromSelection(view->cur_level);
}

void Tsunami::UpdateMenu()
{
	msg_db_r("UpdateMenu", 1);
	bool selected = audio && !audio->selection.empty();
// menu / toolbar
	// edit
	Enable("undo", audio->action_manager->Undoable());
	Enable("redo", audio->action_manager->Redoable());
	Enable("copy", selected || (audio->GetNumSelectedSubs() > 0));
	Enable("paste", false);
	Enable("delete", selected || (audio->GetNumSelectedSubs() > 0));
	Enable("resize", false); // deprecated
	// file
	Enable("save", audio->used);
	Enable("save", audio->used);
	Enable("save_as", audio->used);
	Enable("cut_out", audio->used);
	Enable("close_file", audio->used);
	Enable("export_selection", audio->used);
	Enable("wave_properties", audio->used);
	// track
	Enable("track_import", audio->used);
	Enable("add_track", audio->used);
	Enable("add_time_track", audio->used);
	Enable("delete_track", view->cur_track);
	Enable("track_properties", view->cur_track);
	// level
	Enable("level_add", audio->used);
	// sub
	Enable("sub_import", view->cur_track);
	Enable("sub_from_selection", selected);
	Enable("insert_added", audio->GetNumSelectedSubs() > 0);
	Enable("remove_added", audio->GetNumSelectedSubs() > 0);
	Enable("sub_properties", view->cur_sub);
	// sound
	Enable("play", audio->used);
	Enable("stop", output->IsPlaying());
	Enable("pause", output->IsPlaying());
	Check("play_loop", output->GetLoop());


	Reset("cur_level");
	Enable("cur_level", audio->used);
	foreach(string &l, audio->level_name)
		SetString("cur_level", l);
	SetInt("cur_level", view->cur_level);

	if (audio->used){
		string title = title_filename(audio->filename) + " - " + AppName;
		if (!audio->action_manager->IsSave())
			title = "*" + title;
		SetTitle(title);
	}else
		SetTitle(AppName);
	msg_db_l(1);
}


void Tsunami::OnUpdate(Observable *o)
{
	if (o->GetName() == "AudioOutput"){
		ForceRedraw();
		UpdateMenu();
	}else // "Data" or "AudioView"
		UpdateMenu();
}


void Tsunami::OnExit()
{
	if (AllowTermination())
		delete(this);
}


void Tsunami::OnNew()
{
	NewDialog *d = new NewDialog(tsunami, false, audio);
	d->Update();
	HuiWaitTillWindowClosed(d);
}


void Tsunami::OnOpen()
{
	if (storage->AskOpen(this))
		storage->Load(audio, HuiFilename);
}


void Tsunami::OnSave()
{
	if (!audio->used)
		return;
	if (audio->filename == "")
		OnSaveAs();
	else
		storage->Save(audio, audio->filename);
}


void Tsunami::OnSaveAs()
{
	if (!audio->used)
		return;
	if (storage->AskSave(this))
		storage->Save(audio, HuiFilename);
}

void Tsunami::OnExport()
{
	if (!audio->used)
		return;
	if (storage->AskSaveExport(this))
		storage->Export(audio, HuiFilename);
}
