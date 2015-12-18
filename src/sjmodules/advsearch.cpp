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
 * File:    advsearch.cpp
 * Authors: Björn Petersen
 * Purpose: Searching albums and tracks
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/console.h>
#include <sjtools/msgbox.h>
#include <sjmodules/advsearch.h>
#include <wx/calctrl.h>
#include <wx/spinctrl.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayRuleControls);
WX_DEFINE_OBJARRAY(SjArraySearchHistory);

#define MIN_TEXT_H              48  // the height of a multi-line text control (used for SQL-expressions only)
#define BUTTON_W                20
#define OP_W                    100
#define MIN_TEXT_W1             64
#define MIN_TEXT_W2             32
#define MIN_TEXT_LIMITCTRL_W    40
#define MIN_SELECTBY_W          64


/*******************************************************************************
 * SjDateDialog
 ******************************************************************************/


class SjDateDialog : public SjDialog
{
public:
	SjDateDialog        (wxWindow* parent);

	void            SetDate             (const wxString&);
	void            SetDate             (const wxDateTime&, bool setTime);

	wxString        GetDate             ();
	void            GetDate             (wxDateTime&, bool& timeSet);

private:
	wxCalendarCtrl* m_calCtrl;

	wxCheckBox*     m_timeCheck;
	wxSpinCtrl*     m_hourSpinCtrl;
	wxSpinCtrl*     m_minuteSpinCtrl;
	wxSpinCtrl*     m_secondSpinCtrl;

	void            UpdateSpinCtrls     (bool enable, int hour, int minute, int second);
	void            UpdateSpinCtrl      (wxSpinCtrl*, bool enable, int value);
	void            UpdateMonth         ();

	void            OnCalDoubleClicked  (wxCalendarEvent&)
	{
		EndModal(wxID_OK);
	}
	void            OnCalMonthChange    (wxCalendarEvent&) { UpdateMonth(); }
	void            OnTimeCheck         (wxCommandEvent&) { UpdateSpinCtrls(m_timeCheck->IsChecked(), -1, -1, -1); }
	void            OnHourSpinCtrl      (wxSpinEvent& e) { UpdateSpinCtrls(TRUE, e.GetPosition(), -1, -1); }
	void            OnMinuteSpinCtrl    (wxSpinEvent& e) { UpdateSpinCtrls(TRUE, -1, e.GetPosition(), -1); }
	void            OnSecondSpinCtrl    (wxSpinEvent& e) { UpdateSpinCtrls(TRUE, -1, -1, e.GetPosition()); }
	DECLARE_EVENT_TABLE ()
};


#define IDC_CALCTRL     (IDM_FIRSTPRIVATE+0)
#define IDC_TIMECHECK   (IDM_FIRSTPRIVATE+1)
#define IDC_HOURCTRL    (IDM_FIRSTPRIVATE+2)
#define IDC_MINUTECTRL  (IDM_FIRSTPRIVATE+3)
#define IDC_SECONDCTRL  (IDM_FIRSTPRIVATE+4)


BEGIN_EVENT_TABLE(SjDateDialog, SjDialog)
	EVT_CALENDAR        (IDC_CALCTRL,       SjDateDialog::OnCalDoubleClicked    )
	EVT_CALENDAR_MONTH  (IDC_CALCTRL,       SjDateDialog::OnCalMonthChange      )
	EVT_CALENDAR_YEAR   (IDC_CALCTRL,       SjDateDialog::OnCalMonthChange      )
	EVT_CHECKBOX        (IDC_TIMECHECK,     SjDateDialog::OnTimeCheck           )
	EVT_SPINCTRL        (IDC_HOURCTRL,      SjDateDialog::OnHourSpinCtrl        )
	EVT_SPINCTRL        (IDC_MINUTECTRL,    SjDateDialog::OnMinuteSpinCtrl      )
	EVT_SPINCTRL        (IDC_SECONDCTRL,    SjDateDialog::OnSecondSpinCtrl      )
END_EVENT_TABLE()


