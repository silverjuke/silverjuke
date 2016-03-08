/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2016 Björn Petersen Software Design and Development
 *                   Contact: r10s@b44t.com, http://b44t.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see http://www.gnu.org/licenses/ .
 *
 *******************************************************************************
 *
 * File:    eq_preset_factory.cpp
 * Authors: Björn Petersen
 * Purpose: Handle equalizer presets
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/fx/eq_preset_factory.h>


SjEqPresetFactory::SjEqPresetFactory()
{
	m_allPresetsLoaded = false;
}


SjEqPresetFactory::~SjEqPresetFactory()
{
	wxString       key;
	SjEqPreset*    preset;
	SjHashIterator iterator;
	while( (preset=(SjEqPreset*)m_hash.Iterate(iterator, key)) ) {
		delete preset;
	}
}


void SjEqPresetFactory::hash_preset(const wxString& name__, const SjEqParam& param)
{
	// if a preset with the given name exists, it is overwritten. Private function, does not save anything

	wxString name(name__);
	name.Trim(true);
	name.Trim(false); // needed as we add a spave eg. in SaveAllPresets()
	name.Replace("|", "");

	SjEqPreset* preset = (SjEqPreset*)m_hash.Lookup(name);
	if( preset )
	{
		// modify existing preset
		preset->m_param = param;
	}
	else
	{
		// add new preset
		preset = new SjEqPreset(name, param);
		if( preset == NULL ) { return; } // error
		m_hash.Insert(name, preset);
	}
}


void SjEqPresetFactory::unhash_preset(const wxString& name)
{
	// Private functions, does not save anything
    SjEqPreset* preset = (SjEqPreset*)m_hash.Remove(name);
    if( preset ) {
		delete preset;
    }
}


void SjEqPresetFactory::hash_default_presets()
{
	hash_preset("Bass Boost",   SjEqParam("0;0;-0.4;-1.3;-2.8;-4.2;-4.8;-5.3;-6;-6;-6;-6;-6;-6;-6;-6;-6;-6"));
	hash_preset("Bass Reduce",  SjEqParam("-4.9;-3.9;-3;-2.1;-1.9;-1.7;-1.3;-0.7;0;0;0;0;0;0;0;0;0;0"));
	hash_preset("Classic",      SjEqParam("0;-0.6;-1.7;-2.4;-2.8;-3.6;-5.3;-5.6;-5.6;-5.6;-4.5;-4.5;-2.8;-2.2;-1.3;-1.3;-0.6;-0.6"));
	hash_preset("Club",         SjEqParam("-3.9;-3.5;-2.5;-1.2;-1.2;-1.2;-0.5;-0.5;0;0;-1.2;-1.2;-2.3;-2.3;-3.3;-3.3;-3.9;-3.9"));
	hash_preset("Dance",        SjEqParam("-2.8;0;-1.5;-6.2;-6.2;-6.2;-4.3;-4.3;-2.8;-2.8;-1.5;-1.5;-2.4;-2.4;-3.4;-3.4;-6.6;-6.6"));
	hash_preset("Deep In",      SjEqParam("0;-1.1;-2.8;-3.8;-3.8;-3.8;-1.9;-1.9;-2.6;-2.6;-2.8;-2.8;-6;-6;-7.5;-7.5;-8.6;-8.6"));
	hash_preset("Electric Cafe",SjEqParam("-0.4;-0.9;-3;-4.7;-4.7;-4.7;-6;-6;-2.2;-2.2;-3.9;-3.9;-3.2;-3.2;-0.6;-0.6;0;0"));
	hash_preset("Flat",         SjEqParam("0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0"));
	hash_preset("Grand Piano",  SjEqParam("-1.7;-2.8;-4.3;-1.7;-1.7;-1.7;-1.3;-1.3;-2.6;-2.6;-0.6;-0.6;0;0;-1.5;-1.5;-0.7;-0.7"));
	hash_preset("Hip Hop",      SjEqParam("0;-1.3;-3.4;-2.1;-2.1;-2.1;-6;-6;-5.4;-5.4;-3.2;-3.2;-4.7;-4.7;-2.4;-2.4;-1.5;-1.5"));
	hash_preset("Jazz",         SjEqParam("-0.4;-1.3;-2.8;-2.2;-2.2;-2.2;-5.4;-5.4;-5.2;-5.2;-4.4;-3.4;-2.6;-2.1;-1.3;-0.4;0;0"));
	hash_preset("Latin",        SjEqParam("0;-1.9;-4.7;-4.7;-4.7;-4.7;-5.8;-5.8;-5.8;-5.8;-6;-6;-4.7;-4.7;-1.9;-1.9;-0.6;-0.6"));
	hash_preset("Loudness",     SjEqParam("0;-1.9;-5.8;-6.2;-6.2;-6.2;-7.5;-7.5;-6.4;-6.4;-7.1;-7.1;-10.7;-10.7;-1.7;-1.7;-4.7;-4.7"));
	hash_preset("Lounge",       SjEqParam("-6.9;-5.6;-4.7;-2.3;-2.3;-2.3;0;0;-1.3;-1.3;-3.6;-3.6;-5.5;-5.5;-1.8;-1.8;-3.2;-3.2"));
	hash_preset("Pop",          SjEqParam("-5.6;-5.1;-3.8;-2.1;-2.1;-2.1;0;0;0;0;-1.8;-1.8;-4.2;-4.2;-5.1;-5.1;-6.3;-6.3"));
	hash_preset("Reggae",       SjEqParam("-4.2;-4.2;-4.2;-5.6;-5.6;-5.6;-8.4;-8.4;-4.2;-4.2;0;0;-0.3;-0.3;-4.2;-4.2;-4.2;-4.2"));
	hash_preset("RnB",          SjEqParam("-4;0;-0.9;-5.6;-5.6;-5.6;-9.1;-9.1;-8.4;-8.4;-4.2;-4.2;-4.2;-4.2;-4.2;-4.2;-2.8;-2.8"));
	hash_preset("Rock",         SjEqParam("0;-1.2;-2.1;-3.5;-3.5;-3.5;-5.4;-5.4;-6.3;-6.3;-4.4;-4.4;-2.6;-2.6;-1.4;-1.4;-0.2;-0.2"));
	hash_preset("Ska",          SjEqParam("-5.3;-6.8;-4.2;-3.8;-3.8;-3.8;-2.8;-2.8;-1;-1;-0.3;-0.3;0;0;0;0;0;0"));
	hash_preset("Speech",       SjEqParam("-5.8;-7.2;-7;-2.6;-2.6;-2.6;0;0;0;0;-1.2;-1.2;-2.3;-2.3;-4.2;-4.2;-5.4;-5.4"));
	hash_preset("Techno",       SjEqParam("0;-2;-5.1;-7.2;-7.2;-7.2;-9.6;-9.6;-7.2;-7.2;-4.5;-4.5;-3.3;-3.3;-3.3;-3.3;-3.5;-3.5"));
	hash_preset("Tiny Speakers",SjEqParam("0;-1.2;-2.1;-3;-3;-3.8;-4.5;-4.7;-5.3;-5.8;-6.8;-7.2;-8.3;-8.4;-9.3;-9.3;-9.8;-10.5"));
	hash_preset("Treble Boost", SjEqParam("-6;-5.5;-5.8;-6;-6;-6;-6;-6;-4.2;-4.2;-3.3;-3.3;-1.9;-1.9;-1.2;-1.2;0;0"));
	hash_preset("Treble Reduce",SjEqParam("-0.6;0;-0.4;-0.6;-0.6;-0.6;-0.6;-0.6;-1.5;-1.5;-2.4;-2.4;-4.1;-4.1;-4.5;-4.5;-6;-6"));
}


