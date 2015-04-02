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
 * File:    sj_console.h
 * Authors: Björn Petersen
 * Purpose: Logging that may be used to catch errors in threads
 *
 ******************************************************************************/



#ifndef __SJ_CONSOLE_H__
#define __SJ_CONSOLE_H__



class SjLogGui : public wxLogGui
{
public:
	// it's up to the caller to clear the string if errors are shown!
	SjLogGui            ();
	~SjLogGui           ();
	static void     DiscardAll          () { if(s_this) { s_this->Clear(); } }
	static void     OpenManually        ();

	// all messages, these may be much more than the "most recent" ones
	static void     ExplodeMessage      (const wxString&, unsigned long& severity, unsigned long& time, wxString& msg, wxString& scope);
	static wxString SingleLine          (const wxString&);

	// auto open?
	static bool     GetAutoOpen         ();
	static void     SetAutoOpen         (bool);

protected:
	// overwritten methods
	virtual void    DoLog               (wxLogLevel level, const wxChar* msg, time_t timestamp);

	// show all messages that were logged since the last Flush()
	virtual void    Flush               ();

private:
	// private
	static SjLogGui*
	s_this;

	wxArrayString   m_aPacked;

	long            m_catchErrors;
	long            m_catchErrorsInMainThread;
	wxString        m_catchedErrors;
	static void     StartCatchErrors    ();
	static void     EndCatchErrors      ();
	static wxString GetAndClearCatchedErrors
	();

	wxLog*          m_oldTarget;
	wxLogLevel      m_oldLevel;
	wxCriticalSection
	m_critical;

	long            m_autoOpen;

	friend class    SjLogString;
	friend class    SjLogListCtrl;
	friend class    SjLogDialog;
};



class SjLogString
{
public:
	                SjLogString         (bool logToString=TRUE) { m_logToString=logToString; if( m_logToString ) { SjLogGui::StartCatchErrors(); } }
	               ~SjLogString        () { if( m_logToString ) { SjLogGui::EndCatchErrors(); } }
	wxString       GetAndClearErrors   () { if( m_logToString ) { return SjLogGui::GetAndClearCatchedErrors(); } else { return wxT(""); } }

private:
	bool            m_logToString;
};


#endif // __SJ_CONSOLE_H__
