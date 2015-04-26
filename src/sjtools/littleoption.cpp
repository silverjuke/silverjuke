/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2015 Björn Petersen Software Design and Development
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
 * File:    littleoption.cpp
 * Authors: Björn Petersen
 * Purpose: Handling "little" special options
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/littleoption.h>
#include <sjtools/msgbox.h>
#include <wx/tokenzr.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayLittleOption);


/*******************************************************************************
 * SjLittleOption
 ******************************************************************************/


#define STR_SECTION_SEP wxT(": ")


wxString SjLittleOption::s_section;


void SjLittleOption::SetSection(const wxString& section1, const wxString& section2)
{
	s_section = ApplySection(section1, section2);
}


void SjLittleOption::SetSection(const wxString& section)
{
	s_section = section;
}


wxString SjLittleOption::ApplySection(const wxString& section, const wxString& name)
{
	if( section.IsEmpty() )
	{
		return name;
	}
	else if( name.IsEmpty() )
	{
		return section;
	}
	else
	{
		return section + STR_SECTION_SEP + name;
	}
}


wxArrayString SjLittleOption::CreateArray(const wxString& str)
{
	wxArrayString ret;
	if( !str.IsEmpty() )
	{
		wxStringTokenizer tkz(str, wxT("|"), wxTOKEN_RET_EMPTY_ALL);
		while( tkz.HasMoreTokens() )
		{
			ret.Add(tkz.GetNextToken());
		}
	}
	return ret;
}


/*******************************************************************************
 * SjLittleBit
 ******************************************************************************/


SjLittleBit::SjLittleBit(const wxString& name, const wxString& options,
                         long* value, long defaultValue, long bitInBitfield,
                         const wxString& ini, SjIcon icon, bool saveToObject)
	: SjLittleOption(name, icon)
{
	// get options array
	if( options == wxT("yn") )
	{
		m_options = CreateArray(_("No")+wxString(wxT("|"))+_("Yes"));
	}
	else if( options.IsEmpty() || options == wxT("oo") )
	{
		m_options = CreateArray(_("Off")+wxString(wxT("|"))+_("On"));
	}
	else
	{
		m_options = CreateArray(options);
	}

	// get current values - currently we can only save bitfields to the INI
	// if it is also saved to the object as we do not know the default flags here
	wxASSERT( value );
	m_bitInBitfield         = bitInBitfield;
	if( bitInBitfield )
	{
		wxASSERT( saveToObject || ini.IsEmpty() );
		wxASSERT( m_options.GetCount()==2 );
		m_currValue         = ((*value)&bitInBitfield)? 1 : 0;
		m_defaultValue      = defaultValue? 1 : 0;
	}
	else
	{
		m_currValue         = *value;
		m_defaultValue      = defaultValue;
	}
	if( m_currValue < 0 || m_currValue >= (long)m_options.GetCount() )
	{
		m_currValue         = 0;
	}
	wxASSERT( m_defaultValue >= 0 && m_defaultValue < (long)m_options.GetCount() );
	m_onConstructValue      = m_currValue;


	// prepare saving
	m_saveValueToObject     = saveToObject? value : NULL;
	m_savedValue            = m_currValue;
	m_saveValueToIni        = ini;
}


wxString SjLittleBit::GetDisplayValue() const
{
	wxString ret = m_options[m_currValue];
	if( !m_comment.IsEmpty() )
	{
		ret.Append(wxT(" ("));
		ret.Append(m_comment);
		ret.Append(wxT(")"));
	}
	return ret;
}


