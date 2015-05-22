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
 * File:    littleoption.h
 * Authors: Björn Petersen
 * Purpose: Handling "little" special options
 *
 ******************************************************************************/


#ifndef __SJ_LITTLEOPTION_H__
#define __SJ_LITTLEOPTION_H__


#define SJ_ICON_LITTLEDEFAULT SJ_ICON_SWITCH


class SjLittleOption
{
public:
	SjLittleOption                      (const wxString& name, SjIcon icon=SJ_ICON_LITTLEDEFAULT) { m_name = ApplySection(s_section, name); m_icon = icon; }
	virtual         ~SjLittleOption     () { }
	static void     SetSection          (const wxString&);
	static void     SetSection          (const wxString&, const wxString&);
	static void     ClearSection        () { SetSection(wxEmptyString); }
	static wxString ApplySection        (const wxString& section, const wxString& name);

	// get information
	virtual wxString GetName            () const { return m_name; }
	virtual SjIcon  GetIcon             () const { return m_icon; };
	virtual wxString GetDisplayValue    () const { return wxT("***"); }
	virtual bool    IsModified          () const { return FALSE; }
	virtual bool    IsReadOnly          () const { return FALSE; }

	// handling menu options
	// OnOption() should return TRUE if other options may be affected by the operation
	virtual long    GetOptionCount      () const { return 0; }
	virtual wxString GetOption          (long i) const { return wxT("***"); }
	virtual bool    IsOptionChecked     (long i) const { return FALSE; }
	virtual long    IsOptionCheckable   (long i) const { return 0; } // 0=no, 1=checkable, 2=radio
	virtual bool    IsOptionEnabled     (long i) const { return TRUE; }
	virtual bool    OnOption            (wxWindow* parent, long i) { return FALSE; }

	// handling double clicks,
	// OnDoubleClick() should return TRUE if other options may be affected by the operation
	virtual bool    OnDoubleClick       (wxWindow* parent) { return FALSE; }
	virtual bool    ShowMenuOnDoubleClick() { return FALSE; }

	// Handling default command.
	// OnDefault() should return TRUE if other options may be affected by the operation.
	// If no parent is given, the function should not request any user feedback; this is
	// true eg. when resetting all options in a list of options
	virtual bool    OnDefault           (wxWindow* parent) { return FALSE; }

	// handling cancel/save command
	virtual void    OnApply             () { }
	virtual void    OnCancel            () { }

	// the following stuff may be used by classes derived by the user
	virtual void    OnFeedback          () { }

protected:
	wxArrayString   CreateArray         (const wxString&);

private:
	wxString        m_name;
	SjIcon          m_icon;
	static wxString s_section;
};


class SjLittleBit : public SjLittleOption
{
public:
	SjLittleBit         (const wxString& name, const wxString& options,
	                     long* value, long defaultValue, long bitInBitfield,
	                     const wxString& ini=wxEmptyString, SjIcon icon=SJ_ICON_LITTLEDEFAULT, bool saveToObject=TRUE);
	void            SetComment          (const wxString& comment) { m_comment=comment; }

	// get information
	wxString        GetDisplayValue     () const;
	long            GetLong_            () const { return m_currValue; }
	bool            IsModified          () const { return (m_currValue!=m_defaultValue); }

	// handling menu options
	long            GetOptionCount      () const { return (long)m_options.GetCount(); }
	wxString        GetOption           (long i) const { return m_options[i]; }
	bool            IsOptionChecked     (long i) const { return (i==m_currValue); }
	long            IsOptionCheckable   (long i) const { return 2; /*radio*/ }
	bool            OnOption            (wxWindow* parent, long i) { m_currValue=i; Save(); return FALSE; }
	bool            ShowMenuOnDoubleClick() { return (GetOptionCount() > 2); }
	bool            OnDoubleClick       (wxWindow* parent);
	bool            OnDefault           (wxWindow* parent) { m_currValue=m_defaultValue; Save(); return FALSE; }
	void            OnCancel            () { m_currValue=m_onConstructValue; Save(); }

