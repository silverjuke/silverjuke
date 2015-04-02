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
 * File:    msgboxsj.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke tools
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/msgbox.h>
#include <wx/artprov.h>


/*******************************************************************************
 * SjMessageDlg class
 ******************************************************************************/


class SjMessageDlg : public SjDialog
{
public:
	SjMessageDlg        (const wxString& message, const wxString& caption,
	                     int style,
	                     wxWindow *parent,
	                     const wxArrayString* options, int* selOption,
	                     const wxString& yesTitle,
	                     const wxString& noTitle,
	                     wxBitmap* bitmap);
	int             GetSelOption        ();

private:
	int             m_style;
	wxButton*       m_yesButton;
	wxButton*       m_noButton;
	wxArrayPtrVoid  m_options;
	void            SetFocusToDefaultButton
	();

	// TransferDataToWindow() function is called by the default wxInitDialogEvent-handler,
	// we just set the focus to the "OK" button here as, at least under MSW,
	// this does not work in the constructor and the focus stays eg. on the
	// checkbox, which is ugly for such a minor option
	bool            TransferDataToWindow() { SetFocusToDefaultButton(); return TRUE; }

	void            OnNo                (wxCommandEvent&);
	DECLARE_EVENT_TABLE ()
};


BEGIN_EVENT_TABLE(SjMessageDlg, SjDialog)
	EVT_BUTTON(wxID_NO, SjMessageDlg::OnNo)
END_EVENT_TABLE()


SjMessageDlg::SjMessageDlg(const wxString& message, const wxString& caption,
                           int style,
                           wxWindow *parent,
                           const wxArrayString* options, int* selOption,
                           const wxString& yesTitle__,
                           const wxString& noTitle__,
                           wxBitmap* bitmap__)
	: SjDialog(parent, caption, SJ_MODAL, SJ_NEVER_RESIZEABLE)
{
	m_style = style;

	// correct button texts
	wxString yesTitle(yesTitle__);
	wxString noTitle(noTitle__);

	if( yesTitle.IsEmpty() ) yesTitle = (style&wxYES)? _("Yes") : _("OK");
	if( noTitle.IsEmpty() ) noTitle = (style&wxNO)? _("No") : _("Cancel");

	// create dialog
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	wxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxALL, SJ_DLG_SPACE);

	// icon
	if( bitmap__ && bitmap__->Ok() )
	{
		wxStaticBitmap *icon = new wxStaticBitmap(this, -1, *bitmap__);
		sizer2->Add(icon, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, SJ_DLG_SPACE);
	}
	else if( (style & wxICON_MASK) )
	{
		wxBitmap bitmap;
		switch ( style & wxICON_MASK )
		{
			default:
				wxFAIL_MSG(_T("incorrect log style"));
				// fall through

			case wxICON_ERROR:
				bitmap = wxArtProvider::GetIcon(wxART_ERROR, wxART_MESSAGE_BOX);
				break;

			case wxICON_INFORMATION:
				bitmap = wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MESSAGE_BOX);
				break;

			case wxICON_WARNING:
				bitmap = wxArtProvider::GetIcon(wxART_WARNING, wxART_MESSAGE_BOX);
				break;

			case wxICON_QUESTION:
				bitmap = wxArtProvider::GetIcon(wxART_QUESTION, wxART_MESSAGE_BOX);
				break;
		}
		wxStaticBitmap *icon = new wxStaticBitmap(this, -1, bitmap);
		sizer2->Add(icon, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, SJ_DLG_SPACE);
	}

	wxSizer* sizer3 = new wxBoxSizer(wxVERTICAL);
	sizer2->Add(sizer3, 0, wxALL|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	// text
	wxStaticText* st = new wxStaticText(this, -1, message);
	st->Wrap(SJ_MSG_BOX_WRAP);
	sizer3->Add(st);

	// options
	if( options && selOption )
	{
		int optionsCount = options->GetCount();
		sizer3->Add(SJ_DLG_SPACE*2, SJ_DLG_SPACE*2);

		if( optionsCount == 1 )
		{
			// add a simple check box
			wxCheckBox* checkBox = new wxCheckBox(this, -1, options->Item(0));
			checkBox->SetValue((*selOption)!=0);

			sizer3->Add(checkBox);
			m_options.Add(checkBox);
		}
		else
		{
			// add a list of radio buttons
			wxRadioButton* radioButton;
			int i;
			for( i = 0; i < optionsCount; i++ )
			{
				if( i )
				{
					sizer3->Add(SJ_DLG_SPACE/2, SJ_DLG_SPACE/2);
				}
				radioButton = new wxRadioButton(this, -1, options->Item(i),
				                                wxDefaultPosition, wxDefaultSize, i==0? wxRB_GROUP : 0);
				wxASSERT( selOption );
				if( i == *selOption )
				{
					radioButton->SetValue(TRUE);
				}
				sizer3->Add(radioButton);
				m_options.Add(radioButton);
			}
		}
	}

	sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxALL|wxGROW, SJ_DLG_SPACE);

	// construct the buttons
	m_yesButton = new wxButton(this, wxID_OK, yesTitle);
	m_noButton = NULL;
	wxButton* cancelButton = NULL;
	if( style & (wxNO|wxCANCEL) )
	{
		bool hasExplicitCancelButton = ((style&wxCANCEL) && (style&wxNO));

		m_noButton = new wxButton(this, (hasExplicitCancelButton)? wxID_NO : wxID_CANCEL, noTitle);

		if( hasExplicitCancelButton )
		{
			cancelButton = new wxButton(this, wxID_CANCEL, _("Cancel"));
		}
	}

	// add the buttons
	sizer2->Add(SJ_DLG_SPACE, SJ_DLG_SPACE, 1/*grow*/);


	sizer2->Add(m_yesButton, 0, 0, SJ_DLG_SPACE);
	if( m_noButton ) sizer2->Add(m_noButton, 0, wxLEFT, SJ_DLG_SPACE);

	if( cancelButton ) sizer2->Add(cancelButton, 0, wxLEFT, SJ_DLG_SPACE);


	sizer2->Add(SJ_DLG_SPACE, SJ_DLG_SPACE, 1/*grow*/);

	sizer1->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);

	sizer1->SetSizeHints(this);
	CenterOnParent();
	SetFocusToDefaultButton();
}


