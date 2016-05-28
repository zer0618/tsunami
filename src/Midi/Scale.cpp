/*
 * Scale.cpp
 *
 *  Created on: 09.03.2016
 *      Author: michi
 */

#include "../lib/hui/hui.h"
#include "Scale.h"
#include "MidiData.h"



static const int scale_major_modifiers[12][7] = {
	{MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE}, // C
	{MODIFIER_NONE, MODIFIER_FLAT, MODIFIER_FLAT, MODIFIER_NONE, MODIFIER_FLAT, MODIFIER_FLAT, MODIFIER_FLAT}, // Db
	{MODIFIER_SHARP,MODIFIER_NONE, MODIFIER_NONE, MODIFIER_SHARP,MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE}, // D
	{MODIFIER_NONE, MODIFIER_NONE, MODIFIER_FLAT, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_FLAT, MODIFIER_FLAT}, // Eb
	{MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_NONE, MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_NONE, MODIFIER_NONE}, // E
	{MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_FLAT}, // F
	{MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_NONE}, // F#
	{MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_SHARP,MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE}, // G
	{MODIFIER_NONE, MODIFIER_FLAT, MODIFIER_FLAT, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_FLAT, MODIFIER_FLAT}, // Ab
	{MODIFIER_SHARP,MODIFIER_NONE, MODIFIER_NONE, MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_NONE, MODIFIER_NONE}, // A
	{MODIFIER_NONE, MODIFIER_NONE, MODIFIER_FLAT, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_NONE, MODIFIER_FLAT}, // Bb
	{MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_NONE, MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_SHARP,MODIFIER_NONE}, // B
};

inline int scale_to_major(const Scale &s)
{
	if (s.type == Scale::TYPE_MAJOR)
		return s.root;
	if (s.type == Scale::TYPE_LOCRIAN)
		return (s.root + 1) % 12;
	if (s.type == Scale::TYPE_MINOR)
		return (s.root + 3) % 12;
	if (s.type == Scale::TYPE_MIXOLYDIAN)
		return (s.root + 5) % 12;
	if (s.type == Scale::TYPE_LYDIAN)
		return (s.root + 7) % 12;
	if (s.type == Scale::TYPE_PHRYGIAN)
		return (s.root + 8) % 12;
	if (s.type == Scale::TYPE_DORIAN)
		return (s.root + 10) % 12;
	return s.root;
}

Scale::Scale(int _type, int _root)
{
	type = _type;
	root = _root;
	int ms = scale_to_major(*this);
	for (int i=0; i<7; i++)
		modifiers[i] = scale_major_modifiers[ms][i];

	int offset[7] = {0, 2, 4, 5, 7, 9, 11};
	for (int i=0; i<12; i++){
		int r = (i - root + offset[type] + 24) % 12;
		// 69 = 9 = a
		_contains[i] = !((r == 10) or (r == 1) or (r == 3) or (r == 6) or (r == 8));
	}
}

string Scale::get_type_name(int type)
{
	if (type == TYPE_MAJOR)
		return _("Major");
	if (type == TYPE_DORIAN)
		return _("Dorian");
	if (type == TYPE_PHRYGIAN)
		return _("Phrygian");
	if (type == TYPE_LYDIAN)
		return _("Lydian");
	if (type == TYPE_MIXOLYDIAN)
		return _("Mixolydian");
	if (type == TYPE_MINOR)
		return _("Minor");
	if (type == TYPE_LOCRIAN)
		return _("Locrian");
	return "???";
}

string Scale::type_name() const
{
	return get_type_name(type);
}

bool Scale::contains(int pitch) const
{
	return _contains[pitch % 12];
}

inline int apply_modifier(int pitch, int scale_mod, int mod)
{
	int d = 0;
	if (scale_mod == MODIFIER_SHARP)
		d = 1;
	else if (scale_mod == MODIFIER_FLAT)
		d = -1;
	if (mod == MODIFIER_SHARP)
		d += 1;
	else if (mod == MODIFIER_FLAT)
		d -= 1;
	else if (mod == MODIFIER_NATURAL)
		d = 0;
	return pitch + d;
}

// x in major scale notation
int Scale::transform_out(int x, int mod) const
{
	const int pp[7] = {0,2,4,5,7,9,11};

	int octave = x / 7;
	int rel = x % 7;

	return apply_modifier(pitch_from_octave_and_rel(pp[rel], octave), modifiers[rel], mod);
}


