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
 *  File:    sj_console.cpp
 *  Authors: Björn Petersen
 *  Purpose: Logging that may be used to catch errors in threads
 *
 *******************************************************************************
 *
 *  Please check _ALL_ external pointers as logging may occur very soon
 *  or very late!
 *
 *******************************************************************************
 *
 *  While the logging functions as wxLogError() or wxLogWarning() themselves
 *  are threadsave, wxLog::SetActiveTarget() isn't.  However, we need this
 *  functionality inside the image thread as we want to show any loading
 *  errors in place of the errorous image instead than popping up a message
 *  box.  So, SjLogGui should used globally instead of wxLogGui.  Most stuff
 *  is identical to wxLogGui, but the helper class SjLogString may be used
 *  to catch errors as strings even inside threads.
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/textfile.h>
#include <wx/artprov.h>
#include <sjtools/console.h>
#include <see_dom/sj_see.h>
#include <sjtools/msgbox.h>


// this function is a wrapper around strftime(3)
// allows to exclude the usage of wxDateTime
static wxString TimeStamp(const wxChar *format, time_t t)
{
	wxChar buf[4096];
	if ( !wxStrftime(buf, WXSIZEOF(buf), format, localtime(&t)) )
	{
		// buffer is too small?
		wxFAIL_MSG(_T("strftime() failed"));
	}
	return wxString(buf);
}


/*******************************************************************************
 * SjLogListCtrl
 ******************************************************************************/


class SjLogListCtrl : public wxListCtrl
{
public:
	                SjLogListCtrl       (SjLogGui* logGui, wxWindow* parent, wxWindowID id, long aIndex);
	void            MessagesChanged     (long firstNewIndex);
	void            SizeChanged         ();

	static int      GetSeverityImage    (long severity);
	static wxString GetSeverityChar     (long severity);

private:
	wxString        OnGetItemText       (long item, long column) const;
	int             OnGetItemImage      (long item) const;

	SjLogGui*       m_logGui;
	void            ScrollDownTo        (long aIndex);
};


SjLogListCtrl::SjLogListCtrl(SjLogGui* logGui, wxWindow* parent, wxWindowID id, long aIndex)
	: wxListCtrl(parent, id, wxDefaultPosition, wxSize(-1, 160), wxLC_REPORT | wxLC_VIRTUAL /*| wxLC_NO_HEADER*/)
{
	m_logGui = logGui;

	// assign the imagelist
	wxImageList* imageList = new wxImageList(16, 16);
	imageList->Add( wxArtProvider::GetBitmap(wxART_ERROR,       wxART_MESSAGE_BOX, wxSize(16, 16)) );
	imageList->Add( wxArtProvider::GetBitmap(wxART_WARNING,     wxART_MESSAGE_BOX, wxSize(16, 16)) );
	imageList->Add( wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MESSAGE_BOX, wxSize(16, 16)) );
	AssignImageList(imageList, wxIMAGE_LIST_SMALL);

	// init control
	InsertColumn(0, _("Message"));
	InsertColumn(1, _("Scope"));
	InsertColumn(2, _("Time"));

	SetItemCount(logGui->m_aPacked.GetCount());

	ScrollDownTo(aIndex);
}


wxString SjLogListCtrl::OnGetItemText(long index, long column) const
{
	unsigned long severity, time;
	wxString      msg, scope;
	SjLogGui::ExplodeMessage(m_logGui->m_aPacked[index], severity, time, msg, scope);

	if( column == 0 )
	{
		return SjLogGui::SingleLine(msg);
	}
	else if( column == 1 )
	{
		return scope;
	}
	else
	{
		wxString fmt = wxLog::GetTimestamp();
		if ( !fmt )
		{
			// default format
			fmt = _T("%c");
		}
		return TimeStamp(fmt, time);
	}
}


int SjLogListCtrl::OnGetItemImage(long index) const
{
	unsigned long severity, time;
	wxString      msg, scope;
	SjLogGui::ExplodeMessage(m_logGui->m_aPacked[index], severity, time, msg, scope);
	return GetSeverityImage(severity);
}


int SjLogListCtrl::GetSeverityImage(long severity)
{
	switch ( severity )
	{
		case wxLOG_Error:       return 0;
		case wxLOG_Warning:     return 1;
		default:     			return 2; // Message/Info/Verbose etc.
	}
}