bool SjLittleBit::OnDoubleClick(wxWindow* parent)
{
	if( !ShowMenuOnDoubleClick() )
	{
		m_currValue++;
		if(m_currValue>=GetOptionCount())
		{
			m_currValue = 0;
		}
		Save();
	}

	return FALSE; // no other options affected
}
void SjLittleBit::Save()
{
	if( m_savedValue != m_currValue )
	{
		if( m_saveValueToObject )
		{
			if( m_bitInBitfield )
			{
				SjTools::SetFlag(*m_saveValueToObject, m_bitInBitfield, m_currValue!=0);
			}
			else
			{
				*m_saveValueToObject = m_currValue;
			}
		}

		if( !m_saveValueToIni.IsEmpty() )
		{
			long valueForIni = m_currValue;
			if( m_bitInBitfield )
			{
				wxASSERT( m_saveValueToObject );
				valueForIni = *m_saveValueToObject;
			}
			g_tools->m_config->Write(m_saveValueToIni, valueForIni);
		}

		m_savedValue = m_currValue;

		OnFeedback();
	}
}


static const long s_cacheValues[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 64, 128, 192, 256, 9999 };
wxString SjLittleBit::GetMbOptions(long maxMb, long& retMb2Index1, long& retMb2Index2)
{
	wxString ret;
	int i;
	long checkMb1 = retMb2Index1, checkMb2 = retMb2Index2;
	retMb2Index1 = 0;
	retMb2Index2 = 0;
	for( i = 0; s_cacheValues[i]!=9999; i++ )
	{
		if( s_cacheValues[i] > maxMb )
		{
			break;
		}

		if( !ret.IsEmpty() ) ret.Append('|');
		ret += SjTools::FormatBytes(s_cacheValues[i]*SJ_ONE_MB);

		if( s_cacheValues[i] == checkMb1 ) retMb2Index1 = i;
		if( s_cacheValues[i] == checkMb2 ) retMb2Index2 = i;
	}
	return ret;
}


long SjLittleBit::Index2Mb(long index)
{
	int i;
	for( i = 0; s_cacheValues[i]!=9999; i++ )
	{
		if( i == index ) return s_cacheValues[i];
	}
	return 1;
}


/*******************************************************************************
 * SjLittleEnumStr
 ******************************************************************************/


SjLittleEnumStr::SjLittleEnumStr(const wxString& name, const wxString& valuesNoptionsStr,
                                 wxString* value, const wxString& defaultValue,
                                 const wxString& ini, SjIcon icon, bool saveToObject)
	: SjLittleOption(name, icon)
{
	// convert the string formatted as "value1|descr1|value2|descr2" into two arrays, long (values) and string (options)
	wxArrayString valuesNoptionsArr = CreateArray(valuesNoptionsStr);
	wxASSERT_MSG( valuesNoptionsArr.GetCount() >= 2, valuesNoptionsStr  );
	wxASSERT_MSG( valuesNoptionsArr.GetCount() % 2 == 0, valuesNoptionsStr );
	while( valuesNoptionsArr.GetCount() < 2 ) { valuesNoptionsArr.Add(wxEmptyString); } // and make sure, we have at least one value-descr-pair
	for( int i = 0; i < (int)valuesNoptionsArr.GetCount(); i+=2 )
	{
		m_values.Add(valuesNoptionsArr[i]);
		m_options.Add(valuesNoptionsArr[i+1]);
	}

	// set and validate the default value
	wxASSERT( m_values.Index(defaultValue)!=wxNOT_FOUND );
	m_defaultValue = (m_values.Index(defaultValue)!=wxNOT_FOUND)? defaultValue : m_values[0];

	// set and validate the current value
	wxASSERT( value );
	m_currValue             = (value && m_values.Index(*value)!=wxNOT_FOUND)? *value : m_defaultValue;
	m_onConstructValue      = m_currValue;
	m_savedValue            = m_currValue;
	m_saveValueToObject     = saveToObject? value : NULL;
	m_saveValueToIni        = ini;
}


wxString SjLittleEnumStr::GetDisplayValue() const
{
	long i = m_values.Index(m_currValue);
	if( i == wxNOT_FOUND )
	{
		return wxEmptyString;
	}
	return m_options[i];
}


void SjLittleEnumStr::Save()
{
	if( m_savedValue != m_currValue )
	{
		if( m_saveValueToObject )
		{
			*m_saveValueToObject = m_currValue;
		}

		if( !m_saveValueToIni.IsEmpty() )
		{
			g_tools->m_config->Write(m_saveValueToIni, m_currValue);
		}

		m_savedValue = m_currValue;

		OnFeedback();
	}
}



