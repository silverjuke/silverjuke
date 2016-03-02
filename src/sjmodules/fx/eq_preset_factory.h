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
 * File:    eq_preset_factory.h
 * Authors: Björn Petersen
 * Purpose: Handle equalizer presets
 *
 ******************************************************************************/


#ifndef __SJ_EQ_PRESET_FACTORY_H__
#define __SJ_EQ_PRESET_FACTORY_H__


class SjEqPreset
{
public:
	                SjEqPreset          () { }
	                SjEqPreset          (const wxString& n, const SjEqParam& p) { m_name=n; m_param=p; }
	                SjEqPreset          (const SjEqPreset& o) { CopyFrom(o); }
	void            CopyFrom            (const SjEqPreset& o) { m_name=o.m_name; m_param=o.m_param; }
	SjEqPreset&     operator =          (const SjEqPreset& o) { CopyFrom(o); return *this; }

	wxString        m_name;
	SjEqParam       m_param;
};


class SjEqPresetFactory
{
public:
	                SjEqPresetFactory   ();
	                ~SjEqPresetFactory  ();
	void            AddDefaultPresets   () { hash_default_presets(); save_all_presets(); }
	wxArrayString   GetNames            ();
	SjEqPreset      GetPresetByName     (const wxString&);
	SjEqPreset      GetPresetByParam    (const SjEqParam&);
	void            AddPreset           (const wxString& name, const SjEqParam& param) { hash_preset(name, param), save_all_presets(); }
	void            DeletePreset        (const wxString& name) { unhash_preset(name); save_all_presets(); }

private:
	void            load_all_presets    ();
	void            save_all_presets    ();
	void            hash_default_presets();
	void            hash_preset         (const wxString& name, const SjEqParam& param);
	void            unhash_preset       (const wxString& name);
	bool            m_allPresetsLoaded;
	SjSPHash        m_hash;
};


#endif // __SJ_EQ_PRESET_FACTORY_H__