int SjMessageDlg::GetSelOption()
{
	int ret = 0;
	int optionsCount = m_options.GetCount(), i;

	if( optionsCount == 1 )
	{
		wxCheckBox* checkBox = (wxCheckBox*)(m_options[0]);
		wxASSERT( checkBox );

		ret = checkBox->IsChecked()? 1 : 0;
	}
	else
	{
		for( i = 0; i < optionsCount; i++ )
		{
			wxRadioButton* radioButton = (wxRadioButton*)(m_options[i]);
			wxASSERT( radioButton );
			if( radioButton->GetValue() )
			{
				ret = i;
				break;
			}
		}
	}

	return ret;
}


void SjMessageDlg::SetFocusToDefaultButton()
{
	if( (m_style & wxNO_DEFAULT) && m_noButton )
	{
		m_noButton->SetDefault();
		m_noButton->SetFocus();
	}
	else
	{
		wxASSERT( m_yesButton );
		m_yesButton->SetDefault();
		m_yesButton->SetFocus();
	}
}


void SjMessageDlg::OnNo(wxCommandEvent& WXUNUSED(event))
{
	EndModal(wxID_NO);
}


/*******************************************************************************
 * SjMessageBox function
 ******************************************************************************/


int SjMessageBox        (const wxString& message, const wxString& caption,
                         int style,
                         wxWindow *parent,
                         const wxArrayString* options, int* selOption,
                         const wxString& yesTitle, const wxString& noTitle,
                         wxBitmap* bitmap)
{
	wxWindowDisabler disabler(parent);

	wxASSERT( wxYES != 0 );
	wxASSERT( wxNO != 0 );
	wxASSERT( wxOK != 0 );
	wxASSERT( wxCANCEL != 0 );
	wxASSERT( wxYES_NO != 0 );

	int userChoice = 0;
	bool useViewFontInDlg = false;

	if( g_accelModule )
	{
		if( g_accelModule->m_flags&SJ_ACCEL_USE_VIEW_FONT_IN_DLG )
			useViewFontInDlg = true;
	}

#ifndef __WXMAC__
	if( yesTitle.IsEmpty()
	        && noTitle.IsEmpty()
	        && options == NULL
	        && !useViewFontInDlg )
	{
		// fallback to the default message box
		// (under wxMac, wxMessageBox() forces a context switch (?) which re-allows eg. Cmd+Tab in the kiosk mode - so we don't use it there)
		userChoice = wxMessageBox(message, caption, style, parent);
	}
	else
#endif
	{
		// use our message box
		SjMessageDlg dlg(message, caption, style, parent, options, selOption,
		                 yesTitle, noTitle, bitmap);
		int buttonId = dlg.ShowModal();
		if( buttonId == wxID_OK )
		{
			// OK or YES clicked
			if( selOption )
			{
				*selOption = dlg.GetSelOption();
			}
			userChoice = (style&wxYES)? wxYES : wxOK;
		}
		else if( buttonId == wxID_NO )
		{
			// NO clicked
			if( selOption )
			{
				*selOption = dlg.GetSelOption();
			}
			userChoice = wxNO;
		}
		else
		{
			// NO or CANCEL clicked
			userChoice = ((style&wxNO) && !(style&wxCANCEL))? wxNO : wxCANCEL;
		}
	}

	return userChoice;
}




