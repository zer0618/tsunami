/*----------------------------------------------------------------------------*\
| Hui window                                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2009.12.05 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _HUI_WINDOW_EXISTS_
#define _HUI_WINDOW_EXISTS_

//#include "hui_common.h"

class HuiMenu;
class HuiEvent;
class HuiControl;
class HuiWindow;
class HuiPainter;
class HuiToolbar;
class rect;


struct HuiCompleteWindowMessage
{
	#ifdef HUI_API_WIN
		unsigned int msg,wparam,lparam;
	#endif
};


// user input
struct HuiInputData
{
	// mouse
	float x, y, dx, dy, dz;	// position, change
	float area_x, area_y;
	float mw;					// drection
	bool lb,mb,rb;				// buttons
	int row, column;
	// keyboard
	bool key[256];
	int key_code;
	int KeyBufferDepth;
	int KeyBuffer[HUI_MAX_KEYBUFFER_DEPTH];
	void reset()
	{	memset(this, 0, sizeof(HuiInputData));	}
};

struct irect
{
public:
	int x1,y1,x2,y2;
	irect(){};
	irect(int x1,int x2,int y1,int y2)
	{	this->x1=x1;	this->x2=x2;	this->y1=y1;	this->y2=y2;	}
};

class HuiEventHandler
{
public:
};

struct HuiWinEvent
{
	string id, message;
	hui_callback *function;
	void (HuiEventHandler::*member_function)();
	HuiEventHandler *object;
};

class HuiToolbar;
class HuiControl;
class HuiControlTabControl;
class HuiControlListView;
class HuiControlTreeView;
class HuiControlGrid;
class HuiControlRadioButton;

class HuiWindow : public HuiEventHandler
{
	friend class HuiToolbar;
	friend class HuiControl;
	friend class HuiControlTabControl;
	friend class HuiControlListView;
	friend class HuiControlTreeView;
	friend class HuiControlGrid;
	friend class HuiControlRadioButton;
	friend class HuiMenu;
public:
	HuiWindow();
	HuiWindow(const string &title, int x, int y, int width, int height, HuiWindow *parent, bool allow_parent, int mode);
	HuiWindow(const string &title, int x, int y, int width, int height);
	HuiWindow(const string &id, HuiWindow *parent, bool allow_parent);
	virtual ~HuiWindow();
	void __init_ext__(const string &title, int x, int y, int width, int height);
	virtual void __delete__();

	void _Init_(const string &title, int x, int y, int width, int height, HuiWindow *parent, bool allow_parent, int mode);
	void _InitGeneric_(HuiWindow *parent, bool allow_parent, int mode);
	void _CleanUp_();

	// the window
	string _cdecl Run();
	void _cdecl Show();
	void _cdecl Hide();
	void _cdecl SetMaximized(bool maximized);
	bool _cdecl IsMaximized();
	bool _cdecl IsMinimized();
	void _cdecl SetID(const string &id);
	void _cdecl SetFullscreen(bool fullscreen);
	void _cdecl SetTitle(const string &title);
	void _cdecl SetPosition(int x, int y);
	void _cdecl SetPositionSpecial(HuiWindow *win, int mode);
	void _cdecl SetSize(int width, int height);
	void _cdecl SetOuterior(irect rect);
	irect _cdecl GetOuterior();
	void _cdecl SetOuteriorDesired(irect rect);
	irect _cdecl GetOuteriorDesired();
	irect GetInterior();
	void _cdecl Activate(const string &control_id = "");
	bool _cdecl IsActive(bool include_sub_windows=false);
	void _cdecl SetMenu(HuiMenu *menu);
	HuiMenu *GetMenu();
	void _cdecl SetBorderWidth(int width);
	HuiWindow *GetParent();
	void FromResource(const string &id);


	void _cdecl SetCursorPos(int x,int y);
	void _cdecl ShowCursor(bool show);

	// status bar
	void _cdecl EnableStatusbar(bool enabled);
	//bool _cdecl IsStatusbarEnabled();
	void _cdecl SetStatusText(const string &str);

	// events
	void _cdecl AllowEvents(const string &msg);
	void _cdecl Event(const string &id, hui_callback *function);
	void _cdecl EventX(const string &id, const string &msg, hui_callback *function);
	void _EventM(const string &id, HuiEventHandler *handler, void (HuiEventHandler::*function)());
	void _EventMX(const string &id, const string &msg, HuiEventHandler *handler, void (HuiEventHandler::*function)());
	template<typename T>
	void EventM(const string &id, HuiEventHandler* handler, T fun)
	{	_EventM(id, handler, (void(HuiEventHandler::*)())fun);	}
	template<typename T>
	void EventMX(const string &id, const string &msg, HuiEventHandler* handler, T fun)
	{	_EventMX(id, msg, handler, (void(HuiEventHandler::*)())fun);	}
	bool _SendEvent_(HuiEvent *e);

	// events by overwriting
	virtual void OnMouseMove(){}
	virtual void OnLeftButtonDown(){}
	virtual void OnMiddleButtonDown(){}
	virtual void OnRightButtonDown(){}
	virtual void OnLeftButtonUp(){}
	virtual void OnMiddleButtonUp(){}
	virtual void OnRightButtonUp(){}
	virtual void OnDoubleClick(){}
	virtual void OnMouseWheel(){}
	virtual bool CanClose(){ return true; }
	virtual void OnKeyDown(){}
	virtual void OnKeyUp(){}
	virtual void OnResize(){}
	virtual void OnMove(){}
	virtual void OnRedraw(){}

	// creating controls
	void _cdecl AddButton(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddDefButton(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddColorButton(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddToggleButton(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddCheckBox(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddRadioButton(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddText(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddEdit(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddMultilineEdit(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddGroup(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddComboBox(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddTabControl(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl SetTarget(const string &id, int tab_page);
	void _cdecl AddListView(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddTreeView(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddIconView(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddListView_Test(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddProgressBar(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddSlider(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddImage(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddDrawingArea(const string &title,int x,int y,int width,int height,const string &id);
	void _cdecl AddControlTable(const string &title, int x, int y, int width, int height, const string &id);
	void _cdecl AddSpinButton(const string &title, int x, int y, int width, int height, const string &id);

	void _cdecl EmbedDialog(const string &id, int x, int y);

// using controls
	// string
	void _cdecl SetString(const string &id, const string &str);
	void _cdecl AddString(const string &id, const string &str);
	void _cdecl AddChildString(const string &id, int parent_row, const string &str);
	void _cdecl ChangeString(const string &id, int row, const string &str);
	string _cdecl GetString(const string &id);
	string _cdecl GetCell(const string &id, int row, int column);
	void _cdecl SetCell(const string &id, int row, int column, const string &str);
	// int
	void _cdecl SetInt(const string &id, int i);
	int _cdecl GetInt(const string &id);
	// float
	void _cdecl SetDecimals(int decimals);
	void _cdecl SetFloat(const string &id, float f);
	float _cdecl GetFloat(const string &id);
	// color
	void _cdecl SetColor(const string &id, const color &col);
	color _cdecl GetColor(const string &id);
	// tree
	void _cdecl ExpandAll(const string &id, bool expand);
	void _cdecl Expand(const string &id, int row, bool expand);
	bool _cdecl IsExpanded(const string &id, int row);
	// stuff
	void _cdecl Enable(const string &id, bool enabled);
	bool _cdecl IsEnabled(const string &id);
	void _cdecl HideControl(const string &id, bool hide);
	void _cdecl Check(const string &id, bool checked);
	bool _cdecl IsChecked(const string &id);
	void _cdecl SetImage(const string &id, const string &image);
	void _cdecl SetTooltip(const string &id, const string &tip);
	Array<int> _cdecl GetMultiSelection(const string &id);
	void _cdecl SetMultiSelection(const string &id, Array<int> &sel);
	void _cdecl Reset(const string &id);
	void _cdecl RemoveControl(const string &id);

	// edit completion
	void CompletionAdd(const string &id, const string &text);
	void CompletionClear(const string &id);

	// drawing
	void _cdecl Redraw(const string &id);
	void _cdecl RedrawRect(const string &_id, int x, int y, int w, int h);
	HuiPainter* _cdecl BeginDraw(const string &id);

	// input
	bool GetKey(int key);
	bool GetMouse(int &x, int &y, int button);


	// hui internal
	int _GetMainLevel_();
	int _GetUniqueID_();
	HuiControl *_GetControl_(const string &id);
#ifdef HUI_API_GTK
	HuiControl *_GetControlByWidget_(GtkWidget *widget);
	string _GetIDByWidget_(GtkWidget *widget);
#endif
	string _GetCurID_();
	void _SetCurID_(const string &id);
	bool allow_input;
	HuiInputData input;
	int mouse_offset_x, mouse_offset_y;

	HuiToolbar *toolbar[4];

private:
	int tab_creation_page;


#ifdef OS_WINDOWS
public:
	HWND hWnd;
private:
#endif
#ifdef HUI_API_WIN
	bool ready;
	//hui_callback *NixGetInputFromWindow;
	HWND statusbar, gl_hwnd;
	RECT WindowBounds,WindowClient;
	DWORD WindowStyle;
	int cdx,cdy;
#endif
#ifdef HUI_API_GTK
public:
	GtkWidget *window;
	GtkWidget *gl_widget;
	GtkWidget *plugable;
private:
	GtkWidget *vbox, *hbox, *menubar, *statusbar, *__ttt__, *input_widget;
	Array<GtkWidget*> gtk_menu;
	int gtk_num_menus;
	void _InsertControl_(HuiControl *c, int x, int y, int width, int height);
#endif
	
	int num_float_decimals;
	bool used_by_nix;
	bool is_resizable;
	int border_width;
	Array<HuiControl*> control;
	HuiControl *cur_control;
	Array<HuiWinEvent> event;
	HuiMenu *menu, *popup;
	bool statusbar_enabled;
	bool allowed, allow_keys;
	HuiWindow *parent, *terror_child;
	Array<HuiWindow*> sub_window;

	int unique_id;
	string id;
	int main_level;
	string cur_id;

	//HuiCompleteWindowMessage CompleteWindowMessage;
};


class HuiNixWindow : public HuiWindow
{
public:
	HuiNixWindow(const string &title, int x, int y, int width, int height);
	void __init_ext__(const string &title, int x, int y, int width, int height);
};

class HuiDialog : public HuiWindow
{
public:
	HuiDialog(const string &title, int width, int height, HuiWindow *root, bool allow_root);
	void __init_ext__(const string &title, int width, int height, HuiWindow *root, bool allow_root);
};

class HuiFixedDialog : public HuiWindow
{
public:
	HuiFixedDialog(const string &title, int width, int height, HuiWindow *root, bool allow_root);
	void __init_ext__(const string &title, int width, int height, HuiWindow *root, bool allow_root);
};

extern HuiWindow *HuiCurWindow;

void _cdecl HuiWindowAddControl(HuiWindow *win, const string &type, const string &title, int x, int y, int width, int height, const string &id);


void HuiFuncIgnore();
void HuiFuncClose();

enum{
	HuiWinModeResizable = 1,
	HuiWinModeNoFrame = 2,
	HuiWinModeNoTitle = 4,
	HuiWinModeControls = 8,
	HuiWinModeDummy = 16,
	HuiWinModeNix = 32,
};

#define HuiLeft		1
#define HuiRight	2
#define HuiTop		4
#define HuiBottom	8



// which one of the toolbars?
enum{
	HuiToolbarTop,
	HuiToolbarBottom,
	HuiToolbarLeft,
	HuiToolbarRight
};


#endif