wxString SjLogListCtrl::GetSeverityChar(long severity)
{
	switch ( severity )
	{
		case wxLOG_Error:       return wxT("Error"); /*do not translate as this makes it possible to use the logs from external apps*/
		case wxLOG_Warning:     return wxT("Warning");
		case wxLOG_Message:     return wxT("Message");
		default:                return wxT("Info");
	}
}


void SjLogListCtrl::MessagesChanged(long firstNewIndex)
{
	long newCount = m_logGui->m_aPacked.GetCount();
	SetItemCount(newCount);
	SizeChanged();
	ScrollDownTo(newCount-1);
	Refresh();
}


void SjLogListCtrl::ScrollDownTo(long aIndex)
{
	long count = m_logGui->m_aPacked.GetCount();
	if( count > 0 )
	{
		if( aIndex < 0 || aIndex >= count ) {
			aIndex = count-1;
		}
		EnsureVisible(aIndex);
	}
}


void SjLogListCtrl::SizeChanged()
{
	wxSize size = GetClientSize();
	int    charW = GetCharWidth(); // Returns the average character width for this window.
	int    w1 = charW*10;
	int    w2 = charW*10;

	SetColumnWidth(0, size.x-w1-w2);
	SetColumnWidth(1, w1);
	SetColumnWidth(2, w2);
}


/*******************************************************************************
 * SjLogDialog
 ******************************************************************************/


class SjLogDialog : public SjDialog
{
public:
	                    SjLogDialog         (SjLogGui* logGui, wxWindow *parent, long style, long aIndex);
	                    ~SjLogDialog        ();

	void                MessagesChanged     (long firstNewIndex);

	SjLogGui*           m_logGui;

	#if SJ_USE_SCRIPTS
	SjSee               m_see;
	#endif

	wxStaticBitmap*     m_icon;
	bool                m_iconIsInfo;
	wxStaticText*       m_msg;
	wxSizer*            m_sizerAddRemoveParent;
	wxSizer*            m_sizerDetails;
	bool                m_sizerDetailsAttached;
	SjLogListCtrl*      m_listCtrl;
	#if SJ_USE_SCRIPTS
	wxTextCtrl*         m_evalInput;
	wxWindow*           m_evalButton;
	#endif
	wxWindow*           m_clearButton;
	wxWindow*           m_saveButton;
	wxCheckBox*         m_autoOpenCheckBox;
	bool                m_showingDetails;

	void                ShowDetails         (bool show);
	void                DoClose             ();

	int                 OpenLogFile         (wxFile& file, wxString& retFilename);

	void                OnOk                (wxCommandEvent& e) { DoClose(); }
	void                OnDetails           (wxCommandEvent& e);
	#if SJ_USE_SCRIPTS
	void                OnEval              (wxCommandEvent& e);
	#endif
	void                OnClear             (wxCommandEvent& e);
	void                OnSave              (wxCommandEvent& e);
	void                OnAutoOpen          (wxCommandEvent& e) { SjLogGui::SetAutoOpen(m_autoOpenCheckBox->GetValue()!=0); }
	void                OnSize              (wxSizeEvent& event) { SjDialog::OnSize(event); if(m_listCtrl) { m_listCtrl->SizeChanged(); } }
	void                OnClose             (wxCloseEvent&) { DoClose(); }

	DECLARE_EVENT_TABLE()
};


#define IDC_DETAILS_BUTTON       IDC_BUTTONBARMENU
#define IDC_LIST_CTRL           (IDM_FIRSTPRIVATE+3)
#if SJ_USE_SCRIPTS
#define IDC_EVAL_INPUT          (IDM_FIRSTPRIVATE+4)
#define IDC_EVAL_BUTTON         (IDM_FIRSTPRIVATE+5)
#endif
#define IDC_CLEAR_BUTTON        (IDM_FIRSTPRIVATE+6)
#define IDC_SAVE_BUTTON         (IDM_FIRSTPRIVATE+7)
#define IDC_AUTO_OPEN_CHECKBOX  (IDM_FIRSTPRIVATE+8)


