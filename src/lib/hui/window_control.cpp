#include "Controls/Control.h"
#include "hui.h"

namespace hui
{


//    for all
bool Panel::isEnabled(const string &id)
{
	for (int i=0;i<controls.num;i++)
		if (id == controls[i]->id)
			return controls[i]->enabled;
	return false;
}



//----------------------------------------------------------------------------------
// creating control items



Array<string> PartString;
string OptionString, HuiFormatString;

/*string ScanOptions(int id, const string &title)
{
	string title2 = get_lang(id, title);
	HuiFormatString = "";
	hui_option_multiline = false;
	hui_option_alpha = false;
	hui_option_nobar = false;
	if ((title2.num > 0) && (title2[0] == '!')){
		for (int i=0;i<title2.num;i++){
			if (title2[i] == HuiComboBoxSeparator){
				OptionString = title2.substr(0, i);
				break;
			}
		}
		Array<string> opt = OptionString.substr(1, -1).explode(",");
		for (int i=0;i<opt.num;i++){
			hui_option_multiline |= (opt[i] == "multiline");
			hui_option_alpha |= (opt[i] == "alpha");
			hui_option_nobar |= (opt[i] == "nobar");
			if (opt[i].find("format=") >= 0){
				HuiFormatString = opt[i].substr(7, -1);
				//msg_write(HuiFormatString);
				// t-T-c-C-B-i
			}
		}
		return title2.substr(OptionString.num + 1, title2.num);
	}
	return title2;
}*/

void GetPartStrings(const string &title)
{
	PartString.clear();
	OptionString = "";
	HuiFormatString = "";

	PartString = title.explode(ComboBoxSeparator);
//	msg_write(sa2s(PartString));
	if (PartString.num > 0)
		if (PartString[0].num > 0)
			if (PartString[0][0] == '!'){
				OptionString = PartString[0].substr(1, -1);
				PartString.erase(0);

				int a = OptionString.find("format=");
				if (a >= 0){
					HuiFormatString = OptionString.substr(a + 7, -1);
					int b = HuiFormatString.find(",");
					if (b >= 0)
						HuiFormatString = HuiFormatString.substr(0, b);
				}
			}
	if (PartString.num == 0)
		PartString.add("");
//	msg_write(OptionString);
//	msg_write(HuiFormatString);
}

};