SjDateDialog::SjDateDialog(wxWindow* parent)
	: SjDialog(parent, _("Select date and time"), SJ_MODAL, SJ_NEVER_RESIZEABLE)
{
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, wxT("")), wxVERTICAL);
	sizer1->Add(sizer2, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	// create calendar control (holidays default to saturdays and sundays)
	long style = wxCAL_SHOW_HOLIDAYS | wxWANTS_CHARS;
	style |= SjTools::LocaleConfigRead(wxT("__DATE_SUNDAY_FIRST__"), 1L)? wxCAL_SUNDAY_FIRST : wxCAL_MONDAY_FIRST;

	m_calCtrl = new wxCalendarCtrl(this, IDC_CALCTRL, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, style);

	m_calCtrl->SetHeaderColours(wxColour(212, 208, 200), wxColour(128, 128, 128));
	m_calCtrl->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	sizer2->Add(m_calCtrl, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	// create time controls
	wxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	m_timeCheck = new wxCheckBox(this, IDC_TIMECHECK, _("Time")+wxString(wxT(":")));
	sizer3->Add(m_timeCheck, 0/*grow*/, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

	#define TIME_W 48
	m_hourSpinCtrl = new wxSpinCtrl(this, IDC_HOURCTRL, wxT(""), wxDefaultPosition, wxSize(TIME_W, -1), wxSP_ARROW_KEYS, 0, 23, 0);
	sizer3->Add(m_hourSpinCtrl, 0/*grow*/, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE/2);

	m_minuteSpinCtrl = new wxSpinCtrl(this, IDC_MINUTECTRL, wxT(""), wxDefaultPosition, wxSize(TIME_W, -1), wxSP_ARROW_KEYS, 0, 59, 0);
	sizer3->Add(m_minuteSpinCtrl, 0/*grow*/, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE/2);

	m_secondSpinCtrl = new wxSpinCtrl(this, IDC_SECONDCTRL, wxT(""), wxDefaultPosition, wxSize(TIME_W, -1), wxSP_ARROW_KEYS, 0, 59, 0);
	sizer3->Add(m_secondSpinCtrl, 0/*grow*/, wxALIGN_CENTER_VERTICAL);

	// create default buttons to close the dialog
	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxALL, SJ_DLG_SPACE);

	sizer1->SetSizeHints(this);
	CenterOnParent();

	// init date/time, esp. needed for default values in the spin ctrls
	SetDate(wxDateTime::Now(), TRUE);
}


void SjDateDialog::SetDate(const wxString& str)
{
	wxDateTime dateTime;
	bool timeSet;
	if( SjTools::ParseDate_(str, TRUE, &dateTime, &timeSet) )
	{
		SetDate(dateTime, timeSet);
	}
	else
	{
		SetDate(wxDateTime::Today(), FALSE);
	}
}


void SjDateDialog::SetDate(const wxDateTime& dateTime, bool setTime)
{
	m_calCtrl->SetDate(dateTime);
	m_timeCheck->SetValue(setTime);
	if( setTime )
	{
		UpdateSpinCtrls(TRUE, dateTime.GetHour(), dateTime.GetMinute(), dateTime.GetSecond());
	}
	else
	{
		UpdateSpinCtrls(FALSE, -1, -1, -1);
	}
	UpdateMonth();
}


wxString SjDateDialog::GetDate()
{
	wxDateTime  dateTime;
	bool        timeSet;
	GetDate(dateTime, timeSet);

	return SjTools::FormatDate(dateTime, SJ_FORMAT_EDITABLE | (timeSet? SJ_FORMAT_ADDTIME : 0));
}


void SjDateDialog::GetDate(wxDateTime& dateTime, bool& timeSet)
{
	dateTime = m_calCtrl->GetDate();
	if( m_timeCheck->IsChecked() )
	{
		timeSet = TRUE;
		dateTime.SetHour(m_hourSpinCtrl->GetValue());
		dateTime.SetMinute(m_minuteSpinCtrl->GetValue());
		dateTime.SetSecond(m_secondSpinCtrl->GetValue());
	}
	else
	{
		timeSet = FALSE;
	}
}


void SjDateDialog::UpdateSpinCtrl(wxSpinCtrl* ctrl, bool enable, int value)
{
	ctrl->Enable(enable);
	if( value != -1 )
	{
		ctrl->SetValue(wxString::Format(wxT("%02i"), value));
	}
}
void SjDateDialog::UpdateSpinCtrls(bool enable, int hour, int minute, int second)
{
	UpdateSpinCtrl(m_hourSpinCtrl, enable, hour);
	UpdateSpinCtrl(m_minuteSpinCtrl, enable, minute);
	UpdateSpinCtrl(m_secondSpinCtrl, enable, second);
}


void SjDateDialog::UpdateMonth()
{
	wxDateTime          date = m_calCtrl->GetDate();
	int                 d, dCount = wxDateTime::GetNumberOfDays(date.GetMonth(), date.GetYear());
	bool                isWorkDay;
	wxCalendarDateAttr* attr;
	wxColour            colourWhite = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	wxColour            colourBlack = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	wxColour            colourRed(250, 0, 0);
	wxColour            colourGray(140, 140, 140);
	wxColour            colourGrayRed(255, 128, 128);
	wxColour            colourYellow(0xFF, 0xFF, 0xA0);

	for( d = 1; d <= dCount; d++ )
	{
		// change date
		date.SetDay(d);
		isWorkDay = date.IsWorkDay();

		// create attributes
		attr = new wxCalendarDateAttr();

		if( date == wxDateTime::Today() )
		{
			attr->SetBackgroundColour(colourYellow);
		}
		else
		{
			attr->SetBackgroundColour(colourWhite);
		}

		if( date.IsLaterThan(wxDateTime::Today()) )
		{
			attr->SetTextColour(isWorkDay? colourGray : colourGrayRed);
		}
		else if( !isWorkDay )
		{
			attr->SetTextColour(colourRed);
		}
		else
		{
			attr->SetTextColour(colourBlack);
		}

		// set attributes
		m_calCtrl->SetAttr(d, attr);
	}
}


/*******************************************************************************
 * SjRuleControls - some definitions
 ******************************************************************************/


// IDs
#define MAX_RULES 32

#define IDC_SEARCHLIST          (IDM_FIRSTPRIVATE+0)
#define IDC_UPDATESEARCHLIST    (IDM_FIRSTPRIVATE+1)
#define IDC_REMOVERULEDO        (IDM_FIRSTPRIVATE+2)
#define IDC_RELOADRULEDO        (IDM_FIRSTPRIVATE+3)
#define IDC_SELECTCHOICE        (IDM_FIRSTPRIVATE+5)
#define IDC_SELECTOPCHOICE      (IDM_FIRSTPRIVATE+6)
#define IDC_SEARCHSELCHANGEDO   (IDM_FIRSTPRIVATE+7)
#define IDC_SEARCHINMAINCHANGED (IDM_FIRSTPRIVATE+8)
#define IDC_OPENPLAYLISTDO      (IDM_FIRSTPRIVATE+9)
#define IDC_LAST__MISCID        (IDM_FIRSTPRIVATE+10) // do not use larger IDs than this one!

#define IDC_FIRST_FIELDID       (IDC_LAST__MISCID       +1)
#define IDC_LAST__FIELDID       (IDC_FIRST_FIELDID      +MAX_RULES-1)

#define IDC_FIRST_OPID          (IDC_LAST__FIELDID      +1)
#define IDC_LAST__OPID          (IDC_FIRST_OPID         +MAX_RULES-1)

#define IDC_FIRST_TEXT0ID       (IDC_LAST__OPID         +1)
#define IDC_LAST__TEXT0ID       (IDC_FIRST_TEXT0ID      +MAX_RULES-1)

#define IDC_FIRST_TEXT1ID       (IDC_LAST__TEXT0ID      +1)
#define IDC_LAST__TEXT1ID       (IDC_FIRST_TEXT1ID      +MAX_RULES-1)

#define IDC_FIRST_CHOICE0ID     (IDC_LAST__TEXT1ID      +1)
#define IDC_LAST__CHOICE0ID     (IDC_FIRST_CHOICE0ID    +MAX_RULES-1)

#define IDC_FIRST_CHOICE1ID     (IDC_LAST__CHOICE0ID    +1)
#define IDC_LAST__CHOICE1ID     (IDC_FIRST_CHOICE1ID    +MAX_RULES-1)

#define IDC_FIRST_REMOVEID      (IDC_LAST__CHOICE1ID    +1)
#define IDC_LAST__REMOVEID      (IDC_FIRST_REMOVEID     +MAX_RULES-1)

#define IDC_FIRST_ADDID         (IDC_LAST__REMOVEID     +1)
#define IDC_LAST__ADDID         (IDC_FIRST_ADDID        +MAX_RULES-1)

// misc
SjAdvSearchModule* g_advSearchModule = NULL;

#define RULE_SPACE ((SJ_DLG_SPACE+1)/2)


/*******************************************************************************
 *  SjRuleControls
 ******************************************************************************/


// which controls are needed?
#define INPUTTYPE_OP                        0x00000001
#define INPUTTYPE_VALUE_1                   0x00000002
#define INPUTTYPE_VALUE_2                   0x00000004
#define INPUTTYPE_UNITS                     0x00000008

// more special control information
#define INPUTTYPE_VALUE_MULTILINE           0x00000010
#define INPUTTYPE_VALUE_CHOICE              0x00000020
#define INPUTTYPE_VALUE_COMBOBOX            0x00000040
#define INPUTTYPE_UNITS_CHOICE              0x00000080
#define INPUTTYPE_UNITS_STATIC              0x00000100

// very special control information
#define INPUTTYPE_VALUE_CHOICE_RATING       0x00001000
#define INPUTTYPE_VALUE_CHOICE_LIMIT        0x00002000
#define INPUTTYPE_VALUE_COMBOBOX_INCLUDE    0x00004000
#define INPUTTYPE_VALUE_COMBOBOX_EXCLUDE    0x00008000
#define INPUTTYPE_VALUE_COMBOBOX_GENRE      0x00010000
#define INPUTTYPE_VALUE_COMBOBOX_GROUP      0x00020000
#define INPUTTYPE_VALUE_COMBOBOX_DATE       0x00040000
#define INPUTTYPE_VALUE_COMBOBOX_FILETYPE   0x00080000
#define INPUTTYPE_VALUE_COMBOBOX_QUEUEPOS   0x00100000
#define INPUTTYPE_UNITS_CHOICE_RELDATE      0x00200000
#define INPUTTYPE_UNITS_CHOICE_FILESIZE     0x00400000
#define INPUTTYPE_UNITS_CHOICE_PLAYTIME     0x00800000
#define INPUTTYPE_UNITS_CHOICE_LIMIT        0x01000000
#define INPUTTYPE_UNITS_STATIC_HZ           0x02000000
#define INPUTTYPE_UNITS_STATIC_KBPS         0x04000000


long SjRuleControls::GetRuleInputType()
{
	long ret = INPUTTYPE_OP;

	switch( m_rule.m_field )
	{
		case SJ_PSEUDOFIELD_SQL:
			// this field is very special, no further stuff
			return INPUTTYPE_VALUE_1 | INPUTTYPE_VALUE_MULTILINE;

		case SJ_PSEUDOFIELD_LIMIT:
			// this field is very special, no further stuff
			return  INPUTTYPE_VALUE_CHOICE | INPUTTYPE_VALUE_CHOICE_LIMIT |
			        INPUTTYPE_UNITS | INPUTTYPE_UNITS_CHOICE | INPUTTYPE_UNITS_CHOICE_LIMIT;

		case SJ_FIELD_BITRATE:
			ret |= INPUTTYPE_UNITS | INPUTTYPE_UNITS_STATIC | INPUTTYPE_UNITS_STATIC_KBPS;
			break;

		case SJ_FIELD_SAMPLERATE:
			ret |= INPUTTYPE_UNITS | INPUTTYPE_UNITS_STATIC | INPUTTYPE_UNITS_STATIC_HZ;
			break;

		case SJ_FIELD_PLAYTIME:
			ret |= INPUTTYPE_UNITS | INPUTTYPE_UNITS_CHOICE | INPUTTYPE_UNITS_CHOICE_PLAYTIME;
			break;

		case SJ_FIELD_DATABYTES:
			ret |= INPUTTYPE_UNITS | INPUTTYPE_UNITS_CHOICE | INPUTTYPE_UNITS_CHOICE_FILESIZE;
			break;

		case SJ_FIELD_RATING:
			ret |= INPUTTYPE_VALUE_CHOICE | INPUTTYPE_VALUE_CHOICE_RATING;
			break;

		case SJ_FIELD_GENRENAME:
			ret |= INPUTTYPE_VALUE_COMBOBOX | INPUTTYPE_VALUE_COMBOBOX_GENRE;
			break;

		case SJ_FIELD_GROUPNAME:
			ret |= INPUTTYPE_VALUE_COMBOBOX | INPUTTYPE_VALUE_COMBOBOX_GROUP;
			break;

		case SJ_PSEUDOFIELD_FILETYPE:
			ret |= INPUTTYPE_VALUE_COMBOBOX | INPUTTYPE_VALUE_COMBOBOX_FILETYPE;
			break;

		case SJ_PSEUDOFIELD_QUEUEPOS:
			ret |= INPUTTYPE_VALUE_COMBOBOX | INPUTTYPE_VALUE_COMBOBOX_QUEUEPOS;
			break;

		case SJ_PSEUDOFIELD_INCLUDE:
			return INPUTTYPE_VALUE_1 | INPUTTYPE_VALUE_COMBOBOX | INPUTTYPE_VALUE_COMBOBOX_INCLUDE;
			break;

		case SJ_PSEUDOFIELD_EXCLUDE:
			return INPUTTYPE_VALUE_1 | INPUTTYPE_VALUE_COMBOBOX | INPUTTYPE_VALUE_COMBOBOX_EXCLUDE;
			break;

		default:
			break;
	}

	switch( SjRule::GetFieldType(m_rule.m_field) )
	{
		case SJ_FIELDTYPE_DATE:
			if( m_rule.m_op != SJ_FIELDOP_IS_SET && m_rule.m_op != SJ_FIELDOP_IS_UNSET
			        && m_rule.m_op != SJ_FIELDOP_IS_IN_THE_LAST && m_rule.m_op != SJ_FIELDOP_IS_NOT_IN_THE_LAST )
			{
				ret |= INPUTTYPE_VALUE_COMBOBOX | INPUTTYPE_VALUE_COMBOBOX_DATE;
			}
			break;

		default:
			break;
	}

	switch( m_rule.m_op )
	{
		case SJ_FIELDOP_IS_SET:
		case SJ_FIELDOP_IS_UNSET:
			ret &= ~(INPUTTYPE_UNITS_STATIC | INPUTTYPE_UNITS_CHOICE); // INPUTTYPE_UNITS_CHOICE added for version 2.72, see http://www.silverjuke.net/forum/topic-2852.html
			break;

		case SJ_FIELDOP_IS_IN_RANGE:
		case SJ_FIELDOP_IS_NOT_IN_RANGE:
			ret |= INPUTTYPE_VALUE_1 | INPUTTYPE_VALUE_2;
			break;

		case SJ_FIELDOP_IS_IN_THE_LAST:
		case SJ_FIELDOP_IS_NOT_IN_THE_LAST:
			ret |= INPUTTYPE_VALUE_1 | INPUTTYPE_UNITS | INPUTTYPE_UNITS_CHOICE | INPUTTYPE_UNITS_CHOICE_RELDATE;
			break;

		default:
			ret |= INPUTTYPE_VALUE_1;
			break;
	}

	return ret;
}


void SjRuleControls::InitFieldChoice(SjField field)
{
	#define __SJ_FIELD_SEP__ SJ_FIELD_COUNT
	if( field == __SJ_FIELD_SEP__ )
	{
		m_fieldChoice->Append(SJ_CHOICE_SEP, (void*)__SJ_FIELD_SEP__);
	}
	else
	{
		m_fieldChoice->Append(SjRule::GetFieldDescr(field), (void*)field);
	}
}
void SjRuleControls::InitFieldChoice()
{
	InitFieldChoice(SJ_PSEUDOFIELD_TRACKARTISTALBUM );
	InitFieldChoice(SJ_FIELD_TRACKNAME              );
	InitFieldChoice(SJ_FIELD_LEADARTISTNAME         );
	InitFieldChoice(SJ_FIELD_ORGARTISTNAME          );
	InitFieldChoice(SJ_FIELD_COMPOSERNAME           );
	InitFieldChoice(SJ_FIELD_ALBUMNAME              );
	InitFieldChoice(SJ_FIELD_COMMENT                );

	InitFieldChoice(__SJ_FIELD_SEP__);

	InitFieldChoice(SJ_FIELD_TRACKNR                );
	InitFieldChoice(SJ_FIELD_TRACKCOUNT             );
	InitFieldChoice(SJ_FIELD_DISKNR                 );
	InitFieldChoice(SJ_FIELD_DISKCOUNT              );
	InitFieldChoice(SJ_FIELD_GENRENAME              );
	InitFieldChoice(SJ_FIELD_GROUPNAME              );
	InitFieldChoice(SJ_FIELD_YEAR                   );
	InitFieldChoice(SJ_FIELD_BEATSPERMINUTE         );
	InitFieldChoice(SJ_FIELD_RATING                 );

	InitFieldChoice(__SJ_FIELD_SEP__);

	InitFieldChoice(SJ_FIELD_PLAYTIME               );
	InitFieldChoice(SJ_FIELD_AUTOVOL                );
	InitFieldChoice(SJ_FIELD_CHANNELS               );
	InitFieldChoice(SJ_FIELD_SAMPLERATE             );
	InitFieldChoice(SJ_FIELD_BITRATE                );

	InitFieldChoice(__SJ_FIELD_SEP__);

	InitFieldChoice(SJ_FIELD_URL                    );
	InitFieldChoice(SJ_PSEUDOFIELD_FILETYPE         );
	InitFieldChoice(SJ_FIELD_DATABYTES              );
	InitFieldChoice(SJ_FIELD_TIMEADDED              );
	InitFieldChoice(SJ_FIELD_TIMEMODIFIED           );
	InitFieldChoice(SJ_FIELD_LASTPLAYED             );
	InitFieldChoice(SJ_FIELD_TIMESPLAYED            );
	InitFieldChoice(SJ_PSEUDOFIELD_QUEUEPOS         );

	InitFieldChoice(__SJ_FIELD_SEP__);

	InitFieldChoice(SJ_PSEUDOFIELD_EXCLUDE          );
	InitFieldChoice(SJ_PSEUDOFIELD_INCLUDE          );
	InitFieldChoice(SJ_PSEUDOFIELD_SQL              );
	InitFieldChoice(SJ_PSEUDOFIELD_LIMIT            );
}


void SjRuleControls::InitOpChoice(const wxString& name, SjFieldOp op)
{
	if( SjRule::IsValidFieldOp(m_rule.m_field, op) )
	{
		m_opChoice->Append(name, (void*)op);
	}
}
void SjRuleControls::InitOpChoice()
{
	bool isDate = (SjRule::GetFieldType(m_rule.m_field) == SJ_FIELDTYPE_DATE);

	InitOpChoice(                                     _("contains"),            SJ_FIELDOP_CONTAINS             );
	InitOpChoice(                                     _("does not contain"),    SJ_FIELDOP_DOES_NOT_CONTAIN     );
	InitOpChoice(                                     _("starts with"),         SJ_FIELDOP_STARTS_WITH          );
	InitOpChoice(                                     _("does not start with"), SJ_FIELDOP_DOES_NOT_START_WITH  );
	InitOpChoice(                                     _("starts simelar to"),   SJ_FIELDOP_STARTS_SIMELAR_TO    );
	InitOpChoice(                                     _("ends with"),           SJ_FIELDOP_ENDS_WITH            );
	InitOpChoice(                                     _("does not end with"),   SJ_FIELDOP_DOES_NOT_END_WITH    );
	InitOpChoice(isDate? _("on")                    : _("is equal to"),         SJ_FIELDOP_IS_EQUAL_TO          );
	InitOpChoice(isDate? _("not on")                : _("is unequal to"),       SJ_FIELDOP_IS_UNEQUAL_TO        );
	InitOpChoice(                                     _("is simelar to"),       SJ_FIELDOP_IS_SIMELAR_TO        );
	InitOpChoice(isDate? _("is after")              : _("is greater than"),     SJ_FIELDOP_IS_GREATER_THAN      );
	InitOpChoice(isDate? _("is on or after")        : _("is greater or equal"), SJ_FIELDOP_IS_GREATER_OR_EQUAL  );
	InitOpChoice(isDate? _("is before")             : _("is less than"),        SJ_FIELDOP_IS_LESS_THAN         );
	InitOpChoice(isDate? _("is on or before")       : _("is less or equal"),    SJ_FIELDOP_IS_LESS_OR_EQUAL     );
	InitOpChoice(        _("is in the last"),                                   SJ_FIELDOP_IS_IN_THE_LAST       );
	InitOpChoice(        _("is not in the last"),                               SJ_FIELDOP_IS_NOT_IN_THE_LAST   );
	InitOpChoice(isDate? _("is in the period")      : _("is in range"),         SJ_FIELDOP_IS_IN_RANGE          );
	InitOpChoice(isDate? _("is not in the period")  : _("is not in range"),     SJ_FIELDOP_IS_NOT_IN_RANGE      );
	InitOpChoice(                                     _("is set"),              SJ_FIELDOP_IS_SET               );
	InitOpChoice(                                     _("is unset"),            SJ_FIELDOP_IS_UNSET             );

	if( !SjDialog::SetCbSelection(m_opChoice, m_rule.m_op) )
	{
		// this should normally not happen, as rules are converted correctly (?)
		// using SjRule::Convert() which should set the correct operator.
		// however, never trust incoming data...
		wxASSERT( 0 );
		m_rule.m_op = SJ_FIELDOP_IS_EQUAL_TO;
		SjDialog::SetCbSelection(m_opChoice, m_rule.m_op);
	}
}


void SjRuleControls::InitUnitChoice()
{
	SjUnit defUnit = SJ_UNIT_TRACKS;

	if( m_inputType & INPUTTYPE_UNITS_CHOICE_LIMIT )
	{
		m_choice[0]->Append(_("tracks"),    (void*)SJ_UNIT_TRACKS   );
		m_choice[0]->Append(_("albums"),    (void*)SJ_UNIT_ALBUMS   );
		m_choice[0]->Append(_("minutes"),   (void*)SJ_UNIT_MINUTES  );
		m_choice[0]->Append(_("hours"),     (void*)SJ_UNIT_HOURS    );
		m_choice[0]->Append(wxT("MB"),      (void*)SJ_UNIT_MB       );
		m_choice[0]->Append(wxT("GB"),      (void*)SJ_UNIT_GB       );
		defUnit = SJ_UNIT_TRACKS;
	}
	else if( m_inputType & INPUTTYPE_UNITS_CHOICE_PLAYTIME )
	{
		m_choice[0]->Append(_("seconds"),   (void*)SJ_UNIT_SECONDS  );
		m_choice[0]->Append(_("minutes"),   (void*)SJ_UNIT_MINUTES  );
		defUnit = SJ_UNIT_MINUTES;
	}
	else if( m_inputType & INPUTTYPE_UNITS_CHOICE_RELDATE )
	{
		m_choice[0]->Append(_("minutes"),   (void*)SJ_UNIT_MINUTES  );
		m_choice[0]->Append(_("hours"),     (void*)SJ_UNIT_HOURS    );
		m_choice[0]->Append(_("days"),      (void*)SJ_UNIT_DAYS     );
		defUnit = SJ_UNIT_DAYS;
	}
	else if( m_inputType & INPUTTYPE_UNITS_CHOICE_FILESIZE )
	{
		m_choice[0]->Append(wxT("Byte"),    (void*)SJ_UNIT_BYTE     );
		m_choice[0]->Append(wxT("KB"),      (void*)SJ_UNIT_KB       );
		m_choice[0]->Append(wxT("MB"),      (void*)SJ_UNIT_MB       );
		defUnit = SJ_UNIT_MB;
	}

	if( !SjDialog::SetCbSelection(m_choice[0], m_rule.m_unit) )
	{
		m_rule.m_unit = defUnit;
		SjDialog::SetCbSelection(m_choice[0], m_rule.m_unit);
	}
}


void SjRuleControls::UpdateFieldTooltips()
{
	int         i;
	wxWindow*   window;
	wxString    error[2];

	m_rule.IsOk(error[0], error[1]);

	for( i = 0; i < 2; i++ )
	{
		window = m_valueTextCtrl[i]? (wxWindow*)m_valueTextCtrl[i] : (wxWindow*)m_valueComboBox[i];
		if( window )
		{
			window->SetToolTip(error[i]);
		}
	}
}


wxTextCtrl* SjRuleControls::CreateValueTextCtrl(long baseId, const wxString& value)
{
	wxTextCtrl* textCtrl;

	textCtrl = new wxTextCtrl(m_dialog, baseId+m_ruleId, value,
	                          wxDefaultPosition,
	                          wxSize((m_inputType&INPUTTYPE_VALUE_2)? MIN_TEXT_W2 : MIN_TEXT_W1,
	                                 (m_inputType&INPUTTYPE_VALUE_MULTILINE)? MIN_TEXT_H : -1),
	                          (m_inputType&INPUTTYPE_VALUE_MULTILINE)? wxTE_MULTILINE : 0);

	return textCtrl;
}


wxChoice* SjRuleControls::CreateValueChoice(long baseId, wxString& value)
{
	long longValue = 0, defaultValue = 0;
	if( !value.ToLong(&longValue, 10) ) { longValue = 0; }
	wxChoice* c;

	c = new wxChoice(m_dialog, baseId+m_ruleId, wxDefaultPosition, wxSize(MIN_TEXT_W1, -1));

	if( m_inputType & INPUTTYPE_VALUE_CHOICE_RATING )
	{
		c->Append(SJ_RATING_CHARS_DLG,                                                                                  (void*)1);
		c->Append(SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG,                                                              (void*)2);
		c->Append(SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG,                                          (void*)3);
		c->Append(SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG,                      (void*)4);
		c->Append(SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG,  (void*)5);
		c->Append(_("No rating"),                                                                                       (void*)0);
		defaultValue = 1;
	}
	else if( m_inputType & INPUTTYPE_VALUE_CHOICE_LIMIT )
	{
		c->Append(_("random"),                  (void*)(SJ_PSEUDOFIELD_RANDOM                       ));
		c->Append(_("highest rating"),          (void*)(SJ_FIELD_RATING         | SJ_FIELDFLAG_DESC ));
		c->Append(_("lowest rating"),           (void*)(SJ_FIELD_RATING                             ));
		c->Append(_("most often played"),       (void*)(SJ_FIELD_TIMESPLAYED    | SJ_FIELDFLAG_DESC ));
		c->Append(_("least often played"),      (void*)(SJ_FIELD_TIMESPLAYED                        ));
		c->Append(_("most recently played"),    (void*)(SJ_FIELD_LASTPLAYED     | SJ_FIELDFLAG_DESC ));
		c->Append(_("least recently played"),   (void*)(SJ_FIELD_LASTPLAYED                         ));
		c->Append(_("most recently added"),     (void*)(SJ_FIELD_TIMEADDED      | SJ_FIELDFLAG_DESC ));
		c->Append(_("least recently added"),    (void*)(SJ_FIELD_TIMEADDED                          ));
		c->Append(_("most recently modified"),  (void*)(SJ_FIELD_TIMEMODIFIED   | SJ_FIELDFLAG_DESC ));
		c->Append(_("least recently modified"), (void*)(SJ_FIELD_TIMEMODIFIED                       ));
		c->Append(_("most recently released"),  (void*)(SJ_FIELD_YEAR           | SJ_FIELDFLAG_DESC ));
		c->Append(_("least recently released"), (void*)(SJ_FIELD_YEAR                               ));
		c->Append(_("low queue position"),      (void*)(SJ_PSEUDOFIELD_QUEUEPOS                     ));
		c->Append(_("high queue position"),     (void*)(SJ_PSEUDOFIELD_QUEUEPOS | SJ_FIELDFLAG_DESC ));
		defaultValue = SJ_FIELD_TIMESPLAYED | SJ_FIELDFLAG_DESC;
	}

	if( !SjDialog::SetCbSelection(c, longValue) )
	{
		longValue = defaultValue;
		SjDialog::SetCbSelection(c, longValue);
	}

	value = wxString::Format(wxT("%i"), (int)longValue);

	return c;
}


wxComboBox* SjRuleControls::CreateValueComboBox(long baseId, const wxString& value__)
{
	#define     CB_SPECIAL_TODAY            _("Today")
	#define     CB_SPECIAL_YESTERDAY        _("Yesterday")
	#define     CB_SPECIAL_SELECT_DATE      _("Select...")
	#define     CB_SPECIAL_SHOW_TRACKS      _("Show tracks...")
	#define     CB_SPECIAL_OPEN_PLAYLIST    (_("Add tracks from playlist")+wxString(wxT("...")))
	#define     CB_SPECIAL_CURR_POS         _("Current position")

	wxString    value(value__);
	wxComboBox* c;

	c = new wxComboBox(m_dialog, baseId+m_ruleId, wxT(""), wxDefaultPosition, wxSize(MIN_TEXT_W1, -1));

	if( m_inputType & (INPUTTYPE_VALUE_COMBOBOX_GENRE|INPUTTYPE_VALUE_COMBOBOX_GROUP) )
	{
		wxArrayString data = g_mainFrame->m_libraryModule->GetUniqueValues((m_inputType & INPUTTYPE_VALUE_COMBOBOX_GENRE)? SJ_TI_GENRENAME : SJ_TI_GROUPNAME);
		int i, iCount = (int)data.GetCount();
		for( i =0; i < iCount; i++ )
		{
			c->Append(data[i]);
		}
	}
	else if( m_inputType & INPUTTYPE_VALUE_COMBOBOX_FILETYPE )
	{
		SjExtList ext = g_mainFrame->m_moduleSystem.GetAssignedExt(SJ_EXT_MUSICFILES);
		int i, iCount = ext.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			c->Append(ext.GetExt(i));
		}
	}
	else if( m_inputType & INPUTTYPE_VALUE_COMBOBOX_DATE )
	{
		value.Replace(wxT("today"), CB_SPECIAL_TODAY);
		value.Replace(wxT("yesterday"), CB_SPECIAL_YESTERDAY);

		c->Append(CB_SPECIAL_TODAY);
		c->Append(CB_SPECIAL_YESTERDAY);
		c->Append(CB_SPECIAL_SELECT_DATE);
	}
	else if( m_inputType & INPUTTYPE_VALUE_COMBOBOX_QUEUEPOS )
	{
		value.Replace(SJ_QUEUEPOS_CURR_STR, CB_SPECIAL_CURR_POS);

		c->Append(CB_SPECIAL_CURR_POS);
	}
	else if( m_inputType & (INPUTTYPE_VALUE_COMBOBOX_INCLUDE|INPUTTYPE_VALUE_COMBOBOX_EXCLUDE) )
	{
		value = m_rule.GetInclExclDescr();

		c->Append(CB_SPECIAL_SHOW_TRACKS);
		c->Append(CB_SPECIAL_OPEN_PLAYLIST);
	}

	c->SetValue(value);

	return c;
}


