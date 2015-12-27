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
 * File:    help.cpp
 * Authors: Björn Petersen
 * Purpose: The "help" module
 * OS:      independent
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/help/help.h>
#include <sjmodules/help/htmlwindow.h>
#include <sjmodules/basicsettings.h>

#define IDM_HTMLWINDOW (IDM_FIRSTPRIVATE+4)

SjHelpModule* g_helpModule = NULL;


/*******************************************************************************
 * SjHelpDialog
 ******************************************************************************/


class SjHelpDialog : public SjDialog
{
public:
	                SjHelpDialog        (SjHelpModule* helpModule, wxWindow* parent);
	void            GotoHtml            (const wxString& html, const wxString& title);
	void            GotoProp            (SjProp&);
	void            GotoTopic           (SjHelpTopicId);

private:
	SjHtmlWindow*   m_htmlWindow;

	wxString        GetAboutTopic       () const;
	wxString        GetCreditsTopic     () const;
	wxString        LoadTopic           (SjHelpTopicId, wxString& retTitle) const;

	void            OnCommand           (wxCommandEvent&);
	void            OnMyCancel          (wxCommandEvent&) { g_helpModule->CloseHelp(); }
	void            OnMyClose           (wxCloseEvent&) { g_helpModule->CloseHelp(); }
	DECLARE_EVENT_TABLE ();

	friend class    SjHelpModule;
};


BEGIN_EVENT_TABLE(SjHelpDialog, SjDialog)
	EVT_MENU        (IDO_LINKCLICKED,   SjHelpDialog::OnCommand         )
	EVT_BUTTON      (wxID_OK,           SjHelpDialog::OnMyCancel        )
	EVT_BUTTON      (wxID_CANCEL,       SjHelpDialog::OnMyCancel        )
	EVT_CLOSE       (                   SjHelpDialog::OnMyClose         )
END_EVENT_TABLE()


SjHelpDialog::SjHelpDialog(SjHelpModule* helpModule, wxWindow* parent)
	: SjDialog(parent, _("Help"), SJ_MODELESS, SJ_RESIZEABLE_IF_POSSIBLE)
{
	// create dialog
	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	// create the html window
	m_htmlWindow = new SjHtmlWindow(this, IDM_HTMLWINDOW,
	                                wxDefaultPosition, wxSize(200, 100) /*min. size, dailog size set below*/, wxSUNKEN_BORDER | wxHW_SCROLLBAR_AUTO);
	sizer1->Add(m_htmlWindow, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	// create the button sizer
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(buttonSizer, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	// align close button to the right
	buttonSizer->Add(1, 1, 1, wxGROW|wxRIGHT, SJ_DLG_SPACE*2);

	// add close button
	wxButton* cancelButton = new wxButton(this, wxID_CANCEL,  _("Close"));
	buttonSizer->Add(cancelButton, 0, wxBOTTOM, SJ_DLG_SPACE);

	// set default button, set size hints
	cancelButton->SetDefault();
	sizer1->SetSizeHints(this);

	// set accelerators
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_SETTINGS));
}


void SjHelpDialog::OnCommand(wxCommandEvent& event)
{
	switch( event.GetId() )
	{
		case IDO_LINKCLICKED:
			// needed at least for "more..." in the page after buying silverjuke
			long topicId;
			if( !m_htmlWindow->GetClickedLink().ToLong(&topicId, 10) ) { topicId = 0; }
			GotoTopic((SjHelpTopicId)topicId);
			break;
	}

	m_htmlWindow->SetFocus();
}


/*******************************************************************************
 * SjHelpDialog - Open a complete HTML page
 ******************************************************************************/


void SjHelpDialog::GotoHtml(const wxString& html, const wxString& title)
{
	// set content
	m_htmlWindow->InitPage();
	m_htmlWindow->AppendToPage(wxT("<p><img src=\"memory:aboutlogo.gif\"></p>")+html);

	// set title
	SetTitle(title);
}


/*******************************************************************************
 *  SjHelpDialog - Open a property page
 ******************************************************************************/