/*******************************************************************************
 * SjLittleEnumLong
 ******************************************************************************/


SjLittleEnumLong::SjLittleEnumLong(const wxString& name, const wxString& valuesNoptionsStr,
                                 long* value, long defaultValue,
                                 const wxString& ini, SjIcon icon, bool saveToObject)
	: SjLittleOption(name, icon)
{
	// convert the string formatted as "value1|descr1|value2|descr2" into two arrays, long (values) and string (options)
	wxArrayString valuesNoptionsArr = CreateArray(valuesNoptionsStr);
	wxASSERT_MSG( valuesNoptionsArr.GetCount() >= 2, valuesNoptionsStr  );
	wxASSERT_MSG( valuesNoptionsArr.GetCount() % 2 == 0, valuesNoptionsStr );
	while( valuesNoptionsArr.GetCount() < 2 ) { valuesNoptionsArr.Add(wxEmptyString); } // and make sure, we have at least one value-descr-pair
	for( int i = 0; i < (int)valuesNoptionsArr.GetCount(); i+=2 )
	{
		long temp;
		valuesNoptionsArr[i].ToLong(&temp, 10);
		m_values.Add(temp);
		m_options.Add(valuesNoptionsArr[i+1]);
	}

	// set and validate the default value
	wxASSERT( m_values.Index(defaultValue)!=wxNOT_FOUND );
	m_defaultValue = (m_values.Index(defaultValue)!=wxNOT_FOUND)? defaultValue : m_values[0];

	// set and validate the current value
	wxASSERT( value );
	m_currValue             = (value && m_values.Index(*value)!=wxNOT_FOUND)? *value : m_defaultValue;
	m_onConstructValue      = m_currValue;
	m_savedValue            = m_currValue;
	m_saveValueToObject     = saveToObject? value : NULL;
	m_saveValueToIni        = ini;
}


wxString SjLittleEnumLong::GetDisplayValue() const
{
	long i = m_values.Index(m_currValue);
	if( i == wxNOT_FOUND )
	{
		return wxEmptyString;
	}
	return m_options[i];
}


void SjLittleEnumLong::Save()
{
	if( m_savedValue != m_currValue )
	{
		if( m_saveValueToObject )
		{
			*m_saveValueToObject = m_currValue;
		}

		if( !m_saveValueToIni.IsEmpty() )
		{
			g_tools->m_config->Write(m_saveValueToIni, m_currValue);
		}

		m_savedValue = m_currValue;

		OnFeedback();
	}
}


/*******************************************************************************
 * SjLittleStringSel
 ******************************************************************************/


SjLittleStringSel::SjLittleStringSel(const wxString& name,
                                     wxString* string, const wxString& defaultString,
                                     const wxString& ini, SjIcon icon, bool saveToObject)
	: SjLittleOption(name, icon)
{
	m_currString            = string? *string : g_tools->m_config->Read(ini, defaultString);
	m_onConstructString     = m_currString;
	m_defaultString         = defaultString;

	wxASSERT( saveToObject == FALSE || string != NULL );
	m_saveStringToObject    = saveToObject? string : NULL;

	m_savedString           = m_currString;
	m_saveStringToIni       = ini;
}


bool SjLittleStringSel::OnOption(wxWindow* parent, long i)
{
	wxWindowDisabler disabler(parent);
	wxTextEntryDialog textEntry(parent, wxEmptyString, GetName(), m_currString);
	if( textEntry.ShowModal() == wxID_OK )
	{
		m_currString = textEntry.GetValue();
		Save();
	}

	return FALSE; // no other options affected
}



void SjLittleStringSel::Save()
{
	if( m_savedString != m_currString )
	{
		if( m_saveStringToObject )
		{
			*m_saveStringToObject = m_currString;
		}

		if( !m_saveStringToIni.IsEmpty() )
		{
			if( m_currString == m_defaultString )
			{
				if( g_tools->m_config->HasEntry(m_saveStringToIni) )
				{
					g_tools->m_config->DeleteEntry(m_saveStringToIni);
				}
			}
			else
			{
				g_tools->m_config->Write(m_saveStringToIni, m_currString);
			}
		}

		m_savedString = m_currString;

		OnFeedback();
	}
}


