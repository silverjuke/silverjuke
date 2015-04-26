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
 * File:    htmlwindow.cpp
 * Authors: Björn Petersen
 * Purpose: The html window used in the help and in the tag editor
 * OS:      independent
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/help/htmlwindow.h>


SjHtmlWindow::SjHtmlWindow(wxWindow* parent, int id, const wxPoint& pos, const wxSize& size, long style)
	: wxHtmlWindow(parent, id, pos, size, style)
{
	wxFont guiFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
	if( g_accelModule->m_flags&SJ_ACCEL_USE_VIEW_FONT_IN_DLG )
	{
		guiFont = g_mainFrame->m_baseStdFont;
	}

	int normalFontSize = guiFont.GetPointSize();
	int largeFontSize  = (int)((float)normalFontSize*1.5);

	int sizes[7] =
	{
		normalFontSize, normalFontSize, normalFontSize,
		normalFontSize,
		normalFontSize, largeFontSize, largeFontSize
	};

	SetFonts(guiFont.GetFaceName(), wxT(""), sizes);
	SetBorders(SJ_DLG_SPACE*2);
}


void SjHtmlWindow::InitPage()
{
	SetPage(wxT("<html><head></head><body bgcolor=\"#FFFFFF\" text=\"#000000\" link=\"#0000F0\">"));
}


void SjHtmlWindow::AddPropToPage(SjProp& prop)
{
	wxString    temp, code, value;

	#define TABLE_START     wxT("<table border=\"0\" cellpadding=\"1\" cellspacing=\"0\">")
	#define TABLE_END       wxT("</table>")
	bool    tableStarted    = FALSE;

	prop.InitIterator();
	while( prop.Next() )
	{
		long flags = prop.GetFlags();
		if( flags & SJ_PROP_HEADLINE )
		{
			if( tableStarted )
			{
				code += TABLE_END;
				tableStarted = FALSE;
			}
			code += wxT("<h1>") + SjTools::Htmlentities(prop.GetName()) + wxT("</h1>");
		}
		else
		{
			if( !tableStarted )
			{
				code += TABLE_START;
				tableStarted = TRUE;
			}

			code += wxT("<tr><td width=\"30%\" valign=\"top\">");

			if( flags & SJ_PROP_BOLD )
			{
				code += wxT("<b>");
			}

			if( !prop.GetName().IsEmpty() )
			{
				temp = SjTools::Htmlentities(prop.GetName());
				temp.Replace(wxT(" "), wxT("&nbsp;"));
				code += temp;
				code += wxT(":");
			}

			if( flags & SJ_PROP_BOLD )
			{
				code += wxT("</b>");
			}


			code += wxT("</td><td valign=\"top\" width=\"70%\">");

			value = prop.GetValue().Trim(TRUE).Trim(FALSE);

			if( flags & SJ_PROP_EMPTYIFEMPTY
			        && (value.IsEmpty() || value==_("n/a") || value==wxT("n/a")) )
			{
				code += wxT("<font color=\"#808080\">");
				code += _("n/a");
				code += wxT("</font>");
			}
			else if( flags & SJ_PROP_ID )
			{
				code += wxString::Format(wxT("<a href=\"id:%i\">"), (int)(flags&0x0000FFFFL));
				code += SjTools::Htmlentities(value);
				code += wxT("</a>");
			}
			else if( flags & SJ_PROP_HTML )
			{
				code += value;
			}
			else if( g_tools->IsUrlExplorable(value) )
			{
				code += wxString::Format(wxT("<a href=\"ext:/") wxT("/%s\">"), SjTools::Htmlentities(value).c_str());
				code += SjTools::Htmlentities(value);
				code += wxT("</a>");
			}
			else
			{
				code += SjTools::Htmlentities(value);
			}

			code += wxT("</td></tr>");
		}
	}

	if( tableStarted )
	{
		code += TABLE_END;
	}

	AppendToPage(code);
}


void SjHtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
	wxString href = link.GetHref();
	wxString rest;

	if( href.StartsWith(wxT("ext://"), &rest) )
	{
		g_tools->ExploreUrl(rest);
		return;
	}
	else if( href.StartsWith(wxT("http://")) )
	{
		g_tools->ExploreUrl(href);
		return;
	}
	else if( href.StartsWith(wxT("web:"), &rest) )
	{
		// explore the silverjuke homepage
		long homepageId;
		rest.AfterLast(',').ToLong(&homepageId);
		g_tools->ExploreHomepage((SjHomepageId)homepageId);
		return;
	}
	else if( href.StartsWith(wxT("id:"), &rest) )
	{
		// just send the id as an command event to the top level window where
		// this html windows is placed in
		long id;
		rest.ToLong(&id);
		wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, id);
		SjDialog::FindTopLevel(this)->AddPendingEvent(fwd);
		return;
	}
	else if( href.StartsWith(wxT("page:"), &m_clickedLink) )
	{
		// go to another page, used by the help window;
		// needed at least for "more..." in the page after buying silverjuke
		wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, IDO_LINKCLICKED);
		SjDialog::FindTopLevel(this)->AddPendingEvent(fwd);
		return;
	}

	wxLogError(wxT("Unknown link \"%s\"."), href.c_str());
}