SjRuleControls::SjRuleControls(const SjRule& rule__, SjAdvSearchDialog* dialog, wxFlexGridSizer* ruleSizer, long ruleId,
                               bool setFocus, long focusRepostValue)
{
	m_rule = rule__;
	m_inputType         = GetRuleInputType();
	m_ruleId            = ruleId;

	m_dialog            = dialog;
	m_ruleSizer         = ruleSizer;
	for( int i = 0; i < 2; i++ )
	{
		m_valueTextCtrl[i]  = NULL;
		m_choice[i]         = NULL;
		m_valueComboBox[i]  = NULL;
		m_staticText[i]     = NULL;
	}
	m_opChoice          = NULL;
	int align           = (m_inputType&INPUTTYPE_VALUE_MULTILINE)? wxALIGN_TOP : wxALIGN_CENTER_VERTICAL;
	m_inValueChange     = 1;


	//
	// create field choice
	//

	m_fieldChoice = new wxChoice(dialog, IDC_FIRST_FIELDID+ruleId);
	m_normalColour = m_fieldChoice->GetBackgroundColour();
	InitFieldChoice();
	SjDialog::SetCbWidth(m_fieldChoice);
	SjDialog::SetCbSelection(m_fieldChoice, m_rule.m_field);
	ruleSizer->Add(m_fieldChoice, 0, align);

	//
	// create sizer
	//

	m_sizer1 = new wxBoxSizer(wxHORIZONTAL);
	ruleSizer->Add(m_sizer1, 0, wxGROW|align);

	//
	// create operation choice (should have a fixed width for layout reasons)
	//

	if( m_inputType & INPUTTYPE_OP )
	{
		m_opChoice = new wxChoice(dialog, IDC_FIRST_OPID+ruleId, wxDefaultPosition, wxSize(OP_W, -1));
		InitOpChoice();
		m_sizer1->Add(m_opChoice, 0, align);
	}
	else
	{
		m_rule.m_op = SJ_FIELDOP_IS_EQUAL_TO;
	}

	if( m_inputType & INPUTTYPE_UNITS_CHOICE_LIMIT )
	{
		//
		// create LIMIT controls
		//

		m_valueTextCtrl[0] = new wxTextCtrl(dialog, IDC_FIRST_TEXT0ID+ruleId, m_rule.m_value[0],
		                                    wxDefaultPosition, wxSize(MIN_TEXT_LIMITCTRL_W, -1));
		m_sizer1->Add(m_valueTextCtrl[0], 0, align);

		m_choice[0] = new wxChoice(dialog, IDC_FIRST_CHOICE0ID+ruleId);
		InitUnitChoice();
		SjDialog::SetCbWidth(m_choice[0], MIN_SELECTBY_W);
		m_sizer1->Add(m_choice[0], 0, align|wxLEFT, RULE_SPACE);

		m_staticText[0] = new wxStaticText(dialog, -1, _("by"));
		m_sizer1->Add(m_staticText[0], 0, align|wxLEFT, RULE_SPACE);

		// we're using the choice2 control for our "select by" stuff which is found in "value2"
		m_choice[1] = CreateValueChoice(IDC_FIRST_CHOICE1ID, m_rule.m_value[1]);
		SjDialog::SetCbWidth(m_choice[1], 80);
		m_sizer1->Add(m_choice[1], 1/*grow*/, align|wxLEFT, RULE_SPACE);
	}
	else
	{
		//
		// create CRITERIA controls
		//

		// create 1st value control
		if( m_inputType & INPUTTYPE_VALUE_1 )
		{
			wxWindow* window;
			if( m_inputType & INPUTTYPE_VALUE_CHOICE )
			{
				m_choice[0] = CreateValueChoice(IDC_FIRST_CHOICE0ID, m_rule.m_value[0]);
				window = m_choice[0];
			}
			else if( m_inputType & INPUTTYPE_VALUE_COMBOBOX )
			{
				m_valueComboBox[0] = CreateValueComboBox(IDC_FIRST_TEXT0ID, m_rule.m_value[0]);
				window = m_valueComboBox[0];
			}
			else
			{
				m_valueTextCtrl[0] = CreateValueTextCtrl(IDC_FIRST_TEXT0ID, m_rule.m_value[0]);
				window = m_valueTextCtrl[0];
			}
			m_sizer1->Add(window, 1/*grow*/, align|((m_inputType & INPUTTYPE_OP)? wxLEFT : 0), RULE_SPACE);
		}
		else
		{
			m_sizer1->Add(RULE_SPACE, RULE_SPACE, 1/*grow*/);
			m_rule.m_value[0] = wxT("");
		}

		// create 2nd value control
		if( m_inputType & INPUTTYPE_VALUE_2 )
		{
			m_staticText[0] = new wxStaticText(dialog, -1, _("to"));
			m_sizer1->Add(m_staticText[0], 0, align|wxLEFT, RULE_SPACE);

			wxWindow* window;
			if( m_inputType & INPUTTYPE_VALUE_CHOICE )
			{
				m_choice[1] = CreateValueChoice(IDC_FIRST_CHOICE1ID, m_rule.m_value[1]);
				window = m_choice[1];
			}
			else if( m_inputType & INPUTTYPE_VALUE_COMBOBOX )
			{
				m_valueComboBox[1] = CreateValueComboBox(IDC_FIRST_TEXT1ID, m_rule.m_value[1]);
				window = m_valueComboBox[1];
			}
			else
			{
				m_valueTextCtrl[1] = CreateValueTextCtrl(IDC_FIRST_TEXT1ID, m_rule.m_value[1]);
				window = m_valueTextCtrl[1];
			}
			m_sizer1->Add(window, 1/*grow*/, align|wxLEFT, RULE_SPACE);
		}
		else
		{
			m_rule.m_value[1] = wxT("");
		}

		// static unit
		if( m_inputType & INPUTTYPE_UNITS_STATIC )
		{
			wxASSERT( m_staticText[1] == NULL );
			wxString unit;
			if( m_inputType & INPUTTYPE_UNITS_STATIC_HZ )
			{
				// TRANSLATORS: Abbreviation of "Hertz"
				unit = _("Hz");
			}
			else
			{
				// TRANSLATORS: Abbreviation of "Bits per second"
				unit = _("bit/s");
			}
			m_staticText[1] = new wxStaticText(dialog, -1, unit);
			m_sizer1->Add(m_staticText[1], 0, align|wxLEFT, RULE_SPACE);
		}

		// unit choice...
		// (currently, the control pointer is shared with possible value choices,
		// so these cannot be mixed)
		if( m_inputType & INPUTTYPE_UNITS_CHOICE )
		{
			// ...unit choice
			wxASSERT( m_choice[0] == NULL );
			m_choice[0] = new wxChoice(dialog, IDC_FIRST_CHOICE0ID+ruleId);
			InitUnitChoice();
			SjDialog::SetCbWidth(m_choice[0]);
			m_sizer1->Add(m_choice[0], 0, wxLEFT|align, RULE_SPACE);
		}
		else
		{
			m_rule.m_unit = SJ_UNIT_NA;
		}
	}

	//
	// create "add" and "remove" buttons
	//

	m_addRemoveButton[0] = new wxButton(dialog, IDC_FIRST_REMOVEID+ruleId, wxT("-"), wxDefaultPosition, wxSize(BUTTON_W, -1), wxBU_EXACTFIT);
	m_addRemoveButton[0]->SetToolTip(_("Remove criterion"));
	m_sizer1->Add(m_addRemoveButton[0], 0, align|wxLEFT, SJ_DLG_SPACE);

	m_addRemoveButton[1] = new wxButton(dialog, IDC_FIRST_ADDID+ruleId, wxT("+"), wxDefaultPosition, wxSize(BUTTON_W, -1), wxBU_EXACTFIT);
	m_addRemoveButton[1]->SetToolTip(_("Add criterion"));
	m_sizer1->Add(m_addRemoveButton[1], 0, align|wxLEFT, RULE_SPACE);

	//
	// init focus
	//

	if( setFocus )
	{
		if( focusRepostValue == 1 && m_opChoice )
		{
			m_opChoice->SetFocus();
		}
		else if( focusRepostValue == 2 && m_valueComboBox[0] )
		{
			// we don't re-set the focus to comboboxes as there is a
			// bug in wxWidgets that let the text scroll out of view :-(
			// this bug is true for wxWidgets 2.4.2
			//m_valueComboBox[0]->SetFocus();
		}
		else if( focusRepostValue == 3 && m_valueComboBox[1] )
		{
			// see above
			//m_valueComboBox[1]->SetFocus();
		}
		else
		{
			m_fieldChoice->SetFocus();
		}
	}

	// check if the fields are okay
	UpdateFieldTooltips();

	// done with constructor
	m_inValueChange--;
}


void SjRuleControls::DestroyChild(wxSizer* parentSizer, wxWindow** childToDestroy)
{
	if( *childToDestroy )
	{
		(*childToDestroy)->Hide();
		parentSizer->Detach(*childToDestroy);
		(*childToDestroy)->Destroy();
		*childToDestroy = NULL;
	}
}
void SjRuleControls::DestroySizer(wxSizer* parentSizer, wxSizer** sizerToDestroy)
{
	if( *sizerToDestroy )
	{
		parentSizer->Remove(*sizerToDestroy);
		*sizerToDestroy = NULL;
	}
}
void SjRuleControls::DestroyChildren()
{
	DestroyChild(m_ruleSizer, (wxWindow**)&m_fieldChoice);
	DestroyChild(m_ruleSizer, (wxWindow**)&m_opChoice);
	for( int i = 0; i < 2; i++ )
	{
		DestroyChild(m_ruleSizer, (wxWindow**)&m_valueTextCtrl[i]);
		DestroyChild(m_ruleSizer, (wxWindow**)&m_choice[i]);
		DestroyChild(m_ruleSizer, (wxWindow**)&m_valueComboBox[i]);
		DestroyChild(m_ruleSizer, (wxWindow**)&m_staticText[i]);
		DestroyChild(m_ruleSizer, (wxWindow**)&m_addRemoveButton[i]);
	}
	DestroySizer(m_ruleSizer, &m_sizer1);
}


void SjRuleControls::OnFieldChanged()
{
	if( m_inValueChange == 0 )
	{
		m_inValueChange++;

		SjField newField = (SjField)SjDialog::GetCbSelection(m_fieldChoice);
		if( newField == __SJ_FIELD_SEP__ )
		{
			m_dialog->ReloadRule(m_ruleId, 0);
		}
		else if( m_rule.Convert(newField)
		         || m_inputType != GetRuleInputType() )
		{
			m_dialog->ReloadRule(m_ruleId, 0);
		}
		else
		{
			UpdateFieldTooltips();
			m_dialog->RuleChanged();
		}

		m_inValueChange--;
	}
}


void SjRuleControls::OnOpChanged()
{
	if( m_rule.Convert((SjFieldOp)SjDialog::GetCbSelection(m_opChoice))
	        || m_inputType != GetRuleInputType() )
	{
		m_dialog->ReloadRule(m_ruleId, 1);
	}
	else
	{
		UpdateFieldTooltips();
		m_dialog->RuleChanged();
	}
}