/*******************************************************************************
 *  SjLittleDirSel
 ******************************************************************************/


SjLittleDirSel::SjLittleDirSel( const wxString& name,
                                const wxString& dir, const wxString& defaultDir,
                                SjIcon icon)
	: SjLittleOption(name, icon)
{
	m_onConstructDir    = NormalizeDir(dir);
	m_currDir           = m_onConstructDir;
	m_defaultDir        = NormalizeDir(defaultDir);
}


wxString SjLittleDirSel::NormalizeDir(const wxString& dir)
{
	// makes sure, the file is correct
	wxFileName fn;
	fn.AssignDir(dir);

	// finalize the result, make "long" paths, make sure the dir ends with a (back)slash
	wxString ret = fn.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR);
	#ifdef __WXMSW__
		fn.AssignDir(ret);
		ret = fn.GetLongPath();
		if( ret.Last() != '\\' && ret.Last() != '/' )
		{
			ret += wxT("\\");
		}
	#endif
	return ret;
}


bool SjLittleDirSel::OnOption(wxWindow* parent, long i)
{
	if( i == 0 )
	{
		wxWindowDisabler disabler(parent);

		::wxBeginBusyCursor(wxHOURGLASS_CURSOR);
		wxDirDialog dirDialog(parent, GetName(), m_currDir);
		::wxEndBusyCursor();

		if( dirDialog.ShowModal() == wxID_OK )
		{
			wxFileName fn(dirDialog.GetPath());
			fn.Normalize();
			m_currDir = NormalizeDir(fn.GetFullPath());
		}
	}
	else
	{
		g_tools->ExploreUrl(m_currDir);
	}

	return FALSE; // no other options affected
}


/*******************************************************************************
 * SjLittleReadOnly
 ******************************************************************************/


SjLittleReadOnly::SjLittleReadOnly(const wxString& name, const wxString& value,
                                   bool modified, const wxString& dlbClickComment,
                                   SjIcon icon)
	: SjLittleOption(name, icon)
{
	m_modified = modified;
	m_displayValue = value;
	m_dblClickComment = dlbClickComment;
}


bool SjLittleReadOnly::OnOption(wxWindow* parent, long optionIndex)
{
	if( optionIndex == 0 )
	{
		wxWindowDisabler disabler(parent);
		wxString msg = m_displayValue;
		if( !m_dblClickComment.IsEmpty() )
		{
			msg << wxT("\n\n") << m_dblClickComment;
		}
		::SjMessageBox(msg, GetName(), wxOK, parent);
	}
	else
	{
		g_tools->ExploreUrl(m_displayValue);
	}

	return FALSE; // no other options affected
}


/*******************************************************************************
 * SjLittleExploreSetting (very special)
 ******************************************************************************/


const wxChar* SjLittleExploreSetting::s_explorePrograms[] = {
#ifdef __WXMAC__
	wxT("Finder"),
#else
	wxT("Explore"), /* first is default */
	wxT("Open"),
#endif
	NULL
};


SjLittleExploreSetting::SjLittleExploreSetting(const wxString& name)
	: SjLittleOption(name)
{
	// add predefined options
	const wxChar** p = s_explorePrograms;
	while( *p )
	{
		AddOption(*p);
		p++;
	}

	// add user defined options - the last option in the array will be saved in OnApply()
	AddOption(g_tools->m_exploreProgram);
	AddOption(g_tools->m_config->Read(wxT("main/exploreAlt"), wxT("")));

	// remember user selection
	m_value = g_tools->m_exploreProgram;
}


void SjLittleExploreSetting::AddOption(const wxString& option)
{
	if( !option.IsEmpty() && m_options.Index(option) == wxNOT_FOUND )
	{
		m_options.Add(option);
	}
}