	// some static functions for the creation of the option array
	static wxString GetMbOptions        (long maxMb, long& retMb2Index1, long& retMb2Index2);
	static long     Index2Mb            (long index);

private:
	wxArrayString   m_options;
	wxString        m_comment;

	long            m_bitInBitfield;
	long            m_currValue;
	long            m_defaultValue;
	long            m_onConstructValue;

	void            Save                ();
	long            m_savedValue;
	long*           m_saveValueToObject;
	wxString        m_saveValueToIni;
};


class SjLittleEnumStr : public SjLittleOption
{
public:
	SjLittleEnumStr     (const wxString& name, const wxString& valuesNoptions, // value1|descr1|value2|descr2
	                     wxString* value, const wxString& defaultValue,
	                     const wxString& ini=wxEmptyString, SjIcon icon=SJ_ICON_LITTLEDEFAULT, bool saveToObject=TRUE);
	// get information
	wxString        GetDisplayValue     () const;
	bool            IsModified          () const { return (m_currValue!=m_defaultValue); }

	// handling menu options
	long            GetOptionCount      () const { return (long)(m_options.GetCount()); }
	wxString        GetOption           (long i) const { return m_options[i]; }
	bool            IsOptionChecked     (long i) const { return m_values[i]==m_currValue; }
	long            IsOptionCheckable   (long i) const { return 2; /*radio*/ }
	bool            OnOption            (wxWindow* parent, long i) { m_currValue=m_values[i]; Save(); return false; }
	bool            ShowMenuOnDoubleClick() { return true; }
	bool            OnDefault           (wxWindow* parent) { m_currValue=m_defaultValue; Save(); return FALSE; }
	void            OnCancel            () { m_currValue=m_onConstructValue; Save(); }

protected:
	wxArrayString   m_values;
	wxArrayString   m_options;

	wxString        m_currValue;
	wxString        m_defaultValue;
	wxString        m_onConstructValue;

	void            Save                ();
	wxString        m_savedValue;
	wxString*       m_saveValueToObject;
	wxString        m_saveValueToIni;
};


class SjLittleEnumLong : public SjLittleOption
{
public:
	SjLittleEnumLong    (const wxString& name, const wxString& valuesNoptions, // value1|descr1|value2|descr2
	                     long* value, long defaultValue,
	                     const wxString& ini=wxEmptyString, SjIcon icon=SJ_ICON_LITTLEDEFAULT, bool saveToObject=TRUE);
	// get information
	wxString        GetDisplayValue     () const;
	bool            IsModified          () const { return (m_currValue!=m_defaultValue); }

	// handling menu options
	long            GetOptionCount      () const { return (long)(m_options.GetCount()); }
	wxString        GetOption           (long i) const { return m_options[i]; }
	bool            IsOptionChecked     (long i) const { return m_values[i]==m_currValue; }
	long            IsOptionCheckable   (long i) const { return 2; /*radio*/ }
	bool            OnOption            (wxWindow* parent, long i) { m_currValue=m_values[i]; Save(); return false; }
	bool            ShowMenuOnDoubleClick() { return true; }
	bool            OnDefault           (wxWindow* parent) { m_currValue=m_defaultValue; Save(); return FALSE; }
	void            OnCancel            () { m_currValue=m_onConstructValue; Save(); }

protected:
	wxArrayLong     m_values;
	wxArrayString   m_options;

	long        	m_currValue;
	long	        m_defaultValue;
	long	        m_onConstructValue;

	void            Save                ();
	long	        m_savedValue;
	long*       	m_saveValueToObject;
	wxString        m_saveValueToIni;
};


class SjLittleStringSel : public SjLittleOption
{
public:
	SjLittleStringSel   (const wxString& name,
	                     wxString* string, const wxString& defaultString,
	                     const wxString& ini=wxEmptyString, SjIcon icon=SJ_ICON_LITTLEDEFAULT, bool saveToObject=TRUE);

	// get information
	wxString        GetDisplayValue     () const { return m_currString; }
	bool            IsModified          () const { return (m_currString!=m_defaultString); }

