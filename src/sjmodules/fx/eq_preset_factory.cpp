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


void SjEqPresetFactory::AddPreset(const wxString& name__, const SjEqParam& param)
{
	// if a preset with the given name exists, it is overwritten. Private function, does not save anything

	wxString name(name__);
	name.Trim(true);
	name.Trim(false); // needed as we add a spave eg. in SaveAllPresets()

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


void SjEqPresetFactory::DeletePreset(const wxString& name)
{
	// Private functions, does not save anything
    SjEqPreset* preset = (SjEqPreset*)m_hash.Remove(name);
    if( preset ) {
		delete preset;
    }
}


void SjEqPresetFactory::AddDefaultPresets()
{
	AddPreset("Null", SjEqParam("0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0"));
	AddPreset("Soft Bass", SjEqParam("3;5;4;0;-13;-7;-5;-5;-1;2;5;1;-1;-1;-2;-7;-9;-14"));
}


void SjEqPresetFactory::LoadAllPresets()
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
				AddPreset(name, SjEqParam(expl[i+1]));
			}
		}
	}

	// no presets loaded - add defaults
	if( m_hash.GetCount() == 0 ) {
		AddDefaultPresets();
	}
}


void SjEqPresetFactory::SaveAllPresets()
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
	LoadAllPresets();

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


SjEqPreset SjEqPresetFactory::GetPresetByName(const wxString& nameAndOptParam)
{
	LoadAllPresets();

	wxString  name = nameAndOptParam.BeforeFirst('|'); // whole string, if there is no '|'
	SjEqParam wantedParam(nameAndOptParam.AfterFirst('|'));  // empty, if there is no '|'

	// try to find out the preset by the name
	name.Trim(true);
	name.Trim(false);
	SjEqPreset* preset = (SjEqPreset*)m_hash.Lookup(name);
	if( preset ) {
		return *preset;
	}

	// try to find out the preset by the parameters
	return GetPresetByParam(wantedParam);
}



SjEqPreset SjEqPresetFactory::GetPresetByParam(const SjEqParam& wantedParam)
{
	LoadAllPresets();

	// convert eq-parameters to preset name
	wxString       key;
	SjEqPreset*    preset;
	SjHashIterator iterator;
	while( (preset=(SjEqPreset*)m_hash.Iterate(iterator, key)) ) {
		if( preset->m_param == wantedParam ) {
			return *preset;
		}
	}

	return SjEqPreset("" /*no preset*/, wantedParam);
}
