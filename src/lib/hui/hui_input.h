/*----------------------------------------------------------------------------*\
| Hui input                                                                    |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2009.12.05 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _HUI_INPUT_EXISTS_
#define _HUI_INPUT_EXISTS_

class HuiEventHandler;

struct HuiCommand
{
	string id, image;
	int type, key_code;
	bool enabled;
	hui_callback *func;
	HuiEventHandler *object;
	void (HuiEventHandler::*member_function)();
};

extern Array<HuiCommand> _HuiCommand_;


class HuiEvent
{
	public:
	CHuiWindow *win;
	string message, id;
	bool is_default;
	int mx, my;
	int dx, dy, dz;
	int key, key_code;
	string text;
	int width, height;
	bool lbut, mbut, rbut;
	int row, column;
};
HuiEvent HuiCreateEvent(const string &id, const string &message);

extern HuiEvent _HuiEvent_;
HuiEvent *HuiGetEvent();


void _HuiInitInput_();

// key codes and id table ("shortcuts")
void HuiAddKeyCode(const string &id, int key_code);
void HuiAddCommand(const string &id, const string &image, int default_key_code, hui_callback *func);
void HuiAddCommandToggle(const string &id, const string &image, int default_key_code, hui_callback *func);
void HuiAddCommandM(const string &id, const string &image, int default_key_code, HuiEventHandler *object, void (HuiEventHandler::*function)());
void HuiAddCommandMToggle(const string &id, const string &image, int default_key_code, HuiEventHandler *object, void (HuiEventHandler::*function)());
void HuiLoadKeyCodes(const string &filename);
void HuiSaveKeyCodes(const string &filename);

// input
string _cdecl HuiGetKeyName(int key_code);
string _cdecl HuiGetKeyCodeName(int key_code);
string _cdecl HuiGetKeyChar(int key_code);


#ifdef HUI_API_GTK
extern GdkEvent *HuiGdkEvent;
#endif

#endif