	// handling menu options
	long            GetOptionCount      () const { return 1; }
	wxString        GetOption           (long i) const { return _("Edit..."); }
	bool            OnOption            (wxWindow* parent, long i);
	bool            OnDoubleClick       (wxWindow* parent) { return OnOption(parent, 0); }
	bool            OnDefault           (wxWindow* parent) { m_currString = m_defaultString; Save(); return FALSE; }
	void            OnCancel            () { m_currString = m_onConstructString; Save(); }

private:
	wxString        m_currString;
	wxString        m_defaultString;
	wxString        m_onConstructString;

	void            Save                ();
	wxString        m_savedString;
	wxString*       m_saveStringToObject;
	wxString        m_saveStringToIni;
};


class SjLittleDirSel : public SjLittleOption
{
public:
	SjLittleDirSel      (const wxString& name, const wxString& dir, const wxString& defaultDir, SjIcon icon=SJ_ICON_LITTLEDEFAULT);

	// get information
	wxString        GetDisplayValue     () const { return m_currDir; }
	bool            IsModified          () const { return (m_currDir!=m_defaultDir); }

	// handling menu options
	long            GetOptionCount      () const { return 2; }
	wxString        GetOption           (long i) const { if(i==0) { return _("Select..."); } else { return _("Show file"); } }
	bool            OnOption            (wxWindow* parent, long i);
	bool            OnDoubleClick       (wxWindow* parent) { return OnOption(parent, 0); }
	bool            OnDefault           (wxWindow* parent) { m_currDir = m_defaultDir; return FALSE; }

	// apply - this should be implemented in derived classes
	virtual void    OnApply             () = 0;

	// tools
	static wxString NormalizeDir        (const wxString& dir);

protected:
	wxString        m_currDir;
	wxString        m_defaultDir;
	wxString        m_onConstructDir;
};


class SjLittleReadOnly : public SjLittleOption
{
public:
	SjLittleReadOnly    (const wxString& name, const wxString& value,
	                     bool modified, // even read-only values may be modified eg. by the command-line
	                     const wxString& dlbClickComment=wxEmptyString,
	                     SjIcon icon=SJ_ICON_LITTLEDEFAULT);

	// get information
	wxString        GetDisplayValue     () const { return m_displayValue; }
	bool            IsModified          () const { return m_modified; }
	bool            IsReadOnly          () const { return TRUE; }

	// handling menu options
	long            GetOptionCount      () const { return 1+(g_tools->IsUrlExplorable(m_displayValue)?1:0); }
	wxString        GetOption           (long i) const { if(i==0) { return _("Info..."); } else { return _("Show file"); } }
	bool            OnOption            (wxWindow* parent, long i);
	bool            OnDoubleClick       (wxWindow* parent) { return OnOption(parent, 0); }

protected:
	wxString        m_displayValue;
	wxString        m_dblClickComment;
	bool            m_modified;
};


class SjLittleExploreSetting : public SjLittleOption
{
public:
	SjLittleExploreSetting  (const wxString& name);
	bool            IsModified              () const;
	wxString        GetDisplayValue         () const { return m_value; }
	long            GetOptionCount          () const;
	wxString        GetOption               (long i) const;
	bool            IsOptionChecked         (long i) const;
	long            IsOptionCheckable       (long i) const;
	bool            OnOption                (wxWindow* parent, long i);
	bool            OnDoubleClick           (wxWindow* parent) { return OnOption(parent, GetOptionCount()-3); }
	bool            OnDefault               (wxWindow* parent) { return OnOption(parent, 0); }
	void            OnApply                 ();

	static const wxChar* s_explorePrograms[];

private:
	wxArrayString   m_options;
	wxString        m_value;
	bool            IsPredefined            (const wxString& option) const;
	void            AddOption               (const wxString& option);
};


WX_DECLARE_OBJARRAY(SjLittleOption, SjArrayLittleOption);


#endif // __SJ_LITTLEOPTION_H__