void SjRuleControls::OnTextChanged(int i, WXTYPE eventType)
{
	// avoid recursion on text/combobox events
	// the static member is needed in addition to avoid recursive calls
	// when updating rules (which will have different pointers after updating)
	if( m_inValueChange == 0 )
	{
		m_inValueChange++;

		if( m_inputType & INPUTTYPE_VALUE_COMBOBOX )
		{
			// combobox selection...
			wxASSERT( m_valueComboBox[i] );
			wxString newValue = m_valueComboBox[i]->GetValue();

			if( m_inputType & INPUTTYPE_VALUE_COMBOBOX_DATE )
			{
				// ...date select
				wxString cbValue = newValue.Trim(TRUE).Trim(FALSE).Lower();
				if( cbValue == wxString(CB_SPECIAL_TODAY).Lower() )
				{
					m_rule.m_value[i] = wxT("today");
				}
				else if( cbValue == wxString(CB_SPECIAL_YESTERDAY).Lower() )
				{
					m_rule.m_value[i] = wxT("yesterday");
				}
				else if( cbValue == wxString(CB_SPECIAL_SELECT_DATE).Lower() )
				{
					if( eventType == wxEVT_COMMAND_COMBOBOX_SELECTED )
					{
						wxLogDebug(wxT("OPENING DATE DIALOG..."));

						wxWindowDisabler disabler(m_dialog);
						SjDateDialog dateDialog(m_dialog);
						dateDialog.SetDate(m_rule.m_value[i]);
						if( dateDialog.ShowModal() == wxID_OK )
						{
							m_rule.m_value[i] = dateDialog.GetDate();
						}

						m_dialog->ReloadRule(m_ruleId, i+2);
					}

					m_inValueChange--;
					return;
				}
				else
				{
					m_rule.m_value[i] = newValue;
				}
			}
			else if( m_inputType & INPUTTYPE_VALUE_COMBOBOX_QUEUEPOS )
			{
				// ...queue position select
				wxString cbValue = newValue.Trim(TRUE).Trim(FALSE).Lower();
				if( cbValue == wxString(CB_SPECIAL_CURR_POS).Lower() )
				{
					m_rule.m_value[i] = SJ_QUEUEPOS_CURR_STR;
				}
				else
				{
					m_rule.m_value[i] = newValue;
				}
			}
			else if( m_inputType & (INPUTTYPE_VALUE_COMBOBOX_INCLUDE|INPUTTYPE_VALUE_COMBOBOX_EXCLUDE) )
			{
				bool incl = (m_rule.m_field==SJ_PSEUDOFIELD_INCLUDE);

				// ...manual include / exclude combobox
				if( newValue == wxString(CB_SPECIAL_SHOW_TRACKS) && eventType == wxEVT_COMMAND_COMBOBOX_SELECTED )
				{
					// create special search
					SjAdvSearch recentSearch = g_advSearchModule->GetRecentSearch();
					SjAdvSearch specialSearch;
					specialSearch.Init(
					    wxString::Format(incl? _("Tracks included to \"%s\"") : _("Tracks excluded from \"%s\""), recentSearch.GetName().c_str()),
					    SJ_SELECTSCOPE_TRACKS,
					    SJ_SELECTOP_ALL);
					specialSearch.SetSubset(incl? SJ_SUBSET_INCLUDED_TRACKS : SJ_SUBSET_EXCLUDED_TRACKS, recentSearch.GetId());
					specialSearch.AddRule(SJ_PSEUDOFIELD_INCLUDE, SJ_FIELDOP_IS_EQUAL_TO, m_rule.m_value[0]);

					// set special search
					g_mainFrame->SetSearch(SJ_SETSEARCH_CLEARSIMPLE|SJ_SETSEARCH_SETADV, wxT(""), &specialSearch);

				}
				else if( newValue == wxString(CB_SPECIAL_OPEN_PLAYLIST) && eventType == wxEVT_COMMAND_COMBOBOX_SELECTED )
				{
					// include/exclude a playlist
					wxCommandEvent* fwd = new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_OPENPLAYLISTDO);
					fwd->SetExtraLong(incl? 1 : 0);
					m_dialog->GetEventHandler()->QueueEvent(fwd);
				}
				else if( newValue == wxT("") )
				{
					// delete
					m_rule.m_value[i] = wxT("");
				}

				m_dialog->ReloadRule(m_ruleId, i+2);
				m_inValueChange--;
				return;
			}
			else
			{
				// ...other (eg. genre) select
				m_rule.m_value[i] = newValue;
			}
		}
		else if( m_valueTextCtrl[i] )
		{
			// ...text input
			m_rule.m_value[i] = m_valueTextCtrl[i]->GetValue();
			m_rule.m_value[i].Replace(wxT("\n"), wxT(" ")); // no linebreaks!
			m_rule.m_value[i].Replace(wxT("\r"), wxT(" "));
		}

		UpdateFieldTooltips();
		m_dialog->RuleChanged();

		m_inValueChange--;
	}
	#ifdef __WXDEBUG__
	else
	{
		wxLogDebug(wxT("RECURSIVE CALL TO SjRuleControls::OnTextChanged ?!"));
	}
	#endif
}


void SjRuleControls::OnChoiceChanged(int i)
{
	if( (m_inputType & INPUTTYPE_UNITS) && i == 0 )
	{
		wxASSERT( m_choice[i] );
		m_rule.m_unit = (SjUnit)SjDialog::GetCbSelection(m_choice[i]);
	}
	else if( m_inputType & INPUTTYPE_VALUE_CHOICE )
	{
		wxASSERT( m_choice[i] );
		m_rule.m_value[i] = wxString::Format(wxT("%i"), (int)SjDialog::GetCbSelection(m_choice[i]));
	}

	UpdateFieldTooltips();
	m_dialog->RuleChanged();
}


/*******************************************************************************
 * SjAdvSearchDialog
 ******************************************************************************/


BEGIN_EVENT_TABLE(SjAdvSearchDialog, SjDialog)
	EVT_LIST_ITEM_ACTIVATED     (IDC_SEARCHLIST,        SjAdvSearchDialog::OnSearchDoubleClick      )
	EVT_LIST_ITEM_RIGHT_CLICK   (IDC_SEARCHLIST,        SjAdvSearchDialog::OnSearchContextMenu      )
	EVT_LIST_ITEM_SELECTED      (IDC_SEARCHLIST,        SjAdvSearchDialog::OnSearchSelChange        )
	EVT_LIST_KEY_DOWN           (IDC_SEARCHLIST,        SjAdvSearchDialog::OnSearchKeyDown          )
	EVT_MENU                    (IDC_SEARCHSELCHANGEDO, SjAdvSearchDialog::OnSearchSelChangeDo      )
	EVT_MENU                    (IDC_SEARCHINMAINCHANGED,
	                             SjAdvSearchDialog::OnSearchInMainChanged    )
	EVT_LIST_END_LABEL_EDIT     (IDC_SEARCHLIST,        SjAdvSearchDialog::OnSearchEndEditLabel     )
	EVT_MENU                    (IDC_UPDATESEARCHLIST,  SjAdvSearchDialog::OnUpdateSearchList       )
	EVT_BUTTON                  (IDC_BUTTONBARMENU,     SjAdvSearchDialog::OnMenuButton             )
	EVT_MENU                    (IDC_NEWSEARCH,         SjAdvSearchDialog::OnNewOrSaveAsSearch      )
	EVT_MENU                    (IDC_SAVESEARCHAS,      SjAdvSearchDialog::OnNewOrSaveAsSearch      )
	EVT_MENU                    (IDC_SAVESEARCH,        SjAdvSearchDialog::OnSaveSearch             )
	EVT_MENU                    (IDC_RENAMESEARCH,      SjAdvSearchDialog::OnRenameSearch           )
	EVT_MENU                    (IDC_DELETESEARCH,      SjAdvSearchDialog::OnDeleteSearch           )
	EVT_MENU                    (IDC_REVERTTOSAVEDSEARCH,
	                             SjAdvSearchDialog::OnRevertToSavedSearch    )
	EVT_SIZE                    (                       SjAdvSearchDialog::OnSize                   )
	EVT_BUTTON                  (wxID_OK,               SjAdvSearchDialog::OnSearch                 )
	EVT_MENU                    (wxID_OK,               SjAdvSearchDialog::OnSearch                 )
	EVT_BUTTON                  (IDC_ENDADVSEARCH,      SjAdvSearchDialog::OnEndSearch              )
	EVT_MENU                    (IDC_ENDADVSEARCH,      SjAdvSearchDialog::OnEndSearch              )
	EVT_MENU                    (IDT_ENQUEUE_NOW,       SjAdvSearchDialog::OnPlay                   )
	EVT_MENU                    (IDT_ENQUEUE_LAST,      SjAdvSearchDialog::OnPlay                   )
	EVT_BUTTON                  (wxID_CANCEL,           SjAdvSearchDialog::OnCloseByButton          )
	EVT_CLOSE                   (                       SjAdvSearchDialog::OnCloseByTitlebar        )

	EVT_CHOICE                  (IDC_SELECTCHOICE,      SjAdvSearchDialog::OnSelectChoice           )
	EVT_CHOICE                  (IDC_SELECTOPCHOICE,    SjAdvSearchDialog::OnSelectOpChoice         )

	EVT_COMMAND_RANGE           (IDC_FIRST_FIELDID, IDC_LAST__FIELDID, wxEVT_COMMAND_CHOICE_SELECTED,
	                             SjAdvSearchDialog::OnRuleFieldChanged       )
	EVT_COMMAND_RANGE           (IDC_FIRST_OPID, IDC_LAST__OPID, wxEVT_COMMAND_CHOICE_SELECTED,
	                             SjAdvSearchDialog::OnRuleOpChanged          )

	EVT_COMMAND_RANGE           (IDC_FIRST_TEXT0ID, IDC_LAST__TEXT0ID, wxEVT_COMMAND_TEXT_UPDATED,
	                             SjAdvSearchDialog::OnRuleText0Changed       )
	EVT_COMMAND_RANGE           (IDC_FIRST_TEXT1ID, IDC_LAST__TEXT1ID, wxEVT_COMMAND_TEXT_UPDATED,
	                             SjAdvSearchDialog::OnRuleText1Changed       )
	EVT_COMMAND_RANGE           (IDC_FIRST_TEXT0ID, IDC_LAST__TEXT0ID, wxEVT_COMMAND_COMBOBOX_SELECTED,
	                             SjAdvSearchDialog::OnRuleText0Changed       )
	EVT_COMMAND_RANGE           (IDC_FIRST_TEXT1ID, IDC_LAST__TEXT1ID, wxEVT_COMMAND_COMBOBOX_SELECTED,
	                             SjAdvSearchDialog::OnRuleText1Changed       )

	EVT_COMMAND_RANGE           (IDC_FIRST_CHOICE0ID, IDC_LAST__CHOICE0ID, wxEVT_COMMAND_CHOICE_SELECTED,
	                             SjAdvSearchDialog::OnRuleChoice0Changed     )
	EVT_COMMAND_RANGE           (IDC_FIRST_CHOICE1ID, IDC_LAST__CHOICE1ID, wxEVT_COMMAND_CHOICE_SELECTED,
	                             SjAdvSearchDialog::OnRuleChoice1Changed     )

	EVT_COMMAND_RANGE           (IDC_FIRST_ADDID, IDC_LAST__ADDID, wxEVT_COMMAND_BUTTON_CLICKED,
	                             SjAdvSearchDialog::OnAddRule                )
	EVT_COMMAND_RANGE           (IDC_FIRST_REMOVEID, IDC_LAST__REMOVEID, wxEVT_COMMAND_BUTTON_CLICKED,
	                             SjAdvSearchDialog::OnRemoveRuleRequest      )
	EVT_MENU                    (IDC_REMOVERULEDO,      SjAdvSearchDialog::OnRemoveRuleDo           )
	EVT_MENU                    (IDC_RELOADRULEDO,      SjAdvSearchDialog::OnReloadRuleDo           )
	EVT_MENU                    (IDC_OPENPLAYLISTDO,    SjAdvSearchDialog::OnOpenPlaylistDo         )
END_EVENT_TABLE()


SjAdvSearchDialog::SjAdvSearchDialog(long preselectId)
	: SjDialog(g_mainFrame, _("Music selection"), SJ_MODELESS, SJ_ALWAYS_RESIZEABLE)
{
	// create the dialog...
	m_mainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_mainSizer);

	wxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
	m_mainSizer->Add(sizer2, 1/*grow*/, wxGROW|wxTOP, SJ_DLG_SPACE);

	// ...search selection
	m_searchListCtrl = new wxListCtrl(this, IDC_SEARCHLIST, wxDefaultPosition, wxSize(160, 100),
	                                  wxLC_REPORT /*| wxLC_SMALL_ICON */ | wxLC_EDIT_LABELS | wxSUNKEN_BORDER );
	m_searchListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
	//m_searchListCtrl->SetImageList(g_tools->GetIconlist(TRUE), wxIMAGE_LIST_NORMAL);
	m_searchListCtrl->InsertColumn(0, _("Saved selections"));
	sizer2->Add(m_searchListCtrl, 0/*grow*/, wxGROW|wxALL, SJ_DLG_SPACE);

	// ...edit search selection
	m_staticBox = new wxStaticBox(this, -1, wxT(""));
	wxSizer* sizer3 = new wxStaticBoxSizer(m_staticBox, wxVERTICAL);
	sizer2->Add(sizer3, 1/*grow*/, wxGROW|wxALL, SJ_DLG_SPACE);

	wxSizer* sizer4 = new wxBoxSizer(wxHORIZONTAL);
	sizer3->Add(sizer4, 0, wxALL, SJ_DLG_SPACE);

	wxString selectText  = _("Search for %s matching %s of the following criteria:");
	wxString selectScope = _("tracks|albums");
	wxString selectOp    = _("all|any|none"); // some other programs hides this popup if there is only one rule, however, in Silverjuke this is not possible because we also have the option "none" here (which may be missing in other programs)

	wxStaticText* staticText = new wxStaticText(this, -1, selectText.BeforeFirst('%'));
	sizer4->Add(staticText, 0, wxALIGN_CENTER_VERTICAL, 0);
	selectText = selectText.AfterFirst('%').Mid(1);

	m_selectScopeChoice = new wxChoice(this, IDC_SELECTCHOICE);
	m_selectScopeChoice->Append(selectScope.BeforeFirst('|'), (void*)SJ_SELECTSCOPE_TRACKS);
	m_selectScopeChoice->Append(selectScope.AfterFirst('|'), (void*)SJ_SELECTSCOPE_ALBUMS);
	SjDialog::SetCbWidth(m_selectScopeChoice);
	sizer4->Add(m_selectScopeChoice, 1, wxALIGN_CENTER_VERTICAL);

	staticText = new wxStaticText(this, -1, selectText.BeforeFirst('%'));
	sizer4->Add(staticText, 0, wxALIGN_CENTER_VERTICAL, 0);

	m_selectOpChoice = new wxChoice(this, IDC_SELECTOPCHOICE);
	m_selectOpChoice->Append(selectOp.BeforeFirst('|'), (void*)SJ_SELECTOP_ALL );
	selectOp = selectOp.AfterFirst ('|');
	m_selectOpChoice->Append(selectOp.BeforeFirst('|'), (void*)SJ_SELECTOP_ANY );
	m_selectOpChoice->Append(selectOp.AfterFirst ('|'), (void*)SJ_SELECTOP_NONE);
	SjDialog::SetCbWidth(m_selectOpChoice);
	sizer4->Add(m_selectOpChoice, 1, wxALIGN_CENTER_VERTICAL);

	staticText = new wxStaticText(this, -1, selectText.AfterFirst('%').Mid(1));
	sizer4->Add(staticText, 0, wxALIGN_CENTER_VERTICAL, 0);

	m_ruleSizer = new wxFlexGridSizer(2, RULE_SPACE, RULE_SPACE);
	m_ruleSizer->AddGrowableCol(1);
	sizer3->Add(m_ruleSizer, 1/*grow*/, wxGROW|wxALL, SJ_DLG_SPACE);

	// ...buttons
	sizer2 = new wxBoxSizer(wxHORIZONTAL);
	m_mainSizer->Add(sizer2, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	m_menuButton = new wxButton(this, IDC_BUTTONBARMENU, _("Menu") + wxString(SJ_BUTTON_MENU_ARROW));
	sizer2->Add(m_menuButton, 0, wxRIGHT, SJ_DLG_SPACE);

	sizer2->Add(1,
	            1,
	            1, wxGROW, 0);

	m_startSearchButton = new wxButton(this, wxID_OK, _("Search"));
	sizer2->Add(m_startSearchButton, 0, wxRIGHT, SJ_DLG_SPACE);

	m_endSearchButton = new wxButton(this, IDC_ENDADVSEARCH, _("End search"));
	sizer2->Add(m_endSearchButton, 0, wxRIGHT, SJ_DLG_SPACE);

	sizer2->Add(new wxButton(this, wxID_CANCEL, _("Close")));

	// check the search in edit
	SjAdvSearch recentSearch;

	if( preselectId > 0 )
	{
		recentSearch = g_advSearchModule->GetSearchById(preselectId);
		if( recentSearch.m_id )
		{
			g_advSearchModule->SetRecentSearch(recentSearch);
		}
	}

	if( recentSearch.m_id == 0 )
	{
		const SjSearch* searchInMain = g_mainFrame->GetSearch();
		if( searchInMain->m_adv.IsSet() )
		{
			recentSearch = g_advSearchModule->GetSearchById(searchInMain->m_adv.m_id);
			if( recentSearch.m_id )
			{
				g_advSearchModule->SetRecentSearch(recentSearch);
			}
		}
	}

	if( recentSearch.m_id == 0 )
	{
		recentSearch = g_advSearchModule->GetRecentSearch();
	}

	// init the search list
	UpdateSearchList(recentSearch);

	// init the search selection
	UpdateSearchControls(recentSearch);

	// finally, set the default button. do it last to avoid
	// miscalculated button sizes eg. under Motif
	if( m_startSearchButton )
	{
		m_startSearchButton->SetDefault();
	}

	m_mainSizer->SetSizeHints(this);

	// let the dialog reflect the state of the main window
	UpdateSearchTitle(recentSearch);
	UpdateSearchButtons();
	UpdateSearchListColours(TRUE/*init*/);


	// set accelerators
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_ADVSEARCH));
	SetFocus();
}