void SjHelpDialog::GotoProp(SjProp& prop)
{
	// get the title
	wxString title(_("Info"));
	prop.InitIterator();
	if( prop.Next() )
	{
		title += wxT(" - ") + (prop.GetFlags()&SJ_PROP_HEADLINE? prop.GetName() : prop.GetValue());
	}

	// set content
	m_htmlWindow->InitPage();
	m_htmlWindow->AddPropToPage(prop);

	// set title
	SetTitle(title);
}


/*******************************************************************************
 * SjHelpDialog - Open a topic from the locale
 ******************************************************************************/


wxString SjHelpDialog::GetAboutTopic() const
{
	wxString ret;

	ret = wxT("<center>")
	      wxT("<p>&nbsp;<img src=\"memory:aboutlogo.gif\">&nbsp;</p>") // the &nbsp; left and right are needed for wxHTML to calculate the correct height; a simple image is not sufficient
	      wxT("<h1>") + wxString::Format(wxT("%s %i.%i.%i"), SJ_PROGRAM_NAME, SJ_VERSION_MAJOR, SJ_VERSION_MINOR, SJ_VERSION_REVISION) + wxT("</h1>")
	      wxT("<p>The Jukebox. Grown up.</p>") /*n/t*/;

	if( g_mainFrame->IsAllAvailable() )
	{
		ret += wxT("<p><a href=\"web:0\">") + wxString::Format(_("%s on the web"), SJ_PROGRAM_NAME) + wxT("...</a></p>");
	}
	else
	{
		ret += wxT("<p>http://www.silverjuke.net</p>");
	}

	ret += wxT("<p>Copyright &copy; 2016 Bj&ouml;rn Petersen Software Design and Development</p>")
	      wxT("<p><a href=\"page:3\">Credits and License notes...</a></p>")
	      wxT("</center>");

	return ret;
}


wxString SjHelpDialog::GetCreditsTopic() const
{
	static const char* credits =
	    "<p>"
	    "Credits (not only) to third parties:"
	    "<br />&nbsp;"
	    "</p>"
	    "<ul>"
	    "<li>"
	    "Parts of this software use the wxWidgets User Interface Framework written by the wxWidgets team."
	    "<br />"
	    "</li>"
	    "<li>"
	    "Other parts are based on the work of the Independent JPEG Group "
	    "and on the work of Jean-loup Gailly and Mark Adler."
	    "<br />"
	    "</li>"
	    "<li>"
	    "The used SQLite Database Library was written by Dr. Richard Hipp."
	    "<br />"
	    "</li>"
	    "<li>"
	    "The used Simple ECMAScript Engine is covered by the following notice:<br />"
	    "<br />"
	    "<i>Copyright (c) 2003, 2004 David Leonard. All rights reserved.<br />"
	    "<br />"
	    "Redistribution and use in source and binary forms, with or without modification, "
	    "are permitted provided that the following conditions are met:<br />"
	    "<br />"
	    "1. Redistributions of source code must retain the above copyright notice, this list "
	    "of conditions and the following disclaimer.<br />"
	    "2. Redistributions in binary form must reproduce the above copyright notice, "
	    "this list of conditions and the following disclaimer in the documentation and/or "
	    "other materials provided with the distribution.<br />"
	    "3. Neither the name of Mr Leonard nor the names of the contributors may be used "
	    "to endorse or promote products derived from this software without specific prior "
	    "written permission.<br />"
	    "<br />"
	    "THIS SOFTWARE IS PROVIDED BY DAVID LEONARD AND CONTRIBUTORS &quot;AS IS&quot; AND "
	    "ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE "
	    "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE "
	    "ARE DISCLAIMED. IN NO EVENT SHALL DAVID LEONARD OR CONTRIBUTORS BE LIABLE "
	    "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL "
	    "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS "
	    "OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) "
	    "HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT "
	    "LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY "
	    "OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF "
	    "SUCH DAMAGE.</i>"
	    "<br />"
	    "</li>"
	    "<li>"
	    "Number-to-string and string-to-number conversions are covered by the following notice:<br />"
	    "<br />"
	    "<i>The author of this software is David M. Gay.<br />"
	    "<br />"
	    "Copyright (c) 1991, 2000 by Lucent Technologies.<br />"
	    "<br />"
	    "Permission to use, copy, modify, and distribute this software for any purpose without "
	    "fee is hereby granted, provided that this entire notice is included in all copies of "
	    "any software which is or includes a copy or modification of this software and in all "
	    "copies of the supporting documentation for such software.<br />"
	    "<br />"
	    "THIS SOFTWARE IS BEING PROVIDED &quot;AS IS&quot;, WITHOUT ANY EXPRESS OR IMPLIED "
	    "WARRANTY. IN PARTICULAR, NEITHER THE AUTHOR NOR LUCENT MAKES ANY REPRESENTATION OR "
	    "WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS "
	    "FOR ANY PARTICULAR PURPOSE.</i><br />"
	    "</li>"
	    "<li>"
	    "The tagging code is based upon the work of Scott Wheeler's taglib, "
	    "(C) 2002 - 2008 by Scott Wheeler"
	    "<br />"
	    "</li>"
	    "<li>"
	    "The Silverjuke core is available as free software under the GPL. See the GNU General Public License for more details."
	    "</li>"
	    "</ul>"
	    "<p>"
	    "Björn Petersen Software Design and Development is grateful to the groups and individuals above for their contributions."
	    "</p>"
	    "<p>"
	    "<a href=\"page:0\">Back</a>"
	    "</p>";

	return  wxString(credits, wxConvUTF8);
}