BEGIN_EVENT_TABLE(SjLogDialog, SjDialog)
	EVT_BUTTON      (wxID_OK,               SjLogDialog::OnOk       )
	EVT_BUTTON      (IDC_DETAILS_BUTTON,    SjLogDialog::OnDetails  )
	#if SJ_USE_SCRIPTS
	EVT_TEXT_ENTER  (IDC_EVAL_INPUT,        SjLogDialog::OnEval     )
	EVT_BUTTON      (IDC_EVAL_BUTTON,       SjLogDialog::OnEval     )
	#endif
	EVT_BUTTON      (IDC_CLEAR_BUTTON,      SjLogDialog::OnClear    )
	EVT_BUTTON      (IDC_SAVE_BUTTON,       SjLogDialog::OnSave     )
	EVT_CHECKBOX    (IDC_AUTO_OPEN_CHECKBOX,SjLogDialog::OnAutoOpen )
	EVT_SIZE        (                       SjLogDialog::OnSize     )
	EVT_CLOSE       (                       SjLogDialog::OnClose    )
END_EVENT_TABLE()


static SjLogDialog* s_dlg = NULL;


SjLogDialog::SjLogDialog(SjLogGui* logGui,
                         wxWindow *parent,
                         long style,
                         long aIndex)
	: SjDialog(parent, _("Console"),
	           parent? SJ_MODELESS : SJ_MODAL,
	           SJ_RESIZEABLE_IF_POSSIBLE)
{
	m_logGui = logGui;
	s_dlg = this;

	// create dialog
	m_listCtrl = NULL;
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	m_sizerAddRemoveParent = new wxBoxSizer(wxVERTICAL);
	sizer1->Add(m_sizerAddRemoveParent, 1, wxALL|wxGROW, SJ_DLG_SPACE);

	wxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	m_sizerAddRemoveParent->Add(sizer3, 0, 0, 0);

	// icon
	m_icon = NULL;
	m_iconIsInfo = false;
	if( (style & wxICON_MASK) )
	{
		wxBitmap bitmap;
		switch ( style & wxICON_MASK )
		{
			case wxICON_INFORMATION:    bitmap = wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MESSAGE_BOX);  m_iconIsInfo = true; break;
			case wxICON_WARNING:        bitmap = wxArtProvider::GetIcon(wxART_WARNING, wxART_MESSAGE_BOX);      break;
			default:                    bitmap = wxArtProvider::GetIcon(wxART_ERROR, wxART_MESSAGE_BOX);        break;
		}
		m_icon = new wxStaticBitmap(this, -1, bitmap);
		sizer3->Add(m_icon, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, SJ_DLG_SPACE);
	}

	// text
	wxString msg;
	if( aIndex < 0 || aIndex >= (long)logGui->m_aPacked.GetCount() )
	{
		msg = wxString::Format(
		  // TRANSLATORS: %i will be replaced by a number
		  wxPLURAL("%i message", "%i messages", m_logGui->m_aPacked.GetCount()),
		  (int)m_logGui->m_aPacked.GetCount());
		m_showingDetails = true;
	}
	else
	{
		unsigned long severity, time;
		wxString scope;
		SjLogGui::ExplodeMessage(logGui->m_aPacked[aIndex], severity, time, msg, scope);
		m_showingDetails = false;
	}

	m_msg = new wxStaticText(this, -1, msg);
	m_msg->Wrap(SJ_MSG_BOX_WRAP);
	sizer3->Add(m_msg, 0, wxALL|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	m_sizerDetails = new wxBoxSizer(wxVERTICAL);
	m_sizerAddRemoveParent->Add(m_sizerDetails, 1, wxGROW|wxTOP, SJ_DLG_SPACE);
	m_sizerDetailsAttached = true;

	// list control ( added if needed )
	m_listCtrl = new SjLogListCtrl(logGui, this, IDC_LIST_CTRL, aIndex);
	m_sizerDetails->Add(m_listCtrl, 1, wxGROW|wxTOP, SJ_DLG_SPACE);

	// evaluate etc. ( added if needed )
	wxSizer* sizer4 = new wxBoxSizer(wxHORIZONTAL);
	m_sizerDetails->Add(sizer4, 0, wxGROW|wxTOP, SJ_DLG_SPACE/2);

	#if SJ_USE_SCRIPTS
	m_evalInput = new wxTextCtrl(this, IDC_EVAL_INPUT, wxT(""), wxDefaultPosition, wxSize(300, -1), wxTE_PROCESS_ENTER);
	sizer4->Add(m_evalInput, 1, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE/2);
	#else
	m_autoOpenCheckBox = new wxCheckBox(this, IDC_AUTO_OPEN_CHECKBOX, _("Open console on errors and warnings"));
	m_autoOpenCheckBox->SetValue(SjLogGui::GetAutoOpen());
	sizer4->Add(m_autoOpenCheckBox, 1, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE*8);
	#endif

	#define TOOLBAR_BUTTON(id, name) new wxButton(this, id, name, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT)

	#if SJ_USE_SCRIPTS
	m_evalButton = TOOLBAR_BUTTON(IDC_EVAL_BUTTON, _("Evaluate"));
	sizer4->Add(m_evalButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE/2);
	#endif

	m_clearButton = TOOLBAR_BUTTON(IDC_CLEAR_BUTTON, _("Clear"));
	sizer4->Add(m_clearButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE/2);

	m_saveButton = TOOLBAR_BUTTON(IDC_SAVE_BUTTON, _("Save"));
	sizer4->Add(m_saveButton, 0, wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	#ifndef SJ_USE_SCRIPTS
	m_autoOpenCheckBox = new wxCheckBox(this, IDC_AUTO_OPEN_CHECKBOX, _("Open console on errors and warnings"));
	m_autoOpenCheckBox->SetValue(SjLogGui::GetAutoOpen());
	m_sizerDetails->Add(m_autoOpenCheckBox, 0, wxTOP, SJ_DLG_SPACE);
	#endif

	// buttons
	sizer1->Add(CreateButtons(SJ_DLG_MENU|SJ_DLG_OK, _("Close")),
	            0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	ShowDetails(m_showingDetails);

	sizer1->SetSizeHints(this);

	FindWindow(wxID_OK)->SetFocus();

	m_listCtrl->SizeChanged();
	CenterOnParent();
}


void SjLogDialog::MessagesChanged(long firstNewIndex)
{
	if( m_icon && !m_iconIsInfo )
	{
		wxBitmap bitmap = wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MESSAGE_BOX);
		m_icon->SetBitmap(bitmap);
		m_iconIsInfo = true;
	}

	m_msg->SetLabel(wxString::Format(
	  // TRANSLATORS: %i will be replaced by a number
	  wxPLURAL("%i message", "%i messages", m_logGui->m_aPacked.GetCount()),
	  (int)m_logGui->m_aPacked.GetCount()));
	Layout();

	m_listCtrl->MessagesChanged(firstNewIndex);
}


SjLogDialog::~SjLogDialog()
{
	if( s_dlg == this )
		s_dlg = NULL;

	if( !m_sizerDetailsAttached ) {
		delete m_sizerDetails;
	}
}


void SjLogDialog::ShowDetails(bool show)
{
	wxButton* detailsButton = (wxButton*)FindWindow(IDC_DETAILS_BUTTON);
	m_listCtrl->Show(show);
	#if SJ_USE_SCRIPTS
	m_evalInput->Show(show);
	m_evalButton->Show(show);
	#endif
	m_clearButton->Show(show);
	m_saveButton->Show(show);
	m_autoOpenCheckBox->Show(show);

	if( show )
	{
		detailsButton->SetLabel(wxT("<< ") + wxString(_("Details")));

		if( !m_sizerDetailsAttached ) {
			m_sizerAddRemoveParent->Add(m_sizerDetails, 1, wxGROW|wxTOP, SJ_DLG_SPACE);
		}
		m_sizerDetailsAttached = true;
	}
	else
	{
		detailsButton->SetLabel(wxString(_("Details")) + wxT(" >>"));

		if( m_sizerDetailsAttached ) {
			m_sizerAddRemoveParent->Detach(m_sizerDetails);
		}
		m_sizerDetailsAttached = false;
	}
}


void SjLogDialog::OnDetails(wxCommandEvent& event)
{
	m_showingDetails = !m_showingDetails;
	ShowDetails(m_showingDetails);

	// in any case, our size changed - relayout everything and set new hints
	// ---------------------------------------------------------------------

	// we have to reset min size constraints or Fit() would never reduce the
	// dialog size when collapsing it and we have to reset max constraint
	// because it wouldn't expand it otherwise

	m_minHeight = m_maxHeight = -1;

	// wxSizer::FitSize() is private, otherwise we might use it directly...
	wxSize sizeTotal = GetSize(),
	       sizeClient = GetClientSize();

	wxSize size = GetSizer()->GetMinSize();
	size.x += sizeTotal.x - sizeClient.x;
	size.y += sizeTotal.y - sizeClient.y;

	// we don't want to allow expanding the dialog in vertical direction as
	// this would show the "hidden" details but we can resize the dialog
	// vertically while the details are shown
	if ( !m_showingDetails ) {
		m_maxHeight = size.y;
	}

	SetSizeHints(size.x, size.y, m_maxWidth, m_maxHeight);

	// don't change the width when expanding/collapsing
	SetSize(wxDefaultCoord, size.y);

	#ifdef __WXGTK__
		// VS: this is necessary in order to force frame redraw under
		// WindowMaker or fvwm2 (and probably other broken WMs).
		// Otherwise, detailed list wouldn't be displayed.
		Show();
	#endif // wxGTK
}


#if SJ_USE_SCRIPTS
void SjLogDialog::OnEval(wxCommandEvent& event)
{
	// execute a simple macro
	wxString script = m_evalInput->GetValue();
	script.Trim(true);
	script.Trim(false);
	if( !script.IsEmpty() )
	{
		// create a "printable" excerpt of the script
		wxString scriptShortened(script);
		if( scriptShortened.Len()>110 ) {
			scriptShortened = scriptShortened.Left(100).Trim()+wxT("..");
		}

		// prepare execute
		m_see.SetExecutionScope(_("Console"));
		size_t oldCount1 = m_logGui->m_aMessages.GetCount();
		size_t oldCount2 = m_logGui->m_aPacked.GetCount();

		// execute
		if( m_see.Execute(script) )
		{
			// print the result (if not empty or if nothng has been logged by Execute())
			bool     sthLogged = (oldCount1!=m_logGui->m_aMessages.GetCount() || oldCount2!=m_logGui->m_aPacked.GetCount());
			wxString result = m_see.GetResultString();
			if( !sthLogged || !result.IsEmpty() )
			{
				if( result.IsEmpty() ) {
					result = wxT("undefined");
				}
				else if( result.Len()>110 ) {
					result = result.Left(100).Trim()+wxT("..");
				}

				wxLogInfo(wxT("%s=%s [%s]"), scriptShortened.c_str(), result.c_str(), m_see.m_executionScope.c_str());
			}
		}
	}
}
#endif


void SjLogDialog::OnClear(wxCommandEvent& event)
{
	if( !m_logGui->m_aPacked.IsEmpty() )
	{
		wxWindowDisabler disabler(this);
		if( SjMessageBox(_("Clear all messages?"), SJ_PROGRAM_NAME, wxICON_QUESTION|wxYES_NO, this) == wxYES )
		{
			m_logGui->m_aPacked.Clear();
			MessagesChanged(-1);
		}
	}
}


// pass an uninitialized file object, the function will ask the user for the
// filename and try to open it, returns true on success (file was opened),
// false if file couldn't be opened/created and -1 if the file selection
// dialog was cancelled
int SjLogDialog::OpenLogFile(wxFile& file, wxString& retFilename)
{
	wxWindowDisabler disabler(this);

	// get the file name
	// -----------------
	SjExtList extList; extList.AddExt(wxT("txt"));
	wxFileDialog dlg(this, _("Save"), wxT(""), wxT("log.txt"), extList.GetFileDlgStr(wxFD_SAVE), wxFD_SAVE|wxFD_CHANGE_DIR);
	if( dlg.ShowModal() != wxID_OK ) { return -1; }
	wxString filename = dlg.GetPath();

	// open file
	// ---------
	bool bOk wxDUMMY_INITIALIZE(false);
	if ( wxFile::Exists(filename) )
	{
		wxASSERT( wxYES != wxCANCEL );
		wxASSERT( wxNO != wxCANCEL );
		bool bAppend = false;
		switch( SjMessageBox(wxString::Format(_("Overwrite \"%s\"?"), filename.c_str()), SJ_PROGRAM_NAME,
		                     wxICON_QUESTION | wxYES_NO | wxCANCEL, this, NULL, NULL, _("Yes"), _("Append")) )
		{
			case wxYES:
				bAppend = false;
				break;

			case wxNO:
				bAppend = true;
				break;

			default:
				return -1;
		}

		if ( bAppend ) {
			bOk = file.Open(filename, wxFile::write_append);
		}
		else {
			bOk = file.Create(filename, true /* overwrite */);
		}
	}
	else {
		bOk = file.Create(filename);
	}

	retFilename = filename;

	return bOk;
}


void SjLogDialog::OnSave(wxCommandEvent& event)
{
	wxFile file;
	wxString fileName;
	int rc = OpenLogFile(file, fileName);
	if ( rc == -1 )
	{
		// cancelled
		return;
	}

	bool ok = rc != 0;

	size_t count = m_logGui->m_aPacked.GetCount();
	for ( size_t n = 0; ok && (n < count); n++ )
	{
		unsigned long severity, time;
		wxString      msg, scope;
		SjLogGui::ExplodeMessage(m_logGui->m_aPacked[n], severity, time, msg, scope);
		if( !scope.IsEmpty() )
			scope = wxT(" [") + scope + wxT("]");
		msg = SjLogGui::SingleLine(msg);

		wxString line;
		line << TimeStamp(wxT("%Y-%m-%d %H:%M:%S"), (time_t)time)
		     << wxString::Format(wxT(".%03i"), time%1000)
		     << wxT(": ")
		     << SjLogListCtrl::GetSeverityChar(severity)
		     << wxT(": ")
		     << msg
		     << scope
		     << wxTextFile::GetEOL();

		ok = file.Write(line);
	}

	if ( ok ) {
		ok = file.Close();
	}

	if ( !ok ) {
		wxLogError(wxT("Can't save log contents to \"%s\"."), fileName.c_str());
	}
	else {
		wxLogInfo(wxT("Log saved to \"%s\"."), fileName.c_str());
	}
}


void SjLogDialog::DoClose()
{
	s_dlg = NULL;
	if( IsModal() )
	{
		EndModal(wxID_OK);
	}
	else
	{
		Hide();
		Destroy();
	}
}


/*******************************************************************************
 * SjLogGui
 ******************************************************************************/


SjLogGui* SjLogGui::s_this = NULL;


SjLogGui::SjLogGui()
	: wxLogGui()
{
	// check for errors
	wxASSERT( wxThread::IsMain() );
	wxASSERT( s_this == NULL);

	// init
	m_catchErrors = 0;
	m_catchErrorsInMainThread = 0;
	m_autoOpen = -1;

	// set active logging target to this
	m_oldTarget = wxLog::SetActiveTarget(this);
	wxASSERT( wxLog::GetLogLevel() >= wxLOG_Info ); // true for wx 2.x and 3.x (we want wxLogInfo to work, however, we _pop up_ a message dialog only for warnings and errors)
	wxLog::SetVerbose(true); // verbose defaults to false on wx 3.x and wxLogInfo won't work therefore


	// remember the global logging target
	s_this = this;
}


SjLogGui::~SjLogGui()
{
	// check for errors
	wxASSERT( wxThread::IsMain() );
	wxASSERT( s_this );

	// restore the active logging target
	//wxLog::SetLogLevel(m_oldLevel);
	wxLog::SetActiveTarget(m_oldTarget);

	// clear the global logging target
	s_this = NULL;
}


void SjLogGui::StartCatchErrors()
{
	// this function starts saving logged errors in a
	// string instead of giving it to wxLogGui::DoLog()
	wxASSERT( s_this );
	if( s_this )
	{
		wxCriticalSectionLocker locker(s_this->m_critical);
		if( s_this->m_catchErrors == 0 )
		{
			s_this->m_catchedErrors.Clear();
		}

		s_this->m_catchErrors++;
		if( wxThread::IsMain() )
			s_this->m_catchErrorsInMainThread++;
	}
}


void SjLogGui::EndCatchErrors()
{
	// this function ends saving logged errors in a
	// string and gives them just do wxLogGui::DoLog();
	wxASSERT( s_this );
	wxASSERT( s_this->m_catchErrors );
	if( s_this )
	{
		wxCriticalSectionLocker locker(s_this->m_critical);
		if( s_this->m_catchErrors > 0 )
		{
			s_this->m_catchErrors--;
			if( wxThread::IsMain() )
				s_this->m_catchErrorsInMainThread--;

			// some corrections ...
			if( s_this->m_catchErrors == 0 )
				s_this->m_catchErrorsInMainThread = 0;
		}
	}
}


wxString SjLogGui::GetAndClearCatchedErrors()
{
	// may be called between calls to StartCatchErrors()/EndCatchErrors();
	// returns the catched errors or an empty string if there are no errors.
	// moreover, the catched errors are cleared.
	wxASSERT( s_this );
	wxASSERT( s_this->m_catchErrors );
	wxString catchedErrors;
	if( s_this )
	{
		wxCriticalSectionLocker locker(s_this->m_critical);
		catchedErrors = s_this->m_catchedErrors;
		s_this->m_catchedErrors.Clear();
	}

	return catchedErrors;
}


void SjLogGui::DoLogRecord(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info)
{
	// this function is called by the logging system eg. if
	// wxLogError() or wxLogWarning() is called.

	// If catching errors is enabled, we save the message to our string; otherwise we just
	// forward it to the default GUI logging routine, wxLogGui:DoLog()
	if( m_catchErrors )
	{
		if( m_catchErrorsInMainThread || !wxThread::IsMain() )
		{
			wxCriticalSectionLocker locker(m_critical);
			if( !m_catchedErrors.IsEmpty() )
			{
				m_catchedErrors.Prepend(wxT("; "));
			}
			m_catchedErrors.Prepend(msg);
			return;
		}
	}

	// forward to GUI logging
	wxLogGui::DoLogRecord(level, msg, info);
}


void SjLogGui::ExplodeMessage(const wxString& all___, unsigned long& severity, unsigned long& time, wxString& msg, wxString& scope)
{
	wxString temp;

	// get and strip severity
	temp = all___.BeforeFirst(wxT('|'));
	temp.ToULong(&severity);
	msg = all___.AfterFirst(wxT('|'));

	// get and strip time
	temp = msg.BeforeFirst(wxT('|'));
	temp.ToULong(&time);
	msg = msg.AfterFirst(wxT('|'));

	// now "msg" is message and optional scope enclosured by "[]"
	scope.Empty();
	int p = msg.Find(wxT('['), true/*from end*/);
	if( p!=-1 )
	{
		scope = msg.Mid(p+1);
		if( scope.Len()>=1 && scope.Last()==wxT(']') )
		{
			scope = scope.Left(scope.Len()-1);
			msg = msg.Left(p).Trim();
		}
	}

	// some finalizing translations (some stuff is logged before the translation system is available)
	if( msg.StartsWith(wxT("Loading "), &temp) )
	{
		msg.Printf(_("Loading %s"), temp.c_str());
	}
}


wxString SjLogGui::SingleLine(const wxString& s)
{
	wxString ret(s);
	ret.Replace(wxT("\n"), wxT(" "));
	ret.Replace(wxT("\r"), wxT(" "));
	ret.Replace(wxT("\r"), wxT(""));
	ret.Trim();
	while( ret.Find(wxT("  ")) != -1 ) {
		ret.Replace(wxT("  "), wxT(" "));
	}
	return ret;
}


void SjLogGui::Flush()
{
	wxASSERT( wxThread::IsMain() );

	if( m_bHasMessages )
	{
		// copy all messages to our "all time buffer"
		m_bHasMessages = false;

		#define MAX_MESSAGES 1000
		while( m_aPacked.GetCount() > MAX_MESSAGES*2 ) {
			m_aPacked.RemoveAt(0, MAX_MESSAGES);
		}

		long firstNewIndex = m_aPacked.GetCount();
		long i, iCount = m_aMessages.GetCount();
		wxLogLevel level;
		long errorIndex = -1, warningIndex = -1, messageIndex = -1;
		for( i = 0; i < iCount; i++ )
		{
			level = m_aSeverity[i];
			if( level == wxLOG_Error ) errorIndex = m_aPacked.GetCount();
			if( level == wxLOG_Warning ) warningIndex = m_aPacked.GetCount();
			if( level == wxLOG_Message ) messageIndex = m_aPacked.GetCount();

			m_aPacked.Add(wxString::Format(wxT("%lu|%lu|%s"),
			                               (unsigned long)level, (unsigned long)m_aTimes[i], m_aMessages[i].c_str()));
		}

		Clear();

		// any errors or warnings? (for "info/verbose" we do not open the dialog
		// - even not for messages in wx 3.x as they are equal to info/verbose and we use them for logging)
		// (wxLogInfo and wxLogMessage both results in a message, however, wxLogInfo is not executed if verbose is not set)
		if( errorIndex==-1 && warningIndex==-1 /*&& messageIndex==-1*/ && s_dlg==NULL ) {
			return;
		}

		// check if we should open the dialog
		bool autoOpenNow = GetAutoOpen();

		if( g_mainFrame && !g_mainFrame->IsAllAvailable() ) {
			autoOpenNow = false;
		}

		if( s_dlg ) {
			autoOpenNow = true;
		}

		// get dialog icon
		long aIndex;
		long style;
		     if ( errorIndex!=-1 )      { style = wxICON_STOP;        aIndex = errorIndex;              }
		else if ( warningIndex!=-1 )    { style = wxICON_EXCLAMATION; aIndex = warningIndex;            }
		else if ( messageIndex!=-1 )    { style = wxICON_INFORMATION; aIndex = messageIndex;            }
		else                            { style = wxICON_INFORMATION; aIndex = m_aPacked.GetCount()-1;  }

		// show the dialog ...
		if( !autoOpenNow )
		{
			// show in display
			if( g_mainFrame && !g_mainFrame->InConstruction() )
			{
				if( style == wxICON_STOP )
				{
					unsigned long severity, time;
					wxString      msg, scope;
					SjLogGui::ExplodeMessage(m_aPacked.Last(), severity, time, msg, scope);
					g_mainFrame->SetDisplayMsg(msg);
				}
			}
		}
		else if( g_mainFrame && g_mainFrame->IsEnabled() )
		{
			// ... open modeless
			if( s_dlg )
			{
				s_dlg->MessagesChanged(firstNewIndex);
				if( errorIndex!=-1 || warningIndex!=-1 || messageIndex!=-1 )
					s_dlg->Raise();
			}
			else
			{
				s_dlg = new SjLogDialog(this, g_mainFrame, style, aIndex);
				s_dlg->Show();
			}
		}
		else
		{
			// ... open modal
			if( s_dlg && s_dlg->IsModal() )
			{
				wxASSERT( 0 );
				s_dlg->MessagesChanged(firstNewIndex); // no raise - normally, this should not happen at all ...
			}
			else
			{
				if( s_dlg ) s_dlg->DoClose();
				SjLogDialog dlg(this, NULL, style, aIndex);
				dlg.ShowModal();
			}
		}
	}
}


void SjLogGui::OpenManually()
{
	if( s_this && g_mainFrame )
	{
		// open modeless
		if( s_dlg )
		{
			if( !s_dlg->m_showingDetails )
			{
				wxCommandEvent fwd(wxEVT_COMMAND_BUTTON_CLICKED, IDC_DETAILS_BUTTON);
				s_dlg->GetEventHandler()->AddPendingEvent(fwd);
			}
			s_dlg->Raise();
		}
		else
		{
			s_dlg = new SjLogDialog(s_this, g_mainFrame, wxICON_INFORMATION, -1);
			s_dlg->Show();
		}
	}
}


bool SjLogGui::GetAutoOpen()
{
	if( s_this == NULL ) {
		return true; // wxLogGui is assumed
	}

	if( g_tools == NULL || g_tools->m_config == NULL ) {
		return true; // can't read settings, default is true.
	}

	if( s_this->m_autoOpen == -1 ) {
		s_this->m_autoOpen = g_tools->m_config->Read(wxT("main/autoOpenConsole"), 1L)? 1L : 0L;
	}

	return (s_this->m_autoOpen!=0);
}


void SjLogGui::SetAutoOpen(bool autoOpen)
{
	if( s_this == NULL ) {
		return;
	}

	s_this->m_autoOpen = autoOpen? 1L : 0L;

	if( g_tools && g_tools->m_config ) {
		g_tools->m_config->Write(wxT("main/autoOpenConsole"), s_this->m_autoOpen);
	}
}