void SjAdvSearchDialog::UpdateIncludeExclude(const SjAdvSearch& s, long changedRule)
{
	wxWindow* focus = FindFocus();
	if( focus==NULL || FindTopLevel(focus)!=this ) changedRule = -1;

	UpdateSearchControls(s, changedRule, 2);
	RuleChanged();
}


void SjAdvSearchDialog::OnOpenPlaylistDo(wxCommandEvent& e)
{
	// processing IDC_OPENPLAYLISTDO
	SjPlaylist tempPlaylist;
	long incl = e.GetExtraLong();
	if( tempPlaylist.AddFromFileDlg(this) )
	{
		wxBusyCursor busy;
		wxSqlt       sql;
		wxString     allUrls;
		long i, cnt = tempPlaylist.GetCount();
		for( i = 0; i<cnt; i++ )
		{
			allUrls += i? wxT(", '") : wxT("'");
			allUrls += sql.QParam( tempPlaylist.Item(i).GetUrl() );
			allUrls += wxT("'");
		}

		SjLLHash ids;
		sql.Query(wxT("SELECT id FROM tracks WHERE url IN (") + allUrls + wxT(");"));
		while( sql.Next() )
		{
			ids.Insert(sql.GetLong(0), 1);
		}
		// wxLogDebug(wxT(" -- IDs : %s"), ids.GetKeysAsString().c_str());

		SjAdvSearch searchToChange = GetSearchInEdit();
		long changedRule = searchToChange.IncludeExclude(&ids, incl? +1 : -1);
		if( changedRule != -1 )
		{
			UpdateIncludeExclude(searchToChange, changedRule);
		}

	}
}


void SjAdvSearchDialog::OnUpdateSearchList(wxCommandEvent&)
{
	UpdateSearchList(g_advSearchModule->GetRecentSearch());
}
void SjAdvSearchDialog::UpdateSearchList(const SjAdvSearch& recentSearch)
{
	m_searchListCtrl->DeleteAllItems();

	int             i, iCount = g_advSearchModule->GetSearchCount();
	SjAdvSearch     s;
	wxListItem      listitem;

	listitem.m_mask = wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT | wxLIST_MASK_DATA;

	for( i = 0; i < iCount; i++ )
	{
		s = g_advSearchModule->GetSearchByIndex(i, FALSE/*rules not needed*/);
		wxASSERT( s.m_id != 0 );

		listitem.m_itemId   = i;
		listitem.m_data     = s.m_id;
		listitem.m_image    = SJ_ICON_ADVSEARCH;
		listitem.m_text     = s.m_name;
		m_searchListCtrl->InsertItem(listitem);
	}

	SetSelectionToSearchListCtrl(recentSearch.m_id);

	EnsureSelListCtrlItemVisible(m_searchListCtrl);

	UpdateSearchListColours(TRUE);
}


void SjAdvSearchDialog::RuleChanged()
{
	SjAdvSearch recentSearch = g_advSearchModule->GetRecentSearch();

	UpdateSearchTitle(recentSearch);
}


void SjAdvSearchDialog::OnSearchInMainChanged(wxCommandEvent&)
{
	SjAdvSearch recentSearch = g_advSearchModule->GetRecentSearch();

	UpdateSearchTitle(recentSearch);
	UpdateSearchButtons();
	UpdateSearchListColours(FALSE/*no init*/);
}


SjAdvSearch SjAdvSearchDialog::GetSearchInEdit() const
{
	SjAdvSearch s;

	s.m_selectScope= m_selectScopeChoice? (SjSelectScope)GetCbSelection(m_selectScopeChoice) : SJ_SELECTSCOPE_DEFAULT;
	s.m_selectOp   = m_selectOpChoice?    (SjSelectOp)   GetCbSelection(m_selectOpChoice   ) : SJ_SELECTOP_DEFAULT;

	int i, iCount = (int)m_ruleControls.GetCount();
	SjRule currRule;
	for( i = 0; i < iCount; i++ )
	{
		s.AddRule(m_ruleControls[i].GetRuleInEdit());
	}

	// add the ID and the name from the recent search
	SjAdvSearch recentSearch = g_advSearchModule->GetRecentSearch();
	s.m_id = recentSearch.m_id;
	s.m_name = recentSearch.m_name;

	return s;
}


bool SjAdvSearchDialog::IsSearchInEditEqualToRecentSearch()
{
	return (GetSearchInEdit()==g_advSearchModule->GetRecentSearch());
}


bool SjAdvSearchDialog::IsSearchInEditEqualToSearchInMain()
{
	// get search in main
	const SjSearch* searchInMain = g_mainFrame->GetSearch();
	if( searchInMain->m_adv.IsSet() )
	{
		// compare the searches
		return (GetSearchInEdit()==searchInMain->m_adv);
	}

	return FALSE;
}


bool SjAdvSearchDialog::SaveSearchInEditIfNeeded(bool askUser)
{
	// search modified?
	if( IsSearchInEditEqualToRecentSearch() )
	{
		return TRUE; // search not modified, the caller should continue processing (eg. close the dialog)
	}

	// search modified. ask the user what to do
	SjAdvSearch recentSearch = GetSearchInEdit();
	if( askUser )
	{
		switch( g_advSearchModule->m_saveSearches )
		{
			case SJ_SAVESEARCHES_DONTSAVE:
				return TRUE; // don't save but continue processing

			case SJ_SAVESEARCHES_SAVE:
				break; // always save search

			default:
			{
				wxWindowDisabler disabler(this);
				int confirm = SjMessageBox(
				                  wxString::Format(_("Do you want to save your modifications to the music selection \"%s\"?"),
				                                   recentSearch.m_name.c_str()),
				                  _("Music selection modified"),
				                  wxYES_NO | wxCANCEL | wxICON_QUESTION | wxYES_DEFAULT, this);

				if( confirm == wxCANCEL )
				{
					return FALSE; // cancel processing
				}
				else if( confirm == wxNO )
				{
					return TRUE; // don't save but continue processing
				}
			}
			break;
		}
	}

	// save the search
	g_advSearchModule->SaveSearch(recentSearch);
	#if SJ_USE_PENDING_MUSIC_SEL
	g_advSearchModule->Flush();
	#endif

	// save done, update title
	UpdateSearchTitle(recentSearch);

	return true; // saved, continue processing
}


void SjAdvSearchDialog::UpdateSearchTitle(const SjAdvSearch& recentSearch)
{
	// create the title
	bool modified = !IsSearchInEditEqualToRecentSearch();

	wxString title = wxString::Format(_("Edit music selection \"%s\""), recentSearch.m_name.c_str());

	if( modified )
	{
		title += wxT(" *");
	}

	const SjSearchStat* searchStat = g_mainFrame->GetSearchStat();
	if( IsSearchInEditEqualToSearchInMain()
	        && searchStat->m_advResultCount != -1 )
	{
		title += wxT(" - ") + SjAdvSearchModule::FormatResultString(g_mainFrame->GetSearchStat()->m_advResultCount);

		if( searchStat->m_mbytes > 0 )
		{
			title += wxT(" - ") + SjTools::FormatBytes(searchStat->m_mbytes, SJ_FORMAT_MB|SJ_FORMAT_ADDEXACT);
		}

		if( searchStat->m_seconds > 0 )
		{
			title += wxT(" - ") + SjTools::FormatTime(searchStat->m_seconds);
		}
	}


	// set the title
	m_staticBox->SetLabel(title);
}


void SjAdvSearchDialog::UpdateSearchListColours(bool init)
{
	static long     oldSearchIdInMain; if( init ) oldSearchIdInMain = 0;
	long            newSearchIdInMain = g_mainFrame->GetSearch()->m_adv.m_id;

	if( oldSearchIdInMain != newSearchIdInMain )
	{
		// update the old item
		long itemId = m_searchListCtrl->FindItem(-1, oldSearchIdInMain);
		if( itemId >= 0 )
		{
			wxListItem li;
			li.SetId(itemId);
			m_searchListCtrl->GetItem(li);
			li.SetImage(SJ_ICON_ADVSEARCH);
			m_searchListCtrl->SetItem(li);
		}

		// update the new item
		itemId = m_searchListCtrl->FindItem(-1, newSearchIdInMain);
		if( itemId >= 0 )
		{
			wxListItem li;
			li.SetId(itemId);
			m_searchListCtrl->GetItem(li);
			li.SetImage(SJ_ICON_ADVSEARCH_SEL);
			m_searchListCtrl->SetItem(li);
		}

		oldSearchIdInMain = newSearchIdInMain;
	}
}


void SjAdvSearchDialog::UpdateSearchButtons()
{
	bool mainHasSearch = g_mainFrame->HasAnySearch();
	if( mainHasSearch )
	{
		m_endSearchButton->Enable(TRUE);
	}
	else
	{
		if( wxWindow::FindFocus()==m_endSearchButton )
		{
			m_startSearchButton->SetFocus();
		}
		m_endSearchButton->Enable(FALSE);
	}
}


void SjAdvSearchDialog::UpdateSearchControls(const SjAdvSearch& advSearch, long focusIndex,
        long focusRepostValue)
{
	wxLogDebug(wxT("BEGIN UPDATING SEARCH CONTROLS..."));

	// "on the fly" update
	if( IsShown() )
	{
		// freeze the window to avoid flickering
		Freeze();
	}

	// remove all children belonging to previous rules
	int i, iCount = m_ruleControls.GetCount();
	for( i = 0; i < iCount; i++ ) { m_ruleControls[i].DestroyChildren(); }

	m_ruleControls.Clear();

	// init basic search operations
	SetCbSelection(m_selectScopeChoice, advSearch.m_selectScope);
	SetCbSelection(m_selectOpChoice, advSearch.m_selectOp);

	// add new rules...
	iCount = advSearch.m_rules.GetCount();
	if( iCount > MAX_RULES ) iCount = MAX_RULES;
	if( iCount )
	{
		// ...add existing rules
		if( focusIndex > iCount-1 ) focusIndex = iCount-1;
		for( i = 0; i < iCount; i++ )
		{
			m_ruleControls.Add(new SjRuleControls(advSearch.m_rules[i], this, m_ruleSizer, i, i==focusIndex, focusRepostValue));
		}
	}
	else
	{
		// ...make sure, there is at least one (empty) rule
		if( iCount == 0 )
		{
			SjRule emptyRule;
			m_ruleControls.Add(new SjRuleControls(emptyRule, this, m_ruleSizer, 0, focusIndex!=-1, focusRepostValue));
		}
	}

	// "on the fly" update
	if( IsShown() )
	{
		// re-layout the rule controls
		m_ruleSizer->Layout();

		// enlarge the window, if needed...

		// ...get current window size
		wxSize oldWindowSize = GetSize();

		// ...get borders size
		wxSize borderSize = GetClientSize();
		borderSize.x = oldWindowSize.x-borderSize.x;
		borderSize.y = oldWindowSize.y-borderSize.y;

		// ...calculate new window size (add the borders to the main sizer's minimal size)
		//
		wxSize newWindowSize = m_mainSizer->GetMinSize();
		newWindowSize.x += borderSize.x;
		newWindowSize.y += borderSize.y;
		wxSize minWindowSize = newWindowSize;

		// ...set the new window size (however, we do not shrink the window)
		if( newWindowSize.x > oldWindowSize.x
		        || newWindowSize.y > oldWindowSize.y )
		{
			if( newWindowSize.x < oldWindowSize.x ) newWindowSize.x = oldWindowSize.x;
			if( newWindowSize.y < oldWindowSize.y ) newWindowSize.y = oldWindowSize.y;
			SetSize(newWindowSize);
		}

		// ...set a hint for the minimal window size
		SetSizeHints(minWindowSize.x, minWindowSize.y);

		// thaw the freezed window
		Thaw();
	}

	wxLogDebug(wxT("...END UPDATING SEARCH CONTROLS"));

	Refresh();
}


wxArrayLong SjAdvSearchDialog::GetSelectionFromSearchListCtrl(long* retSingleSelIndex)
{
	// function returns an array of selected search IDs;
	// moreover, retSingleSelIndex may return the first selected index
	wxArrayLong ret;

	if( retSingleSelIndex )
	{
		*retSingleSelIndex = -1;
	}

	long i, iCount = m_searchListCtrl->GetItemCount();
	for( i = 0; i < iCount; i++ )
	{
		if( m_searchListCtrl->GetItemState(i, wxLIST_STATE_SELECTED) &  wxLIST_STATE_SELECTED )
		{
			ret.Add(m_searchListCtrl->GetItemData(i));
			if( retSingleSelIndex && *retSingleSelIndex == -1 )
			{
				*retSingleSelIndex = i;
			}
		}
	}

	return ret;
}