bool SjLittleExploreSetting::IsModified() const
{
	return (m_value!=s_explorePrograms[0]);
}


long SjLittleExploreSetting::GetOptionCount() const
{
	return m_options.GetCount() + 4/*seperator, "select...", "edit..." and "test..."*/;
}


wxString SjLittleExploreSetting::GetOption(long i) const
{
	if( i>=0 && i<(int)m_options.GetCount() )
	{
		wxString ret = m_options[i];
		if( ret.Len() > 50 )
		{
			ret = ret.Left(45).Trim()+wxT("...");
		}
		return ret;
	}
	else if( i==GetOptionCount()-3 )
	{
		return _("Select...");
	}
	else if( i==GetOptionCount()-2 )
	{
		return _("Edit...");
	}
	else if( i==GetOptionCount()-1 )
	{
		return _("Test...");
	}
	else
	{
		return wxT("");
	}
}


bool SjLittleExploreSetting::IsOptionChecked(long i) const
{
	if( i>=0 && i<(int)m_options.GetCount() )
	{
		return (m_value==m_options[i]);
	}
	else
	{
		return FALSE;
	}
}


long SjLittleExploreSetting::IsOptionCheckable(long i) const
{
	if( i>=0 && i<(int)m_options.GetCount() )
	{
		return 2; // radio
	}
	return  0;
}


bool SjLittleExploreSetting::IsOptionEnabled(long i) const
{
	if( i==GetOptionCount()-2 )
	{
		// edit...
		return !IsPredefined(m_value);
	}
	else
	{
		// other
		return TRUE;
	}
}


bool SjLittleExploreSetting::OnOption(wxWindow* parent, long i)
{
	if( i>=0 && i<(int)m_options.GetCount() )
	{
		// select a predefined selection -- parent may be NULL!!!
		m_value = m_options[i];
	}
	else if( i==GetOptionCount()-3 && parent )
	{
		// select...
		wxWindowDisabler disabler(parent);

		wxString wildcards = wxString::Format(wxT("%s (*.*)|*.*"), _("Programs"));
		wxFileDialog fileDialog(parent, GetName(), wxT(""), wxT(""), wildcards, wxFD_OPEN|wxFD_CHANGE_DIR);
		if( fileDialog.ShowModal() == wxID_OK )
		{
			m_value = wxString::Format(wxT("\"%s\" \"<folder>\""), fileDialog.GetPath().c_str());
			AddOption(m_value);
		}
	}
	else if( i==GetOptionCount()-2 && parent )
	{
		// edit...
		wxWindowDisabler disabler(parent);

		wxTextEntryDialog textEntry(parent,
		                            wxString(_("Placeholders:"))+wxT(" <folder>, <file>"),
		                            GetName(), m_value);
		if( textEntry.ShowModal() == wxID_OK )
		{
			m_value = textEntry.GetValue();
			AddOption(m_value);
		}
	}
	else if( i==GetOptionCount()-1 )
	{
		// test...
		wxFileName home;
		home.AssignHomeDir();
		g_tools->ExploreFile_(home.GetFullPath(), m_value);
	}

	return FALSE; // no other options affected
}


bool SjLittleExploreSetting::IsPredefined(const wxString& option) const
{
	const wxChar** p = s_explorePrograms;
	while( *p )
	{
		if( option == *p  )
		{
			return TRUE;
		}
		p++;
	}
	return FALSE;
}


void SjLittleExploreSetting::OnApply()
{
	if( m_value != g_tools->m_exploreProgram )
	{
		// save the last option that is not the current selection;
		// we re-add the old value to allow getting it back later
		m_options.Add(g_tools->m_exploreProgram);
		int i;
		for( i = (int)m_options.GetCount()-1; i>=0; i-- )
		{
			if( m_options[i] != m_value && !IsPredefined(m_options[i]) )
			{
				g_tools->m_config->Write(wxT("main/exploreAlt"), m_options[i]);
				break;
			}
		}

		// save the new value
		g_tools->m_exploreProgram = m_value;
		g_tools->m_config->Write(wxT("main/exploreWith"), m_value);
	}
}