wxString SjHelpDialog::LoadTopic(SjHelpTopicId topicId, wxString& retTitle) const
{
	wxString helpText;
	retTitle = wxString::Format(_("About %s"), SJ_PROGRAM_NAME);
	if( topicId == SJ_HELP_TOPIC_CREDITS )
	{
		helpText = GetCreditsTopic();
	}
	else
	{
		helpText = GetAboutTopic();
	}
	return helpText;
}


void SjHelpDialog::GotoTopic(SjHelpTopicId topicId)
{
	wxString title;
	wxString html = LoadTopic(topicId, title);

	// set content
	m_htmlWindow->InitPage();
	m_htmlWindow->AppendToPage(html);

	// set title
	SetTitle(title);
}


/*******************************************************************************
 * SjHelpModule
 ******************************************************************************/


SjHelpModule::SjHelpModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file          = wxT("memory:help.lib");
	m_name          = _("Help-system");
	m_helpDialog    = NULL;
}


bool SjHelpModule::FirstLoad()
{
	wxASSERT(g_helpModule==NULL);
	g_helpModule = this;
	return TRUE;
}


void SjHelpModule::LastUnload()
{
	wxASSERT(g_helpModule!=NULL);

	CloseHelp();

	g_helpModule = NULL;
}


void SjHelpModule::OpenHelp(wxWindow* parent, SjHelpTopicId topicId, SjProp* prop, const wxString& html,
                            const wxString& title)
{
	wxBusyCursor busy;

	if( m_helpDialog == NULL )
	{
		m_helpDialog = new SjHelpDialog(this, g_mainFrame);
		m_helpDialog->SetSize(580, 380);
	}

	if( prop )
	{
		m_helpDialog->GotoProp(*prop);
	}
	else if( html != wxT("") )
	{
		m_helpDialog->GotoHtml(html, title);
	}
	else
	{
		m_helpDialog->GotoTopic(topicId);
	}

	m_helpDialog->Show();
	m_helpDialog->Raise();
	m_helpDialog->m_htmlWindow->SetFocus();
}


void SjHelpModule::CloseHelp()
{
	if( m_helpDialog )
	{
		m_helpDialog->Hide();
		m_helpDialog->Destroy();
		m_helpDialog = NULL;
	}
}


void SjHelpModule::ReceiveMsg(int msg)
{
	if( msg == IDMODMSG_WINDOW_ICONIZED
	 || msg == IDMODMSG_KIOSK_STARTING
	 || msg == IDMODMSG_KIOSK_ENDED
	 || msg == IDMODMSG_WINDOW_CLOSE )
	{
		CloseHelp();
	}
}