void SjAdvSearchDialog::SetSelectionToSearchListCtrl(long id)
{
	long i = m_searchListCtrl->FindItem(-1, id);
	if( i < 0 || i > m_searchListCtrl->GetItemCount() )
	{
		i = 0;
	}
	m_searchListCtrl->SetItemState(i, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
}


void SjAdvSearchDialog::ShowContextMenu(wxWindow* window, const wxPoint& pt)
{
	// SjAdvSearch searchInEdit = GetSearchInEdit(); -- the "search in edit" is in changing at the moment the context menu appears
	SjMenu m(SJ_SHORTCUTS_LOCAL);

	m.Append(IDC_NEWSEARCH);

	m.AppendSeparator();

	m.Append(IDC_SAVESEARCH);
	m.Enable(IDC_SAVESEARCH, !IsSearchInEditEqualToRecentSearch());

	m.Append(IDC_SAVESEARCHAS);

	m.Append(IDC_RENAMESEARCH);

	m.Append(IDC_REVERTTOSAVEDSEARCH);
	m.Enable(IDC_REVERTTOSAVEDSEARCH, !IsSearchInEditEqualToRecentSearch());

	m.Append(IDC_DELETESEARCH);

	m.AppendSeparator();

	m.Append(wxID_OK, _("Search"));
	m.Append(IDC_ENDADVSEARCH, _("End search"));
	m.Enable(IDC_ENDADVSEARCH, g_mainFrame->HasAnySearch());

	m.AppendSeparator();

	m.Append(IDT_ENQUEUE_LAST);
	m.Append(IDT_ENQUEUE_NOW);

	window->PopupMenu(&m, pt);
}


void SjAdvSearchDialog::OnSearchSelChange(wxListEvent&)
{
	// we do not update the stuff directly as the (maybe appearing) message box
	// disturbs the selection progress
	QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_SEARCHSELCHANGEDO));
}
void SjAdvSearchDialog::OnSearchSelChangeDo(wxCommandEvent&)
{
	wxArrayLong selSearches = GetSelectionFromSearchListCtrl();
	if( selSearches.GetCount() )
	{
		SjAdvSearch selSearch = g_advSearchModule->GetSearchById(selSearches[0]);

		if( selSearch != g_advSearchModule->GetRecentSearch() )
		{
			if( !SaveSearchInEditIfNeeded(TRUE) )
			{
				// reset search listbox selection
				QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_UPDATESEARCHLIST));
				return;
			}

			g_advSearchModule->SetRecentSearch(selSearch);
			UpdateSearchControls(selSearch);
			UpdateSearchTitle(selSearch);
		}
	}
}


void SjAdvSearchDialog::OnNewOrSaveAsSearch(wxCommandEvent& event)
{
	bool        saveSearchAs = (event.GetId()==IDC_SAVESEARCHAS);
	long        index;

	if( !saveSearchAs )
	{
		// for "new", ask the user first, if the search in edit is modified
		if( !SaveSearchInEditIfNeeded() )
		{
			return;
		}
	}

	// create a new search
	SjAdvSearch tempSearchInEdit = GetSearchInEdit();
	SjAdvSearch newSearch = g_advSearchModule->NewSearch(saveSearchAs? &tempSearchInEdit : NULL);
	if( newSearch.m_id )
	{
		g_advSearchModule->SetRecentSearch(newSearch);
		UpdateSearchList(newSearch);
		UpdateSearchControls(newSearch);
		UpdateSearchTitle(newSearch);

		if( GetSelectionFromSearchListCtrl(&index).GetCount() )
		{
			m_searchListCtrl->EditLabel(index);
		}
	}
}


void SjAdvSearchDialog::OnRenameSearch(wxCommandEvent&)
{
	// just start editing the label, "real" renaming is done in OnSearchEndEditLabel()
	long index;
	if( GetSelectionFromSearchListCtrl(&index).GetCount() )
	{
		m_searchListCtrl->EditLabel(index);
	}
}


void SjAdvSearchDialog::OnSearchEndEditLabel(wxListEvent& event)
{
	// edit cancelled?
	// these two lines were edited to the bug http://www.silverjuke.net/forum/post.php?p=8161#8161
	if( event.IsEditCancelled() )
		return;

	// rename the search
	SjAdvSearch searchInRename = g_advSearchModule->GetSearchById(event.GetData());
	if( searchInRename.m_id )
	{
		wxString newName = event.GetLabel();

		// save
		g_advSearchModule->RenameSearch(searchInRename, newName);
		#if SJ_USE_PENDING_MUSIC_SEL
		g_advSearchModule->Flush();
		#endif

		// update controls
		QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_UPDATESEARCHLIST));

		if( searchInRename.m_id == g_mainFrame->GetSearch()->m_adv.m_id )
		{
			// re-search in main window; this will also update our list colours
			// by the module message
			QueueEvent(new wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK));
		}

		// update search title (GetRecentSearch() may be unequal to savedSearch)
		UpdateSearchTitle(g_advSearchModule->GetRecentSearch());

		// renaming implies saving the search
		SaveSearchInEditIfNeeded(false);
	}
}


void SjAdvSearchDialog::OnSearchKeyDown(wxListEvent& event)
{
	switch( event.GetKeyCode() )
	{
		case WXK_DELETE:
		case WXK_BACK:
		{
			QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_DELETESEARCH));
		}
		break;

		case WXK_INSERT:
		{
			QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_NEWSEARCH));
		}
		break;
	}
}


void SjAdvSearchDialog::OnDeleteSearch(wxCommandEvent&)
{
	// get selected searches
	wxArrayLong selSearchIds = GetSelectionFromSearchListCtrl();
	if( selSearchIds.GetCount() == 0 )
	{
		selSearchIds.Add(g_advSearchModule->GetRecentSearch().m_id);
	}

	// ask the user
	wxString msg, title;
	SjAdvSearch searchToDelete;
	if( selSearchIds.GetCount() == 1 )
	{
		searchToDelete = g_advSearchModule->GetSearchById(selSearchIds[0], FALSE);
		if( !searchToDelete.m_id ) { return; }

		msg = wxString::Format(_("Do you really want to delete the music selection \"%s\"?"),
		                       searchToDelete.m_name.c_str());
		title = _("Delete music selection");
	}
	else
	{
		msg = wxString::Format(_("Do you really want to delete the %i selected music selections?"), (int)selSearchIds.GetCount());
		title = _("Delete music selections");
	}

	{
		wxWindowDisabler disabler(this);
		if( SjMessageBox(msg, title,
		                   wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT, this) == wxNO )
		{
			return;
		}
	}

	// delete the searches
	int i, iCount = (int)selSearchIds.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		searchToDelete = g_advSearchModule->GetSearchById(selSearchIds[i]);

		if( g_mainFrame->GetSearch()->m_adv.m_id == searchToDelete.m_id )
		{
			g_mainFrame->EndAllSearch();
		}

		g_advSearchModule->DeleteSearch(searchToDelete);
	}

	// update dialog
	SjAdvSearch recentSearch = g_advSearchModule->GetRecentSearch();

	UpdateSearchList(recentSearch);
	UpdateSearchControls(recentSearch);
	UpdateSearchTitle(recentSearch);
}


void SjAdvSearchDialog::OnRevertToSavedSearch(wxCommandEvent&)
{
	SjAdvSearch recentSearch = g_advSearchModule->GetRecentSearch();

	UpdateSearchControls(recentSearch);
	UpdateSearchTitle(recentSearch);
}


bool SjAdvSearchDialog::OnSearchCheckObviousErrors(const SjAdvSearch& advSearch, wxString& warnings)
{
	wxString errors;
	if( !advSearch.IsOk(errors, warnings) )
	{
		wxWindowDisabler disabler(this);
		SjMessageBox(errors, GetTitle(), wxOK | wxICON_WARNING, this);
		return FALSE;
	}
	return TRUE;
}


void SjAdvSearchDialog::OnSearchShowErrors(const SjAdvSearch& advSearch, wxString& warnings)
{
	// ...no results found - prepend this information to (optional) warnings given from OnSearchCheckObviousErrors()
	if( !warnings.IsEmpty() ) warnings.Prepend(wxT(" "));
	warnings.Prepend(wxString::Format(_("No matches found for \"%s\"."), advSearch.m_name.c_str()));

	if( wxLog::GetActiveTarget()->HasPendingMessages() && SjLogGui::GetAutoOpen() )
	{
		// ...there are already some errors logged (mostly by a bad SQL syntax
		// for user-defined SQL-Expressions) -> log our warning
		wxLogWarning(warnings);
	}
	else
	{
		// ...there are no errors logged -> print our warning directly (looks better)
		wxWindowDisabler disabler(this);
		SjMessageBox(warnings, GetTitle(), wxOK | wxICON_INFORMATION, this);
	}
}


void SjAdvSearchDialog::OnSearch(wxCommandEvent&)
{
	SjAdvSearch advSearch = GetSearchInEdit();

	// any obvious errors?
	wxString warnings;
	if( !OnSearchCheckObviousErrors(advSearch, warnings) )
	{
		return;
	}

	// do the search...
	g_mainFrame->SetSearch(SJ_SETSEARCH_CLEARSIMPLE|SJ_SETSEARCH_SETADV, wxT(""), &advSearch);
	if( g_mainFrame->GetSearchStat()->m_advResultCount == 0 )
	{
		OnSearchShowErrors(advSearch, warnings);
	}
}


void SjAdvSearchDialog::OnPlay(wxCommandEvent& e)
{
	SjHaltAutoPlay  haltAutoPlay;
	SjAdvSearch     advSearch = GetSearchInEdit();

	// any obvious errors?
	wxString warnings;
	if( !OnSearchCheckObviousErrors(advSearch, warnings) )
	{
		return;
	}

	::wxBeginBusyCursor();

	// get all tracks
	SjLLHash trackIdsHash;
	wxString selectSql;
	SjSearchStat stat;
	{
		wxBusyCursor busy;
		stat = advSearch.GetAsSql(&trackIdsHash, selectSql);
	}

	if( stat.m_advResultCount == 0 )
	{
		::wxEndBusyCursor();
		OnSearchShowErrors(advSearch, warnings);
		return;
	}

	// enqueue all tracks
	wxArrayString urls;
	g_mainFrame->m_libraryModule->GetOrderedUrlsFromIDs(trackIdsHash, urls);

	if( e.GetId() == IDT_ENQUEUE_NOW )
	{
		g_mainFrame->UnqueueAll();
	}

	g_mainFrame->Enqueue(urls, -1, TRUE/*verified*/);

	::wxEndBusyCursor();
}


void SjAdvSearchDialog::OnEndSearch(wxCommandEvent&)
{
	g_mainFrame->SetSearch(SJ_SETSEARCH_CLEARSIMPLE|SJ_SETSEARCH_CLEARADV);
}


void SjAdvSearchDialog::OnSize(wxSizeEvent& e)
{
	SjDialog::OnSize(e);

	wxSize size;
	size = m_searchListCtrl->GetClientSize();
	m_searchListCtrl->SetColumnWidth(0, size.x);
}


void SjAdvSearchDialog::OnClose()
{
	if( !SaveSearchInEditIfNeeded() )
	{
		return;
	}

	const SjSearch* searchInMain = g_mainFrame->GetSearch();
	const SjSearchStat* searchStatInMain = g_mainFrame->GetSearchStat();
	if( searchInMain->m_adv.IsSet() && searchStatInMain->m_advResultCount == 0 )
	{
		g_mainFrame->SetSearch(SJ_SETSEARCH_CLEARSIMPLE|SJ_SETSEARCH_CLEARADV, wxT(""), NULL);
	}

	g_advSearchModule->CloseDialog(); // Calls Destroy(), do not used delete!
}


/*******************************************************************************
 * SjAdvSearchDialog - handling rules
 ******************************************************************************/


long SjAdvSearchDialog::GetRuleIndex(long ruleId)
{
	int i, iCount = (int)m_ruleControls.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( m_ruleControls[i].m_ruleId == ruleId )
		{
			return i;
		}
	}
	return -1;
}


void SjAdvSearchDialog::OnRuleFieldChanged(wxCommandEvent& event)
{
	long ruleIndex = GetRuleIndex(event.GetId()-IDC_FIRST_FIELDID);
	if( ruleIndex != -1 )
	{
		m_ruleControls[ruleIndex].OnFieldChanged();
	}
}


void SjAdvSearchDialog::OnRuleOpChanged(wxCommandEvent& event)
{
	long ruleIndex = GetRuleIndex(event.GetId()-IDC_FIRST_OPID);
	if( ruleIndex != -1 )
	{
		m_ruleControls[ruleIndex].OnOpChanged();
	}
}


void SjAdvSearchDialog::OnRuleText0Changed(wxCommandEvent& event)
{
	long ruleIndex = GetRuleIndex(event.GetId()-IDC_FIRST_TEXT0ID);
	if( ruleIndex != -1 )
	{
		m_ruleControls[ruleIndex].OnTextChanged(0, event.GetEventType());
	}
}


void SjAdvSearchDialog::OnRuleText1Changed(wxCommandEvent& event)
{
	long ruleIndex = GetRuleIndex(event.GetId()-IDC_FIRST_TEXT1ID);
	if( ruleIndex != -1 )
	{
		m_ruleControls[ruleIndex].OnTextChanged(1, event.GetEventType());
	}
}


void SjAdvSearchDialog::OnRuleChoice0Changed(wxCommandEvent& event)
{
	long ruleIndex = GetRuleIndex(event.GetId()-IDC_FIRST_CHOICE0ID);
	if( ruleIndex != -1 )
	{
		m_ruleControls[ruleIndex].OnChoiceChanged(0);
	}
}


void SjAdvSearchDialog::OnRuleChoice1Changed(wxCommandEvent& event)
{
	long ruleIndex = GetRuleIndex(event.GetId()-IDC_FIRST_CHOICE1ID);
	if( ruleIndex != -1 )
	{
		m_ruleControls[ruleIndex].OnChoiceChanged(1);
	}
}


void SjAdvSearchDialog::OnRemoveRuleRequest(wxCommandEvent& event)
{
	// delay the remove event as otherwise the clicked button gets deleted before
	// this function returns
	m_ruleTempId = event.GetId()-IDC_FIRST_REMOVEID;
	QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_REMOVERULEDO));
}
void SjAdvSearchDialog::OnRemoveRuleDo(wxCommandEvent& event)
{
	long ruleIndex = GetRuleIndex(m_ruleTempId);
	if( ruleIndex != -1 )
	{
		SjAdvSearch searchInEdit = GetSearchInEdit();

		searchInEdit.m_rules.RemoveAt(ruleIndex);

		UpdateSearchControls(searchInEdit, ruleIndex);
		RuleChanged();
	}
}


void SjAdvSearchDialog::ReloadRule(long ruleId, long focusRepostValue)
{
	// delay the reload event as otherwise the clicked control gets deleted before
	// this function returns
	m_ruleTempId = ruleId;
	m_ruleTempFocusRepostValue = focusRepostValue;
	QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_RELOADRULEDO));
}
void SjAdvSearchDialog::OnReloadRuleDo(wxCommandEvent& event)
{
	long ruleIndex = GetRuleIndex(m_ruleTempId);
	if( ruleIndex != -1 )
	{
		SjAdvSearch searchInEdit = GetSearchInEdit();

		UpdateSearchControls(searchInEdit, ruleIndex, m_ruleTempFocusRepostValue);
		RuleChanged();
	}
}


void SjAdvSearchDialog::OnAddRule(wxCommandEvent& event)
{
	long ruleIndex = GetRuleIndex(event.GetId()-IDC_FIRST_ADDID);
	if( ruleIndex != -1 )
	{
		SjAdvSearch searchInEdit = GetSearchInEdit();

		if( ruleIndex == (long)searchInEdit.m_rules.GetCount()-1 )
		{
			searchInEdit.m_rules.Add(new SjRule());
		}
		else
		{
			searchInEdit.m_rules.Insert(new SjRule(), ruleIndex+1);
		}

		UpdateSearchControls(searchInEdit, ruleIndex+1);
		RuleChanged();
	}
}


/*******************************************************************************
 * SjAdvSearchModule
 ******************************************************************************/


SjAdvSearchModule::SjAdvSearchModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)

