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
 * File:    timeout.h
 * Authors: Björn Petersen
 * Purpose: Smart timeout handling
 *
 ******************************************************************************/



#ifndef __SJ_TIMEOUT_H__
#define __SJ_TIMEOUT_H__



unsigned long SjTimestampDiff(unsigned long first, unsigned long afterFirst);



class SjTimeoutWatcher
{
public:
	// constructor/init timeout
	SjTimeoutWatcher    (long timeoutMs) { SetTimeout(timeoutMs); }
	SjTimeoutWatcher    () { Clear(); }

	void            SetTimeout          (long ms);
	void            Clear               () { m_timeoutSet = FALSE; }

	// reset the timeout, call this function eg. on user input
	void            Reset               ();

	// has the number of ms elapsed between calling Set()/Reset()
	// and calling IsExpired()?
	bool            IsExpired           ();

private:
	// private stuff
	bool            m_timeoutSet;
	unsigned long   m_timeoutMs;
	unsigned long   m_lastInputMs;
};




class SjUptimeWatcher
{
public:
	// constructor
	SjUptimeWatcher     (const wxString& reg) { m_reg = reg; m_started = FALSE; }

	// members
	void            StartWatching       ();
	void            StopWatching        ();
	long            GetSeconds          ();

private:
	// private
	bool            m_started;
	wxString        m_reg;
	wxDateTime      m_start;
};



#endif // __SJ_TIMEOUT_H__