void SjEqPresetFactory::load_all_presets()
{
	// make sure, we do this only once - NB: we do not call SaveAllPresets() from here - this is not needed as we can always recreate the defaults here
	if( m_allPresetsLoaded ) { return; }
	m_allPresetsLoaded = true;

	// load presets from the database (we treat the eq presets to be (more) part of the music library)
	wxSqlt   sql;
	wxString allPresets = sql.ConfigRead("library/eqPresets", ""), name;
	if( allPresets != "" ) {
		wxArrayString expl = SjTools::Explode(allPresets, '|', 0);
		for( size_t i = 0; i < expl.GetCount()-1; i += 2 ) {
			name = expl[i]; name.Trim(true); name.Trim(false);
			if( !name.IsEmpty() ) {
				hash_preset(name, SjEqParam(expl[i+1]));
			}
		}
	}

	// no presets loaded - add defaults
	if( m_hash.GetCount() == 0 ) {
		hash_default_presets();
	}
}


void SjEqPresetFactory::save_all_presets()
{
	if( !m_allPresetsLoaded ) { return; } // error - if the presets are not loaded, we cannot save them

	wxString       allPresets;

	wxSqlt         sql;
	wxString       key;
	SjEqPreset*    preset;
	SjHashIterator iterator;
	while( (preset=(SjEqPreset*)m_hash.Iterate(iterator, key)) ) {
		allPresets += key + "|" + preset->m_param.ToString() + "| "; // add a finaly space to avoid strings that cannot word break
	}

	sql.ConfigWrite("library/eqPresets", allPresets);
}


wxArrayString SjEqPresetFactory::GetNames()
{
	load_all_presets();

	wxArrayString  ret;
	wxString       key;
	SjEqPreset*    preset;
	SjHashIterator iterator;
	while( (preset=(SjEqPreset*)m_hash.Iterate(iterator, key)) ) {
		ret.Add(key);
	}

	ret.Sort();
	return ret;
}


SjEqPreset SjEqPresetFactory::GetPresetByName(const wxString& name__)
{
	load_all_presets();

	// try to find out the preset by the name
	wxString name(name__);
	name.Trim(true);
	name.Trim(false);
	SjEqPreset* preset = (SjEqPreset*)m_hash.Lookup(name);
	if( preset ) {
		return *preset;
	}

	// nothing found, return empty preset
	return SjEqPreset("", SjEqParam());
}



SjEqPreset SjEqPresetFactory::GetPresetByParam(const SjEqParam& wantedParam, const wxString& nameHint)
{
	// the name is _only_ used, if the there are several presets with the same parameters.
	// If there are no matching parameters, the function returns an unamed preset with the given parameters - even if there is a preset with the name!
	load_all_presets();

	// convert eq-parameters to preset name
	wxString       key;
	SjEqPreset*    preset;
	SjHashIterator iterator;
	SjEqPreset     possiblePreset("" /*no preset*/, wantedParam);
	while( (preset=(SjEqPreset*)m_hash.Iterate(iterator, key)) ) {
		if( preset->m_param == wantedParam ) {
			if( preset->m_name == nameHint ) {
				return *preset; // perfect match
			}
			else {
				possiblePreset = *preset; // match, however, continue search - maybe we'll find even a matching name
			}
		}
	}

	return possiblePreset;
}