{
	wxASSERT( IDC_LAST__REMOVEID <= IDM_LASTPRIVATE );

	m_file              = wxT("memory:advsearch.lib");
	m_name              = _("Music selection");
	m_dialog            = NULL;
	m_recentSearchId    = 0;
	m_flags             = SJ_SEARCHFLAG_DEFAULTS;
	m_maxHistorySize    = SJ_DEFAULT_HISTORY_SIZE;
	m_saveSearches      = SJ_SAVESEARCHES_ASK;
	m_needsHistorySave  = FALSE;
	m_historyLoaded     = FALSE;
}


SjAdvSearchModule::~SjAdvSearchModule()
{
	g_advSearchModule = NULL;
}


bool SjAdvSearchModule::FirstLoad()
{
	// read some settings
	m_flags = g_tools->m_config->Read(wxT("advsearch/flags"), SJ_SEARCHFLAG_DEFAULTS);
	m_saveSearches = g_tools->m_config->Read(wxT("advsearch/saveSearches"), SJ_SAVESEARCHES_ASK);
	m_maxHistorySize = g_tools->m_config->Read(wxT("advsearch/maxHistorySize"), SJ_DEFAULT_HISTORY_SIZE);
	m_recentSearchId = g_tools->m_config->Read(wxT("advsearch/recentSearchId"), 0L);
	g_advSearchModule = this;

	// init pending state
	#if SJ_USE_PENDING_MUSIC_SEL
	m_pendingNeedsSave = false;
	#endif

	// create the table to save searches in
	wxSqlt sql;
	if( !sql.TableExists(wxT("advsearch")) )
	{
		sql.Query(  wxT("CREATE TABLE advsearch (")
		            wxT("id INTEGER PRIMARY KEY AUTOINCREMENT, ")
		            wxT("name TEXT, ")
		            wxT("rules TEXT);")
		         );

		AddPredefinedSearches();
	}

	return TRUE;
}


void SjAdvSearchModule::LastUnload()
{
	CloseDialog(); // implies Flush()
	SaveHistory();
}


void SjAdvSearchModule::ReceiveMsg(int msg)
{
	if( msg == IDMODMSG_SEARCH_CHANGED )
	{
		if( m_dialog )
		{
			m_dialog->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_SEARCHINMAINCHANGED));
		}
	}
	else if( msg == IDMODMSG_KIOSK_STARTING || msg == IDMODMSG_WINDOW_CLOSE )
	{
		CloseDialog();
	}
}


void SjAdvSearchModule::OpenDialog(long preselectId)
{
	{
		wxBusyCursor busy;

		if( IsDialogOpen() )
		{
			// raise existing dialog - the preselect id is ignored
			// in this case
			m_dialog->Raise();
		}
		else
		{
			// open new dialog
			m_dialog = new SjAdvSearchDialog(preselectId /*-1 for nothing special*/);

			// load position
			m_dialog->CenterOnParent();
		}
	}

	// show dialog
	m_dialog->Show();
}


void SjAdvSearchModule::CloseDialog()
{
	if( m_dialog )
	{
		// save search in view
		g_tools->m_config->Write(wxT("advsearch/recentSearchId"), m_recentSearchId);

		// destroy (not: delete) dialog
		m_dialog->Destroy();
		m_dialog = NULL;
	}

	// save unsaved searches - even if the dialog was not opened
	#if SJ_USE_PENDING_MUSIC_SEL
	Flush();
	#endif
}


bool SjAdvSearchModule::IncludeExclude(SjLLHash* ids, bool delPressed)
{
	// returns TRUE if tracks were deleted from the view of the caller
	SjAdvSearch         searchToChange;
	const SjAdvSearch*  searchInMain = &(g_mainFrame->GetSearch()->m_adv);
	unsigned long       thisTime = SjTools::GetMsTicks();

	if( delPressed )
	{
		// exclude tracks or reset ex-/including when a subset is modified
		bool            deleteTracksFromBrowser = TRUE;
		int             action = -1;

		// get the search to modify
		if( searchInMain->m_subsetId )
		{
			// we modify a subset of included/excluded tracks
			if( IsDialogOpen() )
			{
				searchToChange = m_dialog->GetSearchInEdit();
				if( searchToChange.m_id == searchInMain->m_subsetId )
				{
					action = 0;
				}
				else
				{
					deleteTracksFromBrowser = FALSE;
				}
			}
			else
			{
				searchToChange = GetSearchById(searchInMain->m_subsetId);
				action = 0;
			}
		}
		else
		{
			// we modify a "normal" adv. search
			if( IsDialogOpen() )
			{
				searchToChange = m_dialog->GetSearchInEdit();
				if( searchToChange.m_id != searchInMain->m_id )
				{
					deleteTracksFromBrowser = FALSE;
				}
			}
			else
			{
				searchToChange = GetSearchById(searchInMain->m_id);
			}
		}

		// check, the adv. search id is valid
		if( searchToChange.m_id == 0 )
		{
			return FALSE;
		}

		// ask the user if the ID of the search has changed or if more than
		// three minutes are gone before the last deletion
		if( action == -1 )
		{
			static long             s_askedDelForId = 0;
			static unsigned long    s_askedDelTime = 0;
			if( s_askedDelForId != searchToChange.m_id
			 || s_askedDelTime+180*1000 < thisTime )
			{
				wxWindowDisabler disabler(g_mainFrame);
				if( SjMessageBox(
				            wxString::Format(_("Do you want to remove the selected track(s) from the music selection \"%s\"?"),
				                             searchToChange.m_name.c_str()),
				            m_name, wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT, g_mainFrame) == wxNO )
				{
					return FALSE;
				}
				s_askedDelForId = searchToChange.m_id;
			}
			s_askedDelTime = thisTime;
		}

		// modify the search
		long changedRule = searchToChange.IncludeExclude(ids, action);
		if( changedRule != -1 )
		{
			if( IsDialogOpen() )
			{
				m_dialog->UpdateIncludeExclude(searchToChange, changedRule);
			}
			else
			{
				SaveSearch(searchToChange);
			}
		}
		else
		{
			deleteTracksFromBrowser = FALSE;
		}

		return deleteTracksFromBrowser;
	}
	else
	{
		// include tracks

		// if the dialog is not opened, do nothing
		if( !IsDialogOpen() )
		{
			return FALSE;
		}

		// get the search to modify
		searchToChange = m_dialog->GetSearchInEdit();

		wxASSERT( searchToChange.m_id != 0 );
		if( searchToChange.m_id == 0 ) { return FALSE; } // unknown search, nothing to do

		// ask the user
		static long             s_askedInsForId = 0;
		static unsigned long    s_askedInsTime = 0;
		if( s_askedInsForId != searchToChange.m_id
		 || s_askedInsTime+180*1000 < thisTime )
		{
			wxWindowDisabler disabler(g_mainFrame);
			if( SjMessageBox(
			            wxString::Format(_("Do you want to add the selected track(s) to the music selection \"%s\"?"),
			                             searchToChange.m_name.c_str()),
			            m_name, wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT, g_mainFrame) == wxNO )
			{
				return FALSE;
			}
			s_askedInsForId = searchToChange.m_id;
		}
		s_askedInsTime = thisTime;

		// modify the search
		long changedRule = searchToChange.IncludeExclude(ids, +1);
		if( changedRule != -1 )
		{
			wxASSERT( IsDialogOpen() );
			m_dialog->UpdateIncludeExclude(searchToChange, changedRule);
		}

		return FALSE;
	}
}


