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
 * File:    busyinfo.cpp
 * Authors: Björn Petersen
 * Purpose: Our busy information and progress dialog
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/busyinfo.h>
#include <sjtools/msgbox.h>

#define UPDATE_INTERVALL_MS 100


BEGIN_EVENT_TABLE(SjBusyInfo, SjDialog)
	EVT_CLOSE(SjBusyInfo::OnCloseWindow)
	EVT_BUTTON(wxID_CANCEL, SjBusyInfo::OnCancel)
END_EVENT_TABLE()


SjBusyInfo*  SjBusyInfo::s_busyInfo = NULL;
long         SjBusyInfo::s_busyInfoUsage = 0;
SjMainFrame* SjBusyInfo::s_mainFrame = NULL;
long         SjBusyInfo::s_inYield = 0;
bool         SjBusyInfo::s_dlgOpen = FALSE;


SjBusyInfo::SjBusyInfo(wxWindow *parent, const wxString& title, bool canCancel, const wxString& questionOnCancel)
	: SjDialog(parent, title, SJ_MODAL, SJ_RESIZEABLE_IF_POSSIBLE)
{
	// check if there is already a busy info dialog
	s_busyInfoUsage++;
	if( s_busyInfoUsage > 1 )
	{
		wxASSERT(s_busyInfo);
		wxASSERT(s_dlgOpen);

		s_busyInfo->Set(title, TRUE);
		return;
	}

	s_dlgOpen = TRUE;
	s_busyInfo = this;

	// init dialog
	m_canCancel         = canCancel;
	m_cancelPressed     = FALSE;
	m_questionOnCancel  = questionOnCancel;
	m_objectCount       = 0;
	m_nextUpdateMs      = 0;
	m_startTimeMs       = SjTools::GetMsTicks();
	m_elapsedSeconds    = 0;

	#define TEXT_W 80

	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	wxStaticBoxSizer* sizer2b = new wxStaticBoxSizer(new wxStaticBox(this, -1, wxEmptyString), wxVERTICAL);
	sizer1->Add(sizer2b, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	m_objectNameWindow = new wxStaticText(this, -1, title,
	                                      wxDefaultPosition, wxSize(TEXT_W, -1), wxALIGN_LEFT | wxST_NO_AUTORESIZE /*| wxBORDER*/);
	sizer2b->Add(m_objectNameWindow, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP
	             , SJ_DLG_SPACE);

	sizer2b->Add(2, 2);

	wxFlexGridSizer* sizer3 = new wxFlexGridSizer(2, 2, 2/*hgap*/, SJ_DLG_SPACE/*vgap*/);
	sizer3->AddGrowableCol(1);
	sizer2b->Add(sizer3, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	wxStaticText* staticText = new wxStaticText(this, -1, _("Object count:"));
	sizer3->Add(staticText, 0, 0, 0);

	m_objectCountWindow = new wxStaticText(this, -1, wxT("0"),
	                                       wxDefaultPosition, wxSize(TEXT_W, -1), wxALIGN_LEFT | wxST_NO_AUTORESIZE /*| wxBORDER*/);
	sizer3->Add(m_objectCountWindow, 1, wxGROW, 0);

	staticText = new wxStaticText(this, -1, _("Elapsed time:"));
	sizer3->Add(staticText, 0, 0, 0);

	m_elapsedTimeWindow = new wxStaticText(this, -1, SjTools::FormatTime(0, SJ_FT_ALLOW_ZERO),
	                                       wxDefaultPosition, wxSize(TEXT_W, -1), wxALIGN_LEFT | wxST_NO_AUTORESIZE /*| wxBORDER*/);
	sizer3->Add(m_elapsedTimeWindow, 1, wxGROW, 0);

	sizer2b->Add(2, 2);

	if( canCancel )
	{
		wxButton* button = new wxButton(this, wxID_CANCEL,  _("Cancel"));
		sizer1->Add(button, 0, wxALIGN_RIGHT|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);
	}

	// init done, center dialog
	sizer1->SetSizeHints(this);
	SetSize(-1, -1, 500,  -1);
	CentreOnParent();

	// show and disable parent
	if( parent )
	{
		parent->Enable(FALSE);
	}

	Show(TRUE);
	#ifdef __WXMAC__
		Update();
	#else
		Refresh();
	#endif
	Yield();
}


SjBusyInfo::~SjBusyInfo()
{
	// is there still a busy info dialog?
	s_busyInfoUsage--;
	if( s_busyInfoUsage > 0 )
	{
		return;
	}

	wxASSERT(s_busyInfo==this);
	s_busyInfo = NULL;

	// wait at a little moment if the dialog will disappear too fast
	#define LITTE_MOMENT_MS 600
	while( SjTools::GetMsTicks() < m_startTimeMs+LITTE_MOMENT_MS )
	{
		Yield();
		wxMilliSleep(LITTE_MOMENT_MS/10);
	}

	// hide and enable parent
	wxWindow* parent = GetParent();
	if( parent )
	{
		parent->Enable(TRUE);
	}

	m_cancelPressed = TRUE;
	Show(FALSE);
	Close();
	Yield();

	s_dlgOpen = FALSE;
}


void SjBusyInfo::AskToClose()
{
	if( m_canCancel )
	{
		if( m_questionOnCancel.IsEmpty()
		        || ::SjMessageBox(m_questionOnCancel, GetTitle(), wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION, this) == wxYES )
		{
			m_cancelPressed = TRUE;
		}
	}
}


void SjBusyInfo::OnCancel(wxCommandEvent& event)
{
	if( !m_cancelPressed )
	{
		AskToClose();
	}
}


void SjBusyInfo::OnCloseWindow(wxCloseEvent& event)
{
	if( !m_cancelPressed )
	{
		AskToClose();
		event.Veto();
	}
}


bool SjBusyInfo::Set__(const wxString& object, bool forceUpdateForLongOp)
{
	static bool inSet = FALSE;

	if( !inSet )
	{
		inSet = TRUE;

		unsigned long currMs = SjTools::GetMsTicks();
		if( !object.IsEmpty() )
		{
			m_objectCount++;
			wxLogDebug(wxT("m_objectCount=%i on SjBusyInfo::Set__(%s)"), m_objectCount, object.c_str());

			if( forceUpdateForLongOp || currMs >= m_nextUpdateMs )
			{
				m_objectNameWindow->SetLabel(/*SjTools::ShortenUrl*/(object));
				m_objectCountWindow->SetLabel(SjTools::FormatNumber(m_objectCount));
				m_nextUpdateMs = currMs + UPDATE_INTERVALL_MS;
			}
		}

		unsigned long elapsedSeconds = (currMs-m_startTimeMs)/1000;
		if( elapsedSeconds != m_elapsedSeconds )
		{
			m_elapsedTimeWindow->SetLabel(SjTools::FormatTime(elapsedSeconds, SJ_FT_ALLOW_ZERO));
			m_elapsedSeconds = elapsedSeconds;
		}

		Yield();

		inSet = FALSE;
	}

	return !m_cancelPressed;
}


bool SjBusyInfo::Set(const wxString& object, bool forceUpdateForLongOp)
{
	if( s_busyInfo )
	{
		return s_busyInfo->Set__(object, forceUpdateForLongOp);
	}
	else if( s_mainFrame && !object.IsEmpty() )
	{
		return s_mainFrame->DisplayBusyInfo(object, forceUpdateForLongOp);
	}
	else
	{
		return TRUE;
	}
}


void SjBusyInfo::SetMainFrame(SjMainFrame* mainFrame)
{
	s_mainFrame = mainFrame;
}

