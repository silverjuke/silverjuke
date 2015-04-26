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
 * File:    help.h
 * Authors: Björn Petersen
 * Purpose: The "help" module
 *
 ******************************************************************************/


#ifndef __SJ_HELP_H__
#define __SJ_HELP_H__


class SjHelpDialog;


enum SjHelpTopicId
{
    SJ_HELP_TOPIC_ABOUT = 0,
    SJ_HELP_TOPIC_CREDITS = 3,
    SJ_HELP_TOPIC_VOID = 666
};



class SjHelpModule : public SjCommonModule
{
public:
	                SjHelpModule        (SjInterfaceBase*);

	void            OpenHtml            (wxWindow* parent, const wxString& html, const wxString& title) { OpenHelp(parent, SJ_HELP_TOPIC_VOID,  NULL, html,         title); }
	void            OpenTopic           (wxWindow* parent, SjHelpTopicId topicId)                       { OpenHelp(parent, topicId,             NULL, wxT(""),      wxT(""));   }

	void            CloseHelp           ();

	bool            IsHelpOpen          () const { return (m_helpDialog!=NULL); }

protected:
	bool            FirstLoad           ();
	void            LastUnload          ();

private:
	void            ReceiveMsg          (int msg);
	void            OpenHelp            (wxWindow* parent, SjHelpTopicId, SjProp*, const wxString& html, const wxString& title);
	SjHelpDialog*   m_helpDialog;

	friend class    SjHelpDialog;
	friend class    SjHelpHistoryItem;
};


extern SjHelpModule* g_helpModule;


#endif // __SJ_HELP_H__