bool SjAdvSearchModule::DragNDrop(SjDragNDropAction action, wxWindow* windowAtMouse, const wxPoint& pos,
                                  SjDataObject* data)
{
	if( windowAtMouse == g_mainFrame )
	{
		// hack: drag'n'drop for the main window is handled here
		int  targetId    = g_mainFrame->FindTargetId(pos.x, pos.y);
		bool overDisplay = ((targetId>=IDT_DISPLAY_LINE_FIRST && targetId<=IDT_DISPLAY_LINE_LAST) || targetId==IDT_PLAY);
		bool acceptable  = ((g_mainFrame->IsAllAvailable() && !g_mainFrame->m_internalDragNDrop) || (g_mainFrame->m_internalDragNDrop && overDisplay));

		if( action == SJ_DND_MOVE )
		{
			return acceptable;
		}
		else if( action == SJ_DND_DROP && acceptable && data )
		{
			g_mainFrame->OpenData(data, SJ_OPENFILES_ENQUEUE, pos.x, pos.y);
		}
		return TRUE;
	}
	else if( windowAtMouse == m_dialog )
	{
		// drag'n'drop over the search dialog
		if( action == SJ_DND_DROP
		 && data
		 && data->m_fileData )
		{
			const wxArrayString& filenames = data->m_fileData->GetFilenames();
			SjLLHash hash;
			int i, iCount = filenames.GetCount();
			for( i = 0; i < iCount; i++ )
			{
				hash.Insert(g_mainFrame->m_libraryModule->GetId(filenames.Item(i)), 1);
			}
			IncludeExclude(&hash, FALSE);
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/*******************************************************************************
 * SjAdvSearchModule - handling the saved searches
 ******************************************************************************/


long SjAdvSearchModule::GetSearchCount() const
{
	wxSqlt sql;
	sql.Query(wxT("SELECT COUNT(*) FROM advsearch"));
	return sql.GetLong(0);
}


SjAdvSearch SjAdvSearchModule::GetSearch__(long index__, long id__, const wxString& name__, bool getRules) const
{
	wxLogDebug(wxT("      used pending ? "));
	#if SJ_USE_PENDING_MUSIC_SEL
	if( index__ == -1 && m_pendingSearch.m_id > 0 && m_pendingSearch.m_id == id__ )
	{
		wxLogDebug(wxT(" :-)) used pending ... "));
		return m_pendingSearch;
	}
	else
	#endif
	{
		SjAdvSearch ret;
		wxSqlt sql;

		if( !name__.IsEmpty() )
		{
			wxASSERT( id__ == -1 && index__ == -1 );
			sql.Query(wxT("SELECT id,name,rules FROM advsearch WHERE name='") + sql.QParam(name__) + wxT("';"));
		}
		else if( index__ == -1 )
		{
			wxASSERT( id__ != -1 );
			sql.Query(wxString::Format(wxT("SELECT id,name,rules FROM advsearch WHERE id=%i;"), (int)id__));
		}
		else
		{
			wxASSERT( id__ == -1 );
			sql.Query(wxString::Format(wxT("SELECT id,name,rules FROM advsearch ORDER BY SORTABLE(name) LIMIT %i,1;"), (int)index__));
		}

		if( sql.Next() )
		{
			ret.m_id = sql.GetLong(0);
			#if SJ_USE_PENDING_MUSIC_SEL
			if( m_pendingSearch.m_id > 0 && m_pendingSearch.m_id == ret.m_id )
			{
				wxLogDebug(wxT(" :-)  used pending ... "));
				return m_pendingSearch;
			}
			else
			#endif
			{
				ret.m_name = sql.GetString(1);
				if( getRules )
				{
					SjStringSerializer ser(sql.GetString(2));
					ret.Unserialize(ser);
					if( ser.HasErrors() )
					{
						ret.m_rules.Clear();
					}

					// the name and the ID are also stored together with the rules;
					// however, eg. for external scripts it is easier to leave them empty, so always prefer
					// the "id" and the "name" fiels from the database.
					// see http://www.silverjuke.net/forum/topic-2540.html
					wxASSERT( ret.m_id == 0 || ret.m_id == sql.GetLong(0) );
					wxASSERT( ret.m_name.IsEmpty() || ret.m_name == sql.GetString(1) );

					ret.m_id = sql.GetLong(0);
					ret.m_name = sql.GetString(1);
				}
			}
		}

		return ret;
	}
}


void SjAdvSearchModule::AddPredefinedSearches()
{
	SjAdvSearch s;

	/* s.Init("All tracks", SJ_SELECTSCOPE_TRACKS, SJ_SELECTOP_ALL);
	s.AddRule(SJ_FIELD_TRACKNAME, SJ_FIELDOP_IS_SET, "", "");
	NewSearch(&s);

	-- not really needed; first, i thought this is useful for the auto-play function,
	-- however, using the predefined search "random selection" has the same effect.
	*/

	s.Init(_("70's music"), SJ_SELECTSCOPE_TRACKS, SJ_SELECTOP_ALL);
	s.AddRule(SJ_FIELD_YEAR, SJ_FIELDOP_IS_IN_RANGE, wxT("1970"), wxT("1979"));
	NewSearch(&s);

	s.Init(_("Worst rated"), SJ_SELECTSCOPE_TRACKS, SJ_SELECTOP_ALL);
	s.AddRule(SJ_FIELD_RATING, SJ_FIELDOP_IS_EQUAL_TO, wxT("1"));
	NewSearch(&s);

	s.Init(_("Top rated"), SJ_SELECTSCOPE_TRACKS, SJ_SELECTOP_ALL);
	s.AddRule(SJ_FIELD_RATING, SJ_FIELDOP_IS_GREATER_THAN, wxT("1"));
	s.AddRule(SJ_PSEUDOFIELD_LIMIT, SJ_FIELDOP_IS_EQUAL_TO, wxT("50"), SJ_FIELD_RATING|SJ_FIELDFLAG_DESC, SJ_UNIT_TRACKS);
	NewSearch(&s);

	s.Init(_("Top 20"), SJ_SELECTSCOPE_TRACKS, SJ_SELECTOP_ALL);
	s.AddRule(SJ_FIELD_TIMESPLAYED, SJ_FIELDOP_IS_GREATER_THAN, wxT("1"));
	s.AddRule(SJ_PSEUDOFIELD_LIMIT, SJ_FIELDOP_IS_EQUAL_TO, wxT("20"), SJ_FIELD_TIMESPLAYED|SJ_FIELDFLAG_DESC, SJ_UNIT_TRACKS);
	NewSearch(&s);

	s.Init(_("Recently played"), SJ_SELECTSCOPE_TRACKS, SJ_SELECTOP_ALL);
	s.AddRule(SJ_FIELD_LASTPLAYED, SJ_FIELDOP_IS_IN_THE_LAST, wxT("3"), wxT(""), SJ_UNIT_DAYS);
	NewSearch(&s);

	s.Init(_("Played today"), SJ_SELECTSCOPE_TRACKS, SJ_SELECTOP_ALL);
	s.AddRule(SJ_FIELD_LASTPLAYED, SJ_FIELDOP_IS_EQUAL_TO, wxT("today"));
	NewSearch(&s);

	s.Init(_("Random selection"), SJ_SELECTSCOPE_ALBUMS, SJ_SELECTOP_ALL);
	s.AddRule(SJ_PSEUDOFIELD_LIMIT, SJ_FIELDOP_IS_EQUAL_TO, wxT("5"), SJ_PSEUDOFIELD_RANDOM, SJ_UNIT_ALBUMS);
	NewSearch(&s);

	s.Init(_("Tracks in queue"), SJ_SELECTSCOPE_TRACKS, SJ_SELECTOP_ALL);
	s.AddRule(SJ_PSEUDOFIELD_QUEUEPOS, SJ_FIELDOP_IS_SET);
	NewSearch(&s);
}


SjAdvSearch SjAdvSearchModule::GetRecentSearch(bool getRules) const
{
	SjAdvSearch ret = GetSearchById(m_recentSearchId, getRules);
	if( ret.m_id == 0 )
	{
		ret = GetSearchByIndex(0, getRules);
	}
	return ret;
}


wxString SjAdvSearchModule::CheckName(long skipId, const wxString& name) const
{
	wxString    base(name),
	            testName;
	SjAdvSearch testSearch;
	int         numToAdd = 1, i, iCount = GetSearchCount();
	bool        nameOk;


	// make sure, the name is empty
	if( SjNormaliseString(name, 0).IsEmpty() )
	{
		base = _("Untitled Music selection");
	}

	// remove any numbers added to the string
	if( base.Find('(') != -1 )
	{
		testName = base.AfterLast('(').BeforeLast(')');
		long testl;
		if( testName.ToLong(&testl, 10)
		        && testl > 1 )
		{
			base = base.BeforeLast('(').Trim();
		}
	}

	// add a number to the name, if needed
	do
	{
		testName = numToAdd>1? wxString::Format(wxT("%s (%i)"), base.c_str(), (int)numToAdd) : base;
		nameOk = TRUE;

		for( i = 0; i < iCount; i++ )
		{
			testSearch = GetSearchByIndex(i, FALSE/*rules not needed*/);
			if( testSearch.m_id != skipId
			 && SjNormaliseString(testSearch.m_name, 0).Cmp(SjNormaliseString(testName, 0)) == 0 )
			{
				nameOk = FALSE;
				break;
			}
		}

		numToAdd++;
	}
	while( !nameOk );

	return testName;
}


SjAdvSearch SjAdvSearchModule::NewSearch(const SjAdvSearch* pattern)
{
	SjAdvSearch newSearch;
	if( pattern )
	{
		newSearch = *pattern; // this also copies the ID and the name which are overwritten in the next steps
		newSearch.m_name = CheckName(0, pattern->m_name);
	}
	else
	{
		newSearch.AddRule();
		newSearch.m_name = CheckName(0, wxT(""));
	}

	wxSqlt sql;
	sql.Query(wxT("INSERT INTO advsearch (name) VALUES ('") + sql.QParam(newSearch.m_name) + wxT("');"));
	newSearch.m_id = sql.GetInsertId();

	SaveSearch(newSearch);

	return newSearch;
}


void SjAdvSearchModule::SaveSearch(const SjAdvSearch& search)
{
	#if SJ_USE_PENDING_MUSIC_SEL
		if( m_pendingSearch.m_id != search.m_id )
		{
			Flush();
		}

		m_pendingSearch = search;
		m_pendingNeedsSave = true;
	#else
		wxBusyCursor busy;
		wxSqlt sql;

		SjStringSerializer ser;
		search.Serialize(ser);

		sql.Query(wxT("UPDATE advsearch SET name='") + sql.QParam(search.m_name) + wxT("', rules='") + sql.QParam(ser.GetResult()) + wxT("' WHERE id=") + sql.LParam(search.m_id) + wxT(";"));
	#endif

	// inform other modues about the new configuration
	m_interface->m_moduleSystem->BroadcastMsg(IDMODMSG_ADV_SEARCH_CONFIG_CHANGED);
}


#if SJ_USE_PENDING_MUSIC_SEL
void SjAdvSearchModule::Flush()
{
	if( m_pendingNeedsSave )
	{
		wxASSERT( m_pendingSearch.m_id != 0 );

		wxBusyCursor busy;
		wxSqlt sql;

		SjStringSerializer ser;
		m_pendingSearch.Serialize(ser);

		sql.Query(wxT("UPDATE advsearch SET name='") + sql.QParam(m_pendingSearch.m_name) + wxT("', rules='") + sql.QParam(ser.GetResult()) + wxT("' WHERE id=") + sql.LParam(m_pendingSearch.m_id) + wxT(";"));

		m_pendingNeedsSave = false;
	}
}
#endif


void SjAdvSearchModule::RenameSearch(SjAdvSearch& search, const wxString& newName)
{
	search.m_name = CheckName(search.m_id, newName);

	SaveSearch(search);
}


long SjAdvSearchModule::GetSearchIndex(const SjAdvSearch& search) const
{
	long index = 0;
	wxSqlt sql;
	sql.Query(wxT("SELECT id FROM advsearch ORDER BY SORTABLE(name);"));
	while( sql.Next() )
	{
		if( sql.GetLong(0) == search.m_id )
		{
			return index;
		}

		index++;
	}
	return -1;
}


void SjAdvSearchModule::DeleteSearch(const SjAdvSearch& search)
{
	wxBusyCursor busy;

	// delete the search
	long index = GetSearchIndex(search);
	wxSqlt sql;
	sql.Query(wxString::Format(wxT("DELETE FROM advsearch WHERE id=%i;"), (int)search.m_id));

	#if SJ_USE_PENDING_MUSIC_SEL
		if( m_pendingSearch.m_id == search.m_id )
		{
			m_pendingSearch.m_id = 0;
			m_pendingNeedsSave = false;
		}
	#endif

	// re-add the predefined searches if the list of searches is empty
	if( GetSearchCount() == 0 )
	{
		AddPredefinedSearches();
	}

	// select the remaining item at the same index
	if( index >= GetSearchCount() )
	{
		index--;
	}

	m_recentSearchId = GetSearchByIndex(index, FALSE).m_id;

	// done
	CheckHistory();

	// inform other modues about the new configuration
	m_interface->m_moduleSystem->BroadcastMsg(IDMODMSG_ADV_SEARCH_CONFIG_CHANGED);
}


wxString SjAdvSearchModule::FormatResultString(long count, const wxString& advSearchName)
{
	wxString ret;
	if( !advSearchName.IsEmpty()
	 && (g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL) || g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE)) )
	{
		if( count == 0 )
		{
			ret = wxString::Format(_("No matches in \"%s\""), advSearchName.c_str());
		}
		else if( count == 1 )
		{
			ret = wxString::Format(_("1 match in \"%s\""), advSearchName.c_str());
		}
		else
		{
			ret = wxString::Format(_("%s matches in \"%s\""), SjTools::FormatNumber(count).c_str(), advSearchName.c_str());
		}
	}
	else
	{
		if( count == 0 )
		{
			ret = _("No matches");
		}
		else if( count == 1 )
		{
			ret = _("1 match");
		}
		else
		{
			ret = wxString::Format(_("%s matches"), SjTools::FormatNumber(count).c_str());
		}
	}
	return ret;
}


/*******************************************************************************
 * SjAdvSearchModule - handling "little" options
 ******************************************************************************/


void SjAdvSearchModule::GetLittleOptions(SjArrayLittleOption& lo)
{
	SjLittleOption::SetSection(_("Search"));

	#define SEP wxString(wxT("|"))
	lo.Add( new SjLittleBit(_("Search single words"), wxT("yn"), &m_flags, 1L /*default:on*/, SJ_SEARCHFLAG_SEARCHSINGLEWORDS, wxT("advsearch/flags")));
	lo.Add( new SjLittleBit(_("Lookup genre on simple search"), wxT("yn"), &m_flags, 0L /*default:off*/, SJ_SEARCHFLAG_SIMPLEGENRELOOKUP, wxT("advsearch/flags")));
	lo.Add( new SjLittleBit(_("Search while typing"), wxT("yn"), &m_flags, 1L /*default: on*/, SJ_SEARCHFLAG_SEARCHWHILETYPING, wxT("advsearch/flags")));
	lo.Add( new SjLittleBit(_("Save modified music selections"), _("Ask") + SEP + _("Yes") + SEP + _("No"), &m_saveSearches, SJ_SAVESEARCHES_ASK, 0L, wxT("advsearch/saveSearches")));

	wxString options;
	int i;
	for( i = 0; i <= 25; i++ )
	{
		if( i ) options += SEP;
		options += wxString::Format(wxT("%i"), i);
	}
	lo.Add( new SjLittleBit(_("Max. history size"), options, &m_maxHistorySize, SJ_DEFAULT_HISTORY_SIZE, 0L, wxT("advsearch/maxHistorySize")));

	SjLittleOption::ClearSection();
	#undef SEP
}


void SjAdvSearchModule::UpdateAdvSearchChoice(wxChoice* choice, long searchIdToSelect,
        const wxString& format, const wxString& specialForId0)
{
	wxASSERT( format.Find(wxT("%s"))!=-1 );

	// get currently selected search, if any
	int i = choice->GetSelection();
	if( i != -1 )
	{
		searchIdToSelect = (long)choice->GetClientData(i);
	}

	// check the selected id
	if( (searchIdToSelect!=0 || specialForId0.IsEmpty())
	        && g_advSearchModule->GetSearchById(searchIdToSelect, FALSE).GetId()==0 )
	{
		searchIdToSelect = g_advSearchModule->GetRecentSearch(FALSE).GetId();
	}

	// init choice control
	SjAdvSearch advSearch;
	choice->Clear();

	// add "special"
	if( !specialForId0.IsEmpty() )
	{
		choice->Append(specialForId0, (void*)0);
		if( 0 == searchIdToSelect )
		{
			choice->SetSelection(0);
		}
	}

	// add all music selections
	int iCount = g_advSearchModule->GetSearchCount();
	for( i = 0; i < iCount; i++ )
	{
		advSearch = g_advSearchModule->GetSearchByIndex(i, FALSE);
		choice->Append(wxString::Format(format, advSearch.GetName().c_str()), (void*)advSearch.GetId());
		if( advSearch.GetId() == searchIdToSelect )
		{
			choice->SetSelection(choice->GetCount()-1);
		}
	}
}


/*******************************************************************************
 * SjAdvSearchModule - handling the history
 ******************************************************************************/


SjSearchHistory::SjSearchHistory(const wxString& simpleSearchWords, long advSearchId, const wxString& advSearchRules)
{
	m_simpleSearchWords =   simpleSearchWords;
	m_advSearchId       =   advSearchId;
	m_advSearchRules    =   advSearchRules;
}


SjSearchHistory::SjSearchHistory(const SjSearch& s)
{
	m_simpleSearchWords =   s.m_simple.GetWords();
	m_advSearchId       =   s.m_adv.GetId();
	if( m_advSearchId == 0
	 && s.m_adv.IsSet() )
	{
		SjStringSerializer ser;
		s.m_adv.Serialize(ser);
		m_advSearchRules = ser.GetResult();
	}
}


wxString SjSearchHistory::GetTitle() const
{
	// get shortened simple search string
	wxString simpleSearch = m_simpleSearchWords;
	if( simpleSearch.Length() > 22 )
	{
		simpleSearch = simpleSearch.Left(20).Trim() + wxT("..");
	}

	// do we have an additional advanced search in the history object?
	if( (m_advSearchId || !m_advSearchRules.IsEmpty())
	 && (g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL) || g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE)) )
	{
		SjSearch savedSearch = GetSearch(FALSE);
		wxString advSearch = savedSearch.m_adv.GetName();
		if( !advSearch.IsEmpty() )
		{
			// ...yes... shorten adv. search name
			if( advSearch.Length() > 22 )
			{
				advSearch = advSearch.Left(20).Trim() + wxT("..");
			}

			if( simpleSearch.IsEmpty() )
			{
				return wxString::Format(_("Search in \"%s\""), advSearch.c_str());
			}
			else
			{
				return wxString::Format(_("Search for \"%s\" in \"%s\""), simpleSearch.c_str(), advSearch.c_str());
			}
		}
	}

	// ...no
	if( !simpleSearch.IsEmpty() )
	{
		return wxString::Format(_("Search for \"%s\""), simpleSearch.c_str());
	}
	else
	{
		return wxT("");
	}
}


SjSearch SjSearchHistory::GetSearch(bool getRules) const
{
	SjSearch ret(m_simpleSearchWords);

	if( m_advSearchId && g_advSearchModule )
	{
		ret.m_adv = g_advSearchModule->GetSearchById(m_advSearchId, getRules);
	}
	else if( !m_advSearchRules.IsEmpty() )
	{
		SjStringSerializer ser(m_advSearchRules);
		ret.m_adv.Unserialize(ser);
	}

	return ret;
}


void SjAdvSearchModule::LoadHistory()
{
	if( !m_historyLoaded )
	{
		m_historyLoaded = TRUE;

		SjStringSerializer ser(g_tools->m_config->Read(wxT("advsearch/history2"), wxT("")));

		int         i, iCount = ser.GetLong();
		wxString    stringToAdd;
		long        advSearchIdToAdd;
		wxString    advSearchRulesToAdd;
		if( iCount > 0 && iCount <= 100 && !ser.HasErrors() )
		{
			for(i = 0; i < iCount; i++ )
			{
				stringToAdd = ser.GetString();
				advSearchIdToAdd = ser.GetLong();
				advSearchRulesToAdd = ser.GetString();
				if( !ser.HasErrors() )
				{
					m_history.Add(new SjSearchHistory(stringToAdd, advSearchIdToAdd, advSearchRulesToAdd));
				}
			}
		}
	}
}


void SjAdvSearchModule::CheckHistory()
{
	// called if a search is deleted

	LoadHistory();

	size_t i;
	long   advSearchId;
	for( i = 0; i < m_history.GetCount(); i++ )
	{
		advSearchId = m_history[i].GetAdvSearchId();
		if( GetSearchById(advSearchId, FALSE).m_id == 0 )
		{
			m_history.RemoveAt(i);
			i--;
		}
	}

	SaveHistory();
}


void SjAdvSearchModule::SaveHistory()
{
	if( m_needsHistorySave )
	{
		SjStringSerializer ser;
		long i, iCount = (long)m_history.GetCount();

		ser.AddLong(iCount);

		for( i = 0; i < iCount; i++ )
		{
			ser.AddString(m_history[i].GetSimpleSearchWords());
			ser.AddLong(m_history[i].GetAdvSearchId());
			ser.AddString(m_history[i].GetAdvSearchRules());
		}

		g_tools->m_config->Write(wxT("advsearch/history2"), ser.GetResult());

		m_needsHistorySave = FALSE;
	}
}


long SjAdvSearchModule::GetHistoryCount()
{
	LoadHistory();
	while( (long)m_history.GetCount() > m_maxHistorySize ) // feedback of changes in m_maxHistorySize;
	{	// normally the size is checked in AddToHistory()
		m_history.RemoveAt(0);
	}
	return m_history.GetCount();
}


const SjSearchHistory* SjAdvSearchModule::GetHistory(long index)
{
	LoadHistory();
	return (index>=0 && index<(long)m_history.GetCount())? &(m_history[index]) : NULL;
}


void SjAdvSearchModule::AddToHistory(const SjSearch& searchToAdd)
{
	LoadHistory();

	// create a new history object
	SjSearchHistory* historyToAdd = new SjSearchHistory(searchToAdd);

	// try to remove some obsolete history entries
	int historyCount = (int)m_history.GetCount();
	if( historyCount )
	{
		// ...has the last entry the same beginnig and the same adv. search as the new entry?
		// In this case, we assume, the user has just added some characters
		// to the search, so remove the last item before continue
		SjSearchHistory* last = &(m_history.Last());
		if( historyToAdd->GetAdvSearchId() == last->GetAdvSearchId()
		 && historyToAdd->GetAdvSearchRules() == last->GetAdvSearchRules()
		 && historyToAdd->GetSimpleSearchWords().Left(last->GetSimpleSearchWords().Len()) == last->GetSimpleSearchWords()  )
		{
			m_history.RemoveAt(historyCount-1);
			historyCount--;
		}

		// ...does the new entry alreday exist in the history?
		// In this case, remove it before continue
		int i;
		for( i = 0; i < historyCount; i++ )
		{
			if( m_history[i] == *historyToAdd )
			{
				m_history.RemoveAt(i);
				historyCount--;
				i--; // try same index again
			}
		}

		// remove old items at the beginning of the history
		while( historyCount >= m_maxHistorySize )
		{
			m_history.RemoveAt(0);
			historyCount--;
		}
	}

	// add the new item to the end of the history
	// (new items are at the end!)
	m_history.Add(historyToAdd);
	m_needsHistorySave = TRUE;
